# -*- coding: utf-8 -*-
"""Extract //cn: comments from C++ source and map (context, source) -> translation.

Scans the same directories that lupdate scans (see CMakeLists.txt DA_LUPDATE_SOURCE_DIRS).
Patterns handled:
  - tr("English")  // cn:中文
  - tr("English", "disambiguation")  // cn:中文
  - QCoreApplication::translate("Ctx", "English")  // cn:中文
  - QApplication::translate("Ctx", "English")  // cn:中文
  - QTranslator::translate("Ctx", "English")  // cn:中文

Context resolution for tr():
  - The class context is the enclosing class/struct name. We also prepend
    namespace `DA::` when the file is under the DA namespace (heuristic: file
    starts with `namespace DA {` or has `DA_NAMESPACE_BEGIN`).
  - We key the map by (context, source). We ALSO keep a global map keyed by
    source text alone (for fallback), recording conflicts.
"""

import os
import re
import sys
import json
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Match lupdate scan dirs (CMakeLists DA_LUPDATE_SOURCE_DIRS)
SCAN_DIRS = [
    "src/APP",
    "src/DAAxOfficeWrapper",
    "src/DAData",
    "src/DAFigure",
    "src/DAGraphicsView",
    "src/DAGui",
    "src/DAInterface",
    "src/DAMessageHandler",
    "src/DAPluginSupport",
    "src/DAPyBindQt",
    "src/DAPyCommonWidgets",
    "src/DAPyScripts",
    "src/DAUtils",
    "src/DAPyWorkFlow",
    "src/DAShared",
    "plugins/DataAnalysis",
]

EXTS = (".cpp", ".h", ".hpp", ".cc", ".cxx", ".ui")

# Regex for a string literal possibly containing escaped quotes.
# Captures the inner content (with escapes still present).
STR = r'"((?:\\.|[^"\\])*)"'

# tr("...") optionally with disambiguation: tr("src"[, "dis"])
TR_RE = re.compile(r'\btr\s*\(\s*' + STR + r'(?:\s*,\s*' + STR + r')?\s*\)')
# translate("Ctx", "src"[, "dis"])
TRANSLATE_RE = re.compile(
    r'\b(?:QCoreApplication|QApplication|QTranslator)::translate\s*\(\s*'
    + STR + r'\s*,\s*' + STR + r'(?:\s*,\s*' + STR + r')?\s*\)'
)

# Comment after the call: // cn:xxx  or  //cn:xxx  or // CN:xxx
CN_COMMENT_RE = re.compile(r'//\s*[Cc][Nn]\s*:\s*(.*)$')

# Class/struct declaration: capture class name
CLASS_RE = re.compile(r'\b(?:class|struct)\s+([A-Za-z_]\w*)\b(?!\s*;)')
NAMESPACE_DA_RE = re.compile(r'\bnamespace\s+DA\b|DA_NAMESPACE_BEGIN')

# Unescape string literal content -> actual string value
def unescape(s):
    out = []
    i = 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            nxt = s[i+1]
            mapping = {'n':'\n','t':'\t','r':'\r','\\':'\\','"':'"','\'':"'",'0':'\0','a':'\a','b':'\b','f':'\f','v':'\v'}
            if nxt in mapping:
                out.append(mapping[nxt])
                i += 2
                continue
            # \xHH or \ooo - keep simple, just skip backslash
            out.append(nxt)
            i += 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def find_class_context(lines, line_idx):
    """Walk backwards to find the enclosing class name."""
    # Track brace depth to find the class we're inside
    depth = 0
    for i in range(line_idx, -1, -1):
        line = lines[i]
        # remove strings to avoid counting braces in strings
        cleaned = re.sub(STR, '""', line)
        # count closing braces we pass through
        for ch in cleaned:
            if ch == '}':
                depth += 1
            elif ch == '{':
                if depth == 0:
                    # this opening brace is the class body start (or function)
                    # search upward for class/struct
                    for j in range(i, -1, -1):
                        m = CLASS_RE.search(lines[j])
                        if m:
                            return m.group(1)
                    return None
                else:
                    depth -= 1
    return None


def find_namespace(lines, line_idx):
    """Find enclosing namespace by scanning upward for `namespace XX`."""
    depth = 0
    for i in range(line_idx, -1, -1):
        line = lines[i]
        cleaned = re.sub(STR, '""', line)
        for ch in cleaned:
            if ch == '}':
                depth += 1
            elif ch == '{':
                if depth == 0:
                    for j in range(i, -1, -1):
                        m = re.search(r'\bnamespace\s+([A-Za-z_]\w*)', lines[j])
                        if m:
                            return m.group(1)
                    return None
                else:
                    depth -= 1
    return None


