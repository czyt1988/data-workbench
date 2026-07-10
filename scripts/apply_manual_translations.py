# -*- coding: utf-8 -*-
"""Apply the manual translation table to da_zh_CN.ts.

Reads scripts/_manual_translations.py (TRANSLATIONS dict keyed by (ctx, src)
where src is the DECODED plain-text source). For each unfinished/empty
<translation>, decode the <source> XML entities, look up the translation,
re-encode to XML, and replace the <translation> tag (removing
type="unfinished").
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TS = os.path.join(ROOT, 'src', 'i18n', 'da_zh_CN.ts')

sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from _manual_translations import TRANSLATIONS
from translation_validate import check_placeholder_parity


def xml_decode(s):
    return (s.replace('&amp;', '&')
             .replace('&lt;', '<')
             .replace('&gt;', '>')
             .replace('&quot;', '"')
             .replace('&apos;', "'"))


def xml_encode(s):
    """Encode text for safe inclusion as element character data.

    Preserves existing XML/HTML entities (e.g. &gt;, &lt;, &quot;, &amp;,
    &apos; and numeric &#NN;), so rich-text / markdown translations that
    already contain entities are written through unchanged. Escapes only
    bare '&', '<', '>' characters that are NOT part of an entity reference.
    """
    out = []
    i = 0
    n = len(s)
    entity_re = re.compile(r'&(?:[a-zA-Z][a-zA-Z0-9]*|#[0-9]+|#[xX][0-9a-fA-F]+);')
    while i < n:
        c = s[i]
        if c == '&':
            m = entity_re.match(s, i)
            if m:
                out.append(m.group(0))
                i = m.end()
            else:
                out.append('&amp;')
                i += 1
        elif c == '<':
            out.append('&lt;')
            i += 1
        elif c == '>':
            out.append('&gt;')
            i += 1
        else:
            out.append(c)
            i += 1
    return ''.join(out)


# Normalize dictionary keys: decode any XML entities written in the source side
# of the key, so matching works against decoded source text from the .ts file.
TRANSLATIONS = {(ctx, xml_decode(src)): trans for (ctx, src), trans in TRANSLATIONS.items()}


def main():
    with open(TS, encoding='utf-8') as f:
        text = f.read()

    ctx_re = re.compile(r'<context>\s*<name>(.*?)</name>(.*?)</context>', re.DOTALL)
    msg_re = re.compile(r'(<message>)(.*?)(</message>)', re.DOTALL)
    src_re = re.compile(r'<source>(.*?)</source>', re.DOTALL)
    trans_re = re.compile(
        r'<translation(\s+type="([^"]*)")?\s*>(.*?)</translation>',
        re.DOTALL,
    )

    filled = 0
    skipped_mismatch = 0
    mismatches = []
    not_found = []
    not_found_keys = set()

    def replace_context(m):
        nonlocal filled, skipped_mismatch
        ctx = m.group(1)
        body = m.group(2)

        def replace_message(mm):
            nonlocal filled, skipped_mismatch
            open_tag, inner, close_tag = mm.group(1), mm.group(2), mm.group(3)
            sm = src_re.search(inner)
            if not sm:
                return open_tag + inner + close_tag
            src_encoded = sm.group(1)
            src_decoded = xml_decode(src_encoded)
            tm = trans_re.search(inner)
            if not tm:
                return open_tag + inner + close_tag
            ttype = tm.group(2)
            tcontent = tm.group(3)
            # Only fill if unfinished or empty
            if ttype != 'unfinished' and tcontent.strip():
                return open_tag + inner + close_tag
            key = (ctx, src_decoded)
            if key not in TRANSLATIONS:
                if key not in not_found_keys:
                    not_found_keys.add(key)
                    not_found.append(key)
                return open_tag + inner + close_tag
            trans = TRANSLATIONS[key]
            # Validate placeholder/glob parity before writing
            warns = check_placeholder_parity(src_decoded, trans, f'{ctx}: {src_decoded[:60]}')
            if warns:
                skipped_mismatch += 1
                mismatches.extend(warns)
                if key not in not_found_keys:
                    not_found_keys.add(key)
                    not_found.append(key)
                return open_tag + inner + close_tag
            new_tag = f'<translation>{xml_encode(trans)}</translation>'
            new_inner = trans_re.sub(lambda _: new_tag, inner)
            filled += 1
            return open_tag + new_inner + close_tag

        new_body = msg_re.sub(replace_message, body)
        return f'<context>\n    <name>{ctx}</name>{new_body}</context>'

    new_text = ctx_re.sub(replace_context, text)
    with open(TS, 'w', encoding='utf-8', newline='') as f:
        f.write(new_text)

    print(f'Filled from manual table: {filled}')
    print(f'Skipped (placeholder/glob mismatch): {skipped_mismatch}')
    print(f'Still unfilled: {len(not_found)}')
    if mismatches:
        print('--- Placeholder/glob mismatches (translations NOT written) ---')
        for w in mismatches:
            print(f'  {w}')
    for k in not_found:
        print(f'  MISS [{k[0]}] {k[1][:80]!r}')


if __name__ == '__main__':
    main()
