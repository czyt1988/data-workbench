#!/usr/bin/env python3
"""LLM API error classification module.

Classifies exceptions from openai-python SDK (and mid-stream interruptions)
into retryable / non-retryable categories with error-type labels for the
protocol layer and UI.

When the ``openai`` package is available, classification uses ``isinstance``
checks against the SDK exception hierarchy. When it is not available
(``_HAS_OPENAI = False``), all ``isinstance`` checks are skipped and
classification degrades to keyword matching on ``str(exc)``.
"""

import asyncio
import logging
from dataclasses import dataclass
from datetime import datetime, timezone
from email.utils import parsedate_to_datetime

# --- openai import guard -------------------------------------------------
try:
    import openai
    _HAS_OPENAI = True
except ImportError:
    _HAS_OPENAI = False

# --- context_manager import (same directory) -----------------------------
# is_context_overflow_error is defined in context_manager.py. If that module
# is unavailable (heavy langchain deps missing), fall back to local keyword
# matching that mirrors the same keyword list.
try:
    from context_manager import is_context_overflow_error
    _HAS_CONTEXT_MANAGER = True
except ImportError:
    _HAS_CONTEXT_MANAGER = False

logger = logging.getLogger(__name__)

# Context-overflow keywords — must match context_manager.is_context_overflow_error
_CONTEXT_OVERFLOW_KEYWORDS = [
    "context length",
    "maximum context",
    "contextwindowexceedederror",
    "context window",
    "too many tokens",
    "request is invalid",  # litellm wrapped message
]


class ErrorType:
    """Error-type labels used in the ``error`` protocol message's ``error_type`` field."""
    QUOTA_EXHAUSTED = "quota_exhausted"
    AUTH_ERROR = "auth_error"
    RATE_LIMIT_EXHAUSTED = "rate_limit_exhausted"
    NETWORK_EXHAUSTED = "network_exhausted"
    SERVER_ERROR_EXHAUSTED = "server_error_exhausted"
    BAD_REQUEST = "bad_request"
    CONTEXT_OVERFLOW = "context_overflow"
    UNKNOWN = "unknown"


@dataclass
class ErrorClassification:
    """Result of classifying an LLM exception."""
    retryable: bool           # whether the error is retryable
    error_type: str           # ErrorType constant for protocol/UI
    user_message: str         # short user-facing description (English, C++ translates)
    detail: str               # raw error detail for logging


# --- internal helpers -----------------------------------------------------

def _check_context_overflow(exc: Exception) -> bool:
    """Check if *exc* is a context-window overflow error."""
    if _HAS_CONTEXT_MANAGER:
        try:
            return is_context_overflow_error(exc)
        except Exception:
            pass  # fall through to keyword matching
    exc_str = str(exc).lower()
    return any(kw in exc_str for kw in _CONTEXT_OVERFLOW_KEYWORDS)


def _is_quota_exhausted(exc: Exception, exc_str: str) -> bool:
    """RateLimitError whose body indicates quota exhaustion (non-retryable)."""
    quota_keywords = ("insufficient_quota", "exceeded_current_quota")
    has_quota_keyword = any(kw in exc_str for kw in quota_keywords)
    if _HAS_OPENAI:
        return isinstance(exc, openai.RateLimitError) and has_quota_keyword
    # Degraded path: keyword-only
    return has_quota_keyword


def _is_auth_error(exc: Exception, exc_str: str) -> bool:
    """Authentication / authorization failure (non-retryable)."""
    if _HAS_OPENAI:
        if isinstance(exc, openai.AuthenticationError):
            return True
        # Also catch APIStatusError with 401/403 (some providers use non-standard subclasses)
        if isinstance(exc, openai.APIStatusError):
            status = getattr(exc, "status_code", None)
            if status in (401, 403):
                return True
        return False
    # Degraded path
    auth_keywords = ("401", "403", "authentication", "unauthorized", "forbidden")
    return any(kw in exc_str for kw in auth_keywords)


def _is_bad_request(exc: Exception, exc_str: str) -> bool:
    """Bad-request format error, excluding context overflow (non-retryable)."""
    if _HAS_OPENAI:
        return isinstance(exc, openai.BadRequestError)
    # Degraded path
    bad_request_keywords = ("400", "bad request")
    return any(kw in exc_str for kw in bad_request_keywords)


def _is_rate_limit(exc: Exception, exc_str: str) -> bool:
    """Rate limiting that is NOT quota exhaustion (retryable)."""
    if _HAS_OPENAI:
        return isinstance(exc, openai.RateLimitError)
    # Degraded path
    rate_limit_keywords = ("429", "rate limit")
    return any(kw in exc_str for kw in rate_limit_keywords)


def _is_server_error(exc: Exception, exc_str: str) -> bool:
    """HTTP 5xx server error (retryable).

    Checked *after* the more specific subclasses (BadRequestError,
    AuthenticationError, RateLimitError) so those are not miscaught.
    """
    if _HAS_OPENAI:
        if isinstance(exc, openai.InternalServerError):
            return True
        if isinstance(exc, openai.APIStatusError):
            status = getattr(exc, "status_code", None)
            if status in (500, 502, 503, 504, 529):
                return True
        return False
    # Degraded path
    server_keywords = (
        "500", "502", "503", "504", "529",
        "internal server", "server error", "bad gateway",
        "service unavailable", "gateway timeout",
    )
    return any(kw in exc_str for kw in server_keywords)