def parse_first_string_after(text, pos):
    """From pos, skip whitespace, then parse one (possibly concatenated) string literal.
    Returns (string_value, end_pos) or (None, pos)."""
    n = len(text)
    i = pos
    while i < n and text[i] in ' \t\r\n':
        i += 1
    if i >= n or text[i] != '"':
        return None, i
    parts = []
    while i < n and text[i] == '"':
        # read one string literal
        j = i + 1
        buf = []
        while j < n:
            c = text[j]
            if c == '\\' and j + 1 < n:
                buf.append(text[j:j+2])
                j += 2
                continue
            if c == '"':
                break
            buf.append(c)
            j += 1
        parts.append(''.join(buf))
        i = j + 1  # skip closing quote
        # skip whitespace, allow adjacent string concat
        while i < n and text[i] in ' \t\r\n':
            i += 1
    return unescape(''.join(parts)) if parts else None, i


def find_statement_start(text, cn_pos):
    """Scan backwards from cn_pos to find the start of the enclosing statement.
    The statement boundary is a `;`, `{`, or `}` at brace depth 0 (ignoring
    strings and comments). The nearest such boundary before cn_pos is usually
    the END of the current statement (e.g. the `;` that ends `tr(...);`), so
    we collect ALL boundary positions in a forward window and return the
    one whose following code is non-empty/non-comment before cn_pos; if the
    last boundary leaves only whitespace+comments, we fall back to the previous
    boundary. Returns position (int)."""
    start = max(0, cn_pos - 6000)
    i = start
    in_str = False
    esc = False
    in_line_comment = False
    in_block_comment = False
    boundaries = [start]  # list of positions = char-after-boundary
    while i < cn_pos:
        c = text[i]
        nxt = text[i+1] if i+1 < cn_pos else ''
        if in_line_comment:
            if c == '\n':
                in_line_comment = False
            i += 1
            continue
        if in_block_comment:
            if c == '*' and nxt == '/':
                in_block_comment = False
                i += 2
                continue
            i += 1
            continue
        if in_str:
            if esc:
                esc = False
            elif c == '\\':
                esc = True
            elif c == '"':
                in_str = False
            i += 1
            continue
        if c == '/' and nxt == '/':
            in_line_comment = True
            i += 2
            continue
        if c == '/' and nxt == '*':
            in_block_comment = True
            i += 2
            continue
        if c == '"':
            in_str = True
            i += 1
            continue
        if c == ';' or c == '{' or c == '}':
            boundaries.append(i + 1)
        i += 1
    # Pick the boundary: walk from the latest backwards; choose the first whose
    # code-to-cn region contains a non-whitespace, non-comment char.
    for b in reversed(boundaries):
        seg = text[b:cn_pos]
        if _has_code(seg):
            return b
    return boundaries[0]


def _has_code(seg):
    """True if seg contains any non-whitespace char that is not part of a comment."""
    i = 0
    n = len(seg)
    while i < n:
        c = seg[i]
        if c in ' \t\r\n':
            i += 1
            continue
        if c == '/' and i + 1 < n and seg[i+1] == '/':
            # rest is line comment
            return False  # but maybe code before it; we already passed leading ws
        if c == '/' and i + 1 < n and seg[i+1] == '*':
            j = i + 2
            while j < n - 1 and not (seg[j] == '*' and seg[j+1] == '/'):
                j += 1
            i = j + 2
            continue
        return True
    return False


def find_enclosing_tr(text, cn_pos, lines, cn_line_idx):
    """Find the LAST tr( or ::translate( call before cn_pos within the enclosing
    statement (i.e., the one that the cn comment actually annotates). Returns
    (kind, open_paren_pos, ctx_str_or_None, call_line_idx) or None."""
    stmt_start = find_statement_start(text, cn_pos)
    stmt = text[stmt_start:cn_pos]

    # collect all candidate positions
    cands = []
    for m in re.finditer(r'\btr\s*\(', stmt):
        cands.append(('tr', m.start()))
    for m in re.finditer(r'::translate\s*\(', stmt):
        cands.append(('translate', m.start()))
    if not cands:
        return None
    # pick the latest one (closest to cn_pos)
    cands.sort(key=lambda x: x[1])
    kind, pos_in_stmt = cands[-1]
    open_paren_pos = stmt_start + pos_in_stmt + (stmt[pos_in_stmt:].index('('))
    line_idx = text[:open_paren_pos].count('\n')
    ctx_str = None
    if kind == 'translate':
        ctx_str, _ = parse_first_string_after(text, open_paren_pos + 1)
    return (kind, open_paren_pos, ctx_str, line_idx)


