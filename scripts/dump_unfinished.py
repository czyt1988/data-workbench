# -*- coding: utf-8 -*-
"""Dump all unfinished <message> entries with exact context + source text,
so we can build a precise translation table. Writes scripts/_unfinished_dump.txt
in a pipe-delimited format: CTX ||| SRC (with a marker line)."""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TS = os.path.join(ROOT, 'src', 'i18n', 'da_zh_CN.ts')

with open(TS, encoding='utf-8') as f:
    text = f.read()

ctx_re = re.compile(r'<context>\s*<name>(.*?)</name>(.*?)</context>', re.DOTALL)
msg_re = re.compile(r'<message>(.*?)</message>', re.DOTALL)
src_re = re.compile(r'<source>(.*?)</source>', re.DOTALL)
trans_re = re.compile(r'<translation(\s+type="([^"]*)")?\s*>(.*?)</translation>', re.DOTALL)

out_lines = []
for cm in ctx_re.finditer(text):
    ctx = cm.group(1)
    body = cm.group(2)
    for mm in msg_re.finditer(body):
        inner = mm.group(1)
        sm = src_re.search(inner)
        if not sm:
            continue
        src = sm.group(1)
        tm = trans_re.search(inner)
        if not tm:
            continue
        ttype = tm.group(2)
        tcontent = tm.group(3)
        if ttype == 'unfinished' or not tcontent.strip():
            out_lines.append(f'{ctx} ||| {src}')

outpath = os.path.join(ROOT, 'scripts', '_unfinished_dump.txt')
with open(outpath, 'w', encoding='utf-8') as f:
    f.write('\n'.join(out_lines))
print(f'wrote {len(out_lines)} entries -> {outpath}')