def _is_network_error(exc: Exception, exc_str: str) -> bool:
    """Network connection / timeout error (retryable)."""
    if _HAS_OPENAI:
        return isinstance(exc, (openai.APIConnectionError, openai.APITimeoutError))
    # Degraded path
    network_keywords = ("connection", "timeout", "timed out", "network", "connect")
    return any(kw in exc_str for kw in network_keywords)


def _is_stream_interrupted(exc_str: str) -> bool:
    """Mid-stream interruption via raw exception string (retryable).

    Catches httpx.ReadError, TypeError('terminated'), ValueError('Empty
    response from LLM') etc. that are not openai SDK exceptions.
    """
    stream_keywords = (
        "terminated", "connection", "network", "disconnected", "empty response",
    )
    return any(kw in exc_str for kw in stream_keywords)


# --- public API -----------------------------------------------------------

def classify_error(exc: Exception) -> ErrorClassification:
    """Classify an LLM API exception into retryable/non-retryable categories.

    Checks are ordered most-specific-first so that e.g. a RateLimitError
    with quota-exhaustion body is caught before the generic APIStatusError
    5xx check.

    Empty-response detection is the *caller's* responsibility: if
    ``astream()`` yields no chunks, the caller should raise
    ``ValueError("Empty response from LLM")`` so this function classifies
    it via the stream-interruption path.
    """
    # 1. Context overflow (non-retryable)
    if _check_context_overflow(exc):
        return ErrorClassification(
            retryable=False,
            error_type=ErrorType.CONTEXT_OVERFLOW,
            user_message="Context window exceeded",
            detail=str(exc),
        )

    # 2. User cancel (non-retryable) — asyncio.CancelledError does not need openai
    if isinstance(exc, asyncio.CancelledError):
        return ErrorClassification(
            retryable=False,
            error_type=ErrorType.UNKNOWN,
            user_message="Unknown error",
            detail="Agent stopped by user",
        )

    exc_str = str(exc).lower()

    # 3. Quota exhausted (non-retryable)
    if _is_quota_exhausted(exc, exc_str):
        return ErrorClassification(
            retryable=False,
            error_type=ErrorType.QUOTA_EXHAUSTED,
            user_message="API quota exhausted",
            detail="API quota exhausted, please check account balance or change API key",
        )

    # 4. Authentication failure (non-retryable)
    if _is_auth_error(exc, exc_str):
        return ErrorClassification(
            retryable=False,
            error_type=ErrorType.AUTH_ERROR,
            user_message="Authentication failed",
            detail=str(exc),
        )

    # 5. Bad request format (non-retryable, excluding overflow already caught)
    if _is_bad_request(exc, exc_str):
        return ErrorClassification(
            retryable=False,
            error_type=ErrorType.BAD_REQUEST,
            user_message="Bad request",
            detail=str(exc),
        )

    # 6. Rate limit, non-quota (retryable)
    if _is_rate_limit(exc, exc_str):
        return ErrorClassification(
            retryable=True,
            error_type=ErrorType.RATE_LIMIT_EXHAUSTED,
            user_message="Rate limited",
            detail=str(exc),
        )

    # 7. Server error 5xx (retryable)
    if _is_server_error(exc, exc_str):
        return ErrorClassification(
            retryable=True,
            error_type=ErrorType.SERVER_ERROR_EXHAUSTED,
            user_message="Server error",
            detail=str(exc),
        )

    # 8. Network error (retryable)
    if _is_network_error(exc, exc_str):
        return ErrorClassification(
            retryable=True,
            error_type=ErrorType.NETWORK_EXHAUSTED,
            user_message="Network error",
            detail=str(exc),
        )

    # 9. Stream interrupted (retryable) — raw exceptions not from openai SDK
    if _is_stream_interrupted(exc_str):
        user_msg = "Empty response from LLM" if "empty response" in exc_str else "Network error"
        return ErrorClassification(
            retryable=True,
            error_type=ErrorType.NETWORK_EXHAUSTED,
            user_message=user_msg,
            detail=str(exc),
        )

    # 10. Unknown (non-retryable)
    return ErrorClassification(
        retryable=False,
        error_type=ErrorType.UNKNOWN,
        user_message="Unknown error",
        detail=str(exc),
    )


def extract_retry_after_ms(exc: Exception) -> int | None:
    """Extract ``Retry-After`` header value from *exc*, returning milliseconds.

    Supports both integer-seconds and HTTP-date formats per RFC 7231.
    Returns ``None`` when the header is absent or unparseable.
    """
    response = getattr(exc, "response", None)
    if response is None:
        return None
    headers = getattr(response, "headers", None)
    if headers is None:
        return None
    # httpx.Headers is case-insensitive, but try both just in case
    retry_after = headers.get("retry-after") or headers.get("Retry-After")
    if not retry_after:
        return None

    # Try integer seconds first
    try:
        seconds = int(retry_after)
        if seconds < 0:
            return None
        return seconds * 1000
    except (ValueError, TypeError):
        pass

    # Try HTTP-date format
    try:
        retry_time = parsedate_to_datetime(retry_after)
        if retry_time is None:
            return None
        # Ensure timezone-aware for comparison
        if retry_time.tzinfo is None:
            retry_time = retry_time.replace(tzinfo=timezone.utc)
        now = datetime.now(timezone.utc)
        delta = (retry_time - now).total_seconds()
        if delta <= 0:
            return None
        return int(delta * 1000)
    except (ValueError, TypeError, OverflowError):
        return None