def extract_file(path):
    """Return list of (context, source, translation) tuples found in file."""
    results = []
    try:
        with open(path, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
    except Exception:
        return results
    lines = content.split('\n')

    file_in_da_ns = bool(NAMESPACE_DA_RE.search(content))

    for idx, line in enumerate(lines):
        low = line.lower()
        if 'cn:' not in low and '//cn' not in low:
            continue
        cn_match = CN_COMMENT_RE.search(line)
        if not cn_match:
            continue
        cn_text = cn_match.group(1).strip().rstrip()

        # Position in content where the // begins
        # compute offset of this line
        line_start = 0
        for i in range(idx):
            line_start += len(lines[i]) + 1  # +1 for \n
        cn_pos = line_start + cn_match.start()

        found = find_enclosing_tr(content, cn_pos, lines, idx)
        if not found:
            continue
        kind, open_paren_pos, ctx_str, call_line_idx = found

        if kind == 'translate':
            # context already captured; parse source after context string
            # find end of context string, then comma, then source
            # parse ctx string
            ctx_val, after_ctx = parse_first_string_after(content, open_paren_pos + 1)
            if ctx_val is None:
                continue
            # skip to comma
            i = after_ctx
            while i < len(content) and content[i] in ' \t\r\n':
                i += 1
            if i >= len(content) or content[i] != ',':
                continue
            i += 1
            src_val, _ = parse_first_string_after(content, i)
            if src_val is None:
                continue
            results.append((ctx_val, src_val, cn_text))
        else:
            # tr()
            src_val, _ = parse_first_string_after(content, open_paren_pos + 1)
            if src_val is None:
                continue
            cls = find_class_context(lines, call_line_idx)
            ns = find_namespace(lines, call_line_idx)
            if cls:
                if ns and ns != 'DA' and file_in_da_ns is False:
                    ctx = f"{ns}::{cls}"
                elif ns == 'DA' or (file_in_da_ns and ns is None):
                    ctx = f"DA::{cls}"
                else:
                    ctx = cls
            else:
                ctx = None
            results.append((ctx, src_val, cn_text))
    return results


def main():
    by_ctx_src = {}      # (ctx, src) -> translation
    by_src = {}          # src -> translation (fallback)
    src_conflicts = defaultdict(set)
    src_to_ctxs = defaultdict(set)

    for d in SCAN_DIRS:
        full = os.path.join(ROOT, d.replace('/', os.sep))
        if not os.path.isdir(full):
            continue
        for dirpath, _, files in os.walk(full):
            # skip 3rdparty / build artifacts
            if '3rdparty' in dirpath.split(os.sep):
                continue
            if 'build' in dirpath.split(os.sep):
                continue
            for fn in files:
                if not fn.endswith(EXTS):
                    continue
                fp = os.path.join(dirpath, fn)
                for ctx, src, cn in extract_file(fp):
                    if ctx:
                        key = (ctx, src)
                        if key in by_ctx_src and by_ctx_src[key] != cn:
                            pass  # keep first
                        by_ctx_src.setdefault(key, cn)
                    # global fallback
                    if src in by_src and by_src[src] != cn:
                        src_conflicts[src].add(by_src[src])
                        src_conflicts[src].add(cn)
                    else:
                        by_src.setdefault(src, cn)
                    src_to_ctxs[src].add(ctx)

    # Build output
    out = {
        "by_ctx_src": {f"{k[0]}|||{k[1]}": v for k, v in by_ctx_src.items()},
        "by_src": by_src,
        "src_conflicts": {k: list(v) for k, v in src_conflicts.items()},
    }
    print(f"by_ctx_src entries: {len(by_ctx_src)}")
    print(f"by_src entries: {len(by_src)}")
    print(f"conflicts: {len(src_conflicts)}")
    outpath = os.path.join(ROOT, 'scripts', '_cn_map.json')
    with open(outpath, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    print(f"wrote {outpath}")


if __name__ == '__main__':
    main()
