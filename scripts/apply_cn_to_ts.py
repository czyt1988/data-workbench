# -*- coding: utf-8 -*-
"""Apply extracted cn: mappings to da_zh_CN.ts.

Reads scripts/_cn_map.json (produced by extract_cn_comments.py) and rewrites
src/i18n/da_zh_CN.ts: for each <message> whose <translation> is unfinished
(type="unfinished") or empty, fill it with a translation looked up by
(context, source), falling back to source-only. Marks them finished
(removes type="unfinished").

Reports how many were filled and how many remain unfilled (with their source
text) so they can be translated manually.
"""

import os
import re
import json
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TS = os.path.join(ROOT, 'src', 'i18n', 'da_zh_CN.ts')
MAP = os.path.join(ROOT, 'scripts', '_cn_map.json')


def xml_escape(s):
    return (s.replace('&', '&amp;')
             .replace('<', '&lt;')
             .replace('>', '&gt;'))


def main():
    with open(MAP, encoding='utf-8') as f:
        data = json.load(f)
    by_ctx_src = {}
    for k, v in data['by_ctx_src'].items():
        ctx, src = k.split('|||', 1)
        by_ctx_src[(ctx, src)] = v
    by_src = data['by_src']

    with open(TS, encoding='utf-8') as f:
        text = f.read()

    # Parse message blocks. A message block:
    #   <message>
    #     <location .../>
    #     <source>SRC</source>
    #     [<comment>...</comment>]
    #     <translation [type="unfinished"]>TRANS</translation>
    #   </message>
    # We capture the context name from the enclosing <context><name>...
    # Simpler: iterate contexts.

    # Regex for a whole context block
    ctx_re = re.compile(
        r'<context>\s*<name>(.*?)</name>(.*?)</context>',
        re.DOTALL,
    )
    msg_re = re.compile(
        r'(<message>)(.*?)(</message>)',
        re.DOTALL,
    )
    src_re = re.compile(r'<source>(.*?)</source>', re.DOTALL)
    trans_re = re.compile(
        r'<translation(\s+type="([^"]*)")?\s*>(.*?)</translation>',
        re.DOTALL,
    )

    filled = 0
    skipped_already = 0
    not_found = []
    total_msgs = 0

    def replace_context(m):
        nonlocal filled, skipped_already, total_msgs
        ctx_name = m.group(1)
        body = m.group(2)

        def replace_message(mm):
            nonlocal filled, skipped_already, total_msgs
            open_tag, inner, close_tag = mm.group(1), mm.group(2), mm.group(3)
            total_msgs += 1
            sm = src_re.search(inner)
            if not sm:
                return open_tag + inner + close_tag
            src = sm.group(1)
            # decode XML entities in source (we re-encode when emitting)
            src_decoded = (src.replace('&amp;', '&')
                              .replace('&lt;', '<')
                              .replace('&gt;', '>')
                              .replace('&quot;', '"')
                              .replace('&apos;', "'"))
            tm = trans_re.search(inner)
            if not tm:
                return open_tag + inner + close_tag
            ttype = tm.group(2)  # e.g. "unfinished" or None
            tcontent = tm.group(3)
            # If already has a real translation (non-empty, not unfinished), keep.
            if ttype != 'unfinished' and tcontent.strip():
                skipped_already += 1
                return open_tag + inner + close_tag

            # Look up
            trans = by_ctx_src.get((ctx_name, src_decoded))
            if trans is None:
                trans = by_src.get(src_decoded)
            if trans is None:
                not_found.append((ctx_name, src_decoded))
                # leave as-is
                return open_tag + inner + close_tag
            filled += 1
            new_trans_tag = f'<translation>{xml_escape(trans)}</translation>'
            new_inner = trans_re.sub(new_trans_tag, inner)
            return open_tag + new_inner + close_tag

        new_body = msg_re.sub(replace_message, body)
        return f'<context>\n    <name>{ctx_name}</name>{new_body}</context>'

    new_text = ctx_re.sub(replace_context, text)

    # preserve leading <?xml and DOCTYPE
    with open(TS, 'w', encoding='utf-8', newline='') as f:
        f.write(new_text)

    print(f'Total messages: {total_msgs}')
    print(f'Already translated (skipped): {skipped_already}')
    print(f'Filled from cn: comments: {filled}')
    print(f'Remaining unfinished (need manual): {len(not_found)}')
    # write remaining list for manual translation
    rem_path = os.path.join(ROOT, 'scripts', '_remaining_unfinished.txt')
    with open(rem_path, 'w', encoding='utf-8') as f:
        seen = set()
        for ctx, src in not_found:
            key = (ctx, src)
            if key in seen:
                continue
            seen.add(key)
            f.write(f'[{ctx}] {src}\n')
    print(f'Remaining list -> {rem_path}')


if __name__ == '__main__':
    main()
