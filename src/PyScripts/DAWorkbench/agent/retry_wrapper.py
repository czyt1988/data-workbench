#!/usr/bin/env python3
"""Exponential-backoff retry wrapper for LLM API calls.

Pure standard-library implementation (asyncio + random). Does **not** import
``tenacity`` or ``backoff``. Does **not** import ``AgentStoppedError`` from
``agent_runner`` (would create a circular import); instead defines
``RetryAbortedError`` for the case where the user stops the agent during
backoff sleep.
"""

import asyncio
import logging
import random
from typing import Any, Callable

from error_classifier import classify_error, extract_retry_after_ms, ErrorClassification

logger = logging.getLogger(__name__)

# --- hardcoded constants (D8 decision) ------------------------------------
RETRY_BASE_DELAY_MS = 1000
RETRY_MAX_DELAY_MS = 30000
RETRY_BACKOFF_FACTOR = 2.0
RETRY_JITTER_RATIO = 0.25
DEFAULT_MAX_RETRIES = 7


class RetryAbortedError(Exception):
    """Raised when retry backoff sleep is aborted by user (stop_event set)."""
    pass


def compute_backoff_delay(
    attempt: int,
    base_delay: float = RETRY_BASE_DELAY_MS,
    max_delay: float = RETRY_MAX_DELAY_MS,
    factor: float = RETRY_BACKOFF_FACTOR,
    jitter: float = RETRY_JITTER_RATIO,
) -> float:
    """Compute the backoff delay for *attempt* (1-indexed).

    Formula::

        delay = min(base_delay * factor ** (attempt - 1), max_delay)
        delay *= (1 + random.uniform(-jitter, jitter))   # symmetric jitter

    Returns delay in **milliseconds** (float). Callers convert to seconds
    via ``delay_ms / 1000`` when passing to :func:`sleep_with_abort`.
    """
    delay = min(base_delay * factor ** (attempt - 1), max_delay)
    # Symmetric jitter: interval [delay*(1-jitter), delay*(1+jitter)]
    delay *= (1 + random.uniform(-jitter, jitter))
    return delay


async def sleep_with_abort(
    seconds: float,
    stop_event: asyncio.Event | None = None,
) -> bool:
    """Sleep for *seconds*, but abort early if *stop_event* is set.

    Returns ``True`` if the full wait completed normally, ``False`` if the
    sleep was aborted (either by *stop_event* being set or by
    ``asyncio.CancelledError``).
    """
    try:
        if stop_event is not None:
            # wait_for returns when stop_event.wait() completes (event set)
            # or raises TimeoutError when *seconds* elapses.
            await asyncio.wait_for(stop_event.wait(), timeout=seconds)
            return False  # stop_event was set → user cancelled
        else:
            await asyncio.sleep(seconds)
            return True
    except asyncio.TimeoutError:
        return True  # normal wait completed
    except asyncio.CancelledError:
        return False  # cancelled externally


async def retry_with_backoff(
    func: Callable[..., Any],
    *,
    max_retries: int = DEFAULT_MAX_RETRIES,
    on_retry: Callable[[int, int, float, ErrorClassification], Any] | None = None,
    stop_event: asyncio.Event | None = None,
    retryable_check: Callable[[Exception], bool] | None = None,
) -> Any:
    """Call *func* with exponential-backoff retry.

    Parameters
    ----------
    func
        Zero-argument async callable. The caller is responsible for closing
        over any arguments (messages, LLM instance, etc.).
    max_retries
        Maximum number of retries after the first attempt.
    on_retry
        Optional async callback invoked before each retry's backoff sleep::

            await on_retry(attempt, max_retries, delay_ms, classification)

        where *attempt* is the 1-indexed attempt number that just failed.
        The callback lets the caller (e.g. ``_stream_llm``) emit a
        ``retrying`` protocol message without coupling this module to the
        protocol layer.
    stop_event
        Optional ``asyncio.Event``. When set during backoff sleep, the sleep
        aborts immediately and ``RetryAbortedError`` is raised.
    retryable_check
        Optional extra predicate. After ``classify_error`` reports the error
        as retryable, this function is called with the exception; if it
        returns ``False`` the error is **not** retried (e.g. plan-02 uses
        this to suppress retries after the first token has streamed).

    Behaviour
    ---------
    * Non-retryable errors are re-raised immediately.
    * If ``retryable_check`` returns ``False`` for a retryable-classified
      error, it is re-raised immediately.
    * After ``max_retries`` retries are exhausted, the original exception is
      re-raised (the caller can catch it and use
      ``classification.error_type`` to emit an ``*_exhausted`` error
      message).
    * If *stop_event* is set during backoff sleep,
      :class:`RetryAbortedError` is raised.
    """
    for attempt in range(1, max_retries + 2):  # 1 .. max_retries+1
        try:
            return await func()
        except Exception as e:
            classification = classify_error(e)

            # Non-retryable → re-raise immediately
            if not classification.retryable:
                raise

            # Caller-supplied extra check (e.g. no retry after first token)
            if retryable_check is not None and not retryable_check(e):
                raise

            # Exhausted all retries → re-raise original exception
            if attempt > max_retries:
                raise

            # Compute delay: server-provided Retry-After takes precedence
            delay_ms = extract_retry_after_ms(e)
            if delay_ms is None:
                delay_ms = compute_backoff_delay(attempt)

            # Notify caller before sleeping
            if on_retry is not None:
                await on_retry(attempt, max_retries, delay_ms, classification)

            # Backoff sleep (abortable)
            completed = await sleep_with_abort(delay_ms / 1000, stop_event)
            if not completed:
                raise RetryAbortedError("Retry aborted by user")
