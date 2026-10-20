# -*- coding: utf-8 -*-
"""语义级 ts 合并：把主工作树在途的翻译润色重放到工作树 lupdate 新版 ts 上。

用法：python ts_semantic_merge.py <old_ts> <polished_ts> <mine_ts>
- old_ts:     git show dev:src/i18n/da_zh_CN.ts（润色前基线）
- polished_ts: 主工作树工作区 da_zh_CN.ts（润色后）
- mine_ts:    任务工作树 da_zh_CN.ts（lupdate 重写的新结构，输出也写它）
原理：逐行状态机解析三个 ts，按 (context, source 实体解码后) 提取润色产生的
translation 变更，再在 mine 中按同键匹配并整行替换 translation 行。
"""
import io
import re
import sys


def xml_decode(s):
    return (s.replace('&quot;', '"').replace('&apos;', "'")
             .replace('&lt;', '<').replace('&gt;', '>').replace('&amp;', '&'))


def parse_ts(path):
    """返回 dict[(context, source_decoded)] = translation 行原文（含缩进）。"""
    ctx = None
    source = None
    out = {}
    with io.open(path, 'r', encoding='utf-8') as f:
        for line in f:
            t = line.strip()
            if t.startswith('<name>'):
                ctx = t[6:t.find('</name>')]
            elif t.startswith('<source>'):
                source = xml_decode(t[7:t.rfind('</source>')])
            elif t.startswith('<translation'):
                if ctx is not None and source is not None:
                    out[(ctx, source)] = line.rstrip('\r\n')
                source = None
    return out


def main():
    old_ts, polished_ts, mine_ts = sys.argv[1:4]
    old_map = parse_ts(old_ts)
    new_map = parse_ts(polished_ts)

    changes = {}
    for k, v in new_map.items():
        if old_map.get(k) != v:
            changes[k] = v  # 新 translation 行原文（已按 polished 的转义/格式）
    if not changes:
        print('no changes detected')
        return

    applied = 0
    missing = []
    out_lines = []
    ctx = None
    source = None
    with io.open(mine_ts, 'r', encoding='utf-8') as f:
        for line in f:
            t = line.rstrip('\r\n').strip()
            if t.startswith('<name>'):
                ctx = t[6:t.find('</name>')]
            elif t.startswith('<source>'):
                source = xml_decode(t[7:t.rfind('</source>')])
            elif t.startswith('<translation'):
                if ctx is not None and source is not None:
                    repl = changes.get((ctx, source))
                    if repl is not None:
                        indent = line[:len(line) - len(line.lstrip())]
                        rstrip = repl.lstrip()
                        out_lines.append(indent + rstrip + '\n')
                        applied += 1
                        source = None
                        continue
                source = None
            out_lines.append(line)
    with io.open(mine_ts, 'w', encoding='utf-8', newline='') as f:
        f.writelines(out_lines)

    hit_keys = set()
    # 再扫一遍 mine 统计未命中的变更
    ctx = None
    source = None
    with io.open(mine_ts, 'r', encoding='utf-8') as f:
        for line in f:
            t = line.strip()
            if t.startswith('<name>'):
                ctx = t[6:t.find('</name>')]
            elif t.startswith('<source>'):
                source = xml_decode(t[7:t.rfind('</source>')])
            elif t.startswith('<translation'):
                if (ctx, source) in changes:
                    hit_keys.add((ctx, source))
                source = None
    missing = [k for k in changes if k not in hit_keys]
    print('changes=%d applied=%d missing=%d' % (len(changes), applied, len(missing)))
    for k in missing[:40]:
        print('  MISSING:', k[0], '|', k[1][:80])


if __name__ == '__main__':
    main()
