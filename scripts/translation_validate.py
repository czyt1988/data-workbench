# -*- coding: utf-8 -*-
"""Shared validation helpers for the translation scripts.

Provides placeholder/format-specifier parity checks so that a translation
cannot silently drop ``%1``/``%n``/``(*.%1)`` patterns that exist in the
source string — the class of bug that broke the ``.dapro`` file dialog
filter (a Chinese translation dropped ``(*.%1)``).
"""

import re

# Qt format specifiers: %1, %2, ..., %9, %L1, %n (plural), %% (literal percent)
# We treat %% as a literal and exclude it from the placeholder set.
_PLACEHOLDER_RE = re.compile(r'%(?:L?\d+|n)(?!\d)')

# File-filter glob patterns that may appear inside tr(): (*.ext), (*.png *.jpg)
# These are NOT Qt format specifiers but are equally dangerous if dropped.
_GLOB_RE = re.compile(r'\(\*\.[^)]*\)')


def extract_placeholders(text):
    """Return the sorted list of Qt placeholder tokens (%1, %n, %L2, ...) in text."""
    found = _PLACEHOLDER_RE.findall(text or '')
    return sorted(found)


def extract_globs(text):
    """Return the list of file-filter glob patterns (*.ext) in text."""
    return _GLOB_RE.findall(text or '')


def check_placeholder_parity(source, translation, source_label=''):
    """Validate that *translation* preserves the placeholders and glob patterns
    present in *source*.

    Returns a list of human-readable warning strings (empty list = OK).

    Checks performed:
      1. Every ``%N``/``%n``/``%LN`` in source must appear in translation
         (same multiset). Extra placeholders in translation are also flagged.
      2. Every file-filter glob ``(*.ext)`` in source must appear in translation.

    The glob check catches the exact bug class where a translator wrote just
    the label (e.g. "工程文件") and dropped the "(*.%1)" glob from
    "Project File (*.%1)".
    """
    warnings = []
    if source is None:
        return warnings
    src = source or ''
    trans = translation or ''

    src_ph = extract_placeholders(src)
    trans_ph = extract_placeholders(trans)
    if src_ph != trans_ph:
        missing = sorted(set(src_ph) - set(trans_ph))
        extra = sorted(set(trans_ph) - set(src_ph))
        parts = []
        if missing:
            parts.append(f"translation missing placeholders {missing}")
        if extra:
            parts.append(f"translation has extra placeholders {extra}")
        prefix = f"[{source_label}] " if source_label else ''
        warnings.append(f"{prefix}placeholder mismatch: {'; '.join(parts)}")

    src_globs = extract_globs(src)
    trans_globs = extract_globs(trans)
    if src_globs and src_globs != trans_globs:
        missing_globs = [g for g in src_globs if g not in trans_globs]
        if missing_globs:
            prefix = f"[{source_label}] " if source_label else ''
            warnings.append(f"{prefix}translation dropped glob pattern(s) {missing_globs} "
                            f"present in source")

    return warnings
