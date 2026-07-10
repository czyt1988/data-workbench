#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Validate .ts translation files for placeholder/glob parity and corruption.

Exit code 0 = all checks passed; 1 = issues found.

Checks performed on each finished (non-unfinished) <message>:
  1. Placeholder parity: every %N/%n/%LN in <source> must appear in
     <translation> in the same multiset, and vice versa.
  2. Glob parity: every (*.ext) file-filter glob in <source> must appear in
     <translation>.
  3. cn: marker leak: <translation> must not contain the literal text "cn:"
     (indicates a // cn: comment was mistakenly written into the .ts).

Usage:
    python scripts/check_translations.py [path/to/file.ts ...]

If no files are given, defaults to all *.ts under src/i18n/.
"""
import os
import re
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_DIR = os.path.join(ROOT, 'src', 'i18n')

sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from translation_validate import check_placeholder_parity


def parse_ts_messages(ts_path):
    """Yield (context, source, translation, translation_type, line_number) tuples."""
    try:
        tree = ET.parse(ts_path)
    except ET.ParseError as e:
        print(f"ERROR: XML parse error in {ts_path}: {e}")
        return
    root = tree.getroot()
    for ctx_el in root.findall('context'):
        ctx_name_el = ctx_el.find('name')
        ctx_name = ctx_name_el.text if ctx_name_el is not None and ctx_name_el.text else ''
        for msg_el in ctx_el.findall('message'):
            src_el = msg_el.find('source')
            trans_el = msg_el.find('translation')
            if src_el is None or trans_el is None:
                continue
            source = ''.join(src_el.itertext())
            # For simple translations, text is in .text; for plurals, use itertext
            translation = ''.join(trans_el.itertext()) if list(trans_el) else (trans_el.text or '')
            ttype = trans_el.get('type', '')
            yield (ctx_name, source, translation, ttype)


def check_file(ts_path):
    issues = []
    for ctx, source, translation, ttype in parse_ts_messages(ts_path):
        # Skip unfinished/vanished/obsolete — they fall back to source at runtime
        if ttype in ('unfinished', 'vanished', 'obsolete'):
            continue
        # Check 1 & 2: placeholder/glob parity
        warns = check_placeholder_parity(source, translation, f'{ctx}: {source[:60]}')
        issues.extend(warns)
        # Check 3: cn: marker leak
        if 'cn:' in (translation or ''):
            issues.append(
                f"[{ctx}: {source[:60]}] translation contains 'cn:' marker leak: "
                f"{translation[:80]!r}"
            )
    return issues


def main():
    args = sys.argv[1:]
    if not args:
        # Default: all .ts files under src/i18n/
        args = []
        for fn in sorted(os.listdir(DEFAULT_DIR)):
            if fn.endswith('.ts') and not fn.startswith('backup'):
                args.append(os.path.join(DEFAULT_DIR, fn))
    if not args:
        print("No .ts files found to check.")
        return 0

    total_issues = 0
    for ts_path in args:
        rel = os.path.relpath(ts_path, ROOT)
        issues = check_file(ts_path)
        if issues:
            print(f"\n{'='*60}")
            print(f"ISSUES in {rel} ({len(issues)}):")
            print(f"{'='*60}")
            for issue in issues:
                print(f"  {issue}")
            total_issues += len(issues)
        else:
            print(f"OK: {rel} — no issues")

    print(f"\n{'='*60}")
    if total_issues:
        print(f"FAILED: {total_issues} translation issue(s) found across {len(args)} file(s).")
        return 1
    else:
        print(f"PASSED: all {len(args)} file(s) clean.")
        return 0


if __name__ == '__main__':
    sys.exit(main())
