#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将 backup-da_zh_CN.ts 中已有的翻译合并到 da_zh_CN.ts 中。

使用方法:
    python merge_translations.py

匹配规则: context name + source text
- 仅当目标文件中的翻译为空或标记为 unfinished 时才合并
- backup 中标记为 vanished/obsolete 的条目会被跳过
- 不会覆盖目标文件中已有的翻译
"""

import xml.etree.ElementTree as ET
import os
import sys
import re


def parse_ts(filepath):
    """解析 .ts 文件，返回 (tree, translations_dict)
    translations_dict: {(context_name, source_text): translation_element}
    """
    tree = ET.parse(filepath)
    root = tree.getroot()
    trans = {}
    for context in root.findall('context'):
        ctx_name = context.find('name').text or ''
        for message in context.findall('message'):
            source_el = message.find('source')
            if source_el is None:
                continue
            # source 可能包含子元素或尾部文本，用 itertext 获取完整文本
            source_text = ''.join(source_el.itertext()) if source_el.text is None and len(source_el) > 0 else (source_el.text or '')
            translation_el = message.find('translation')
            if translation_el is not None:
                trans[(ctx_name, source_text)] = translation_el
    return tree, trans


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    backup_path = os.path.join(script_dir, 'backup-da_zh_CN.ts')
    current_path = os.path.join(script_dir, 'da_zh_CN.ts')

    if not os.path.exists(backup_path):
        print(f"错误: 备份文件不存在: {backup_path}")
        sys.exit(1)
    if not os.path.exists(current_path):
        print(f"错误: 目标文件不存在: {current_path}")
        sys.exit(1)

    # 解析两个文件
    _, backup_trans = parse_ts(backup_path)
    current_tree, current_trans = parse_ts(current_path)

    merged_count = 0
    already_translated = 0
    not_found = 0

    # 遍历 backup 中的翻译
    for (ctx_name, source_text), backup_el in backup_trans.items():
        backup_text = backup_el.text
        backup_type = backup_el.get('type', '')

        # 跳过 backup 中没有实际翻译的条目
        if not backup_text or backup_text.strip() == '':
            continue
        # 跳过标记为 vanished/obsolete 的
        if backup_type in ('vanished', 'obsolete'):
            continue

        # 在当前文件中查找匹配
        if (ctx_name, source_text) in current_trans:
            cur_el = current_trans[(ctx_name, source_text)]
            cur_text = cur_el.text
            cur_type = cur_el.get('type', '')

            # 只在当前翻译为空或 unfinished 时合并
            if not cur_text or cur_text.strip() == '' or cur_type == 'unfinished':
                cur_el.text = backup_text
                # 如果 backup 中不是 unfinished，移除 type 属性
                if backup_type != 'unfinished':
                    if 'type' in cur_el.attrib:
                        del cur_el.attrib['type']
                merged_count += 1
            else:
                already_translated += 1
        else:
            not_found += 1

    # 写回当前文件
    current_tree.write(current_path, encoding='utf-8', xml_declaration=True)

    # ElementTree 不保留 DOCTYPE，手动补回
    with open(current_path, 'r', encoding='utf-8') as f:
        content = f.read()
    if '<!DOCTYPE TS>' not in content:
        content = content.replace('?>\n<TS', '?>\n<!DOCTYPE TS>\n<TS')
    with open(current_path, 'w', encoding='utf-8') as f:
        f.write(content)

    # 统计结果
    total_backup = len(backup_trans)
    total_current = len(current_trans)
    remaining = sum(
        1 for el in current_trans.values()
        if el.get('type') == 'unfinished' or (not el.text or el.text.strip() == '')
    )

    print(f"合并完成:")
    print(f"  - 本次成功合并翻译: {merged_count}")
    print(f"  - 已有翻译(跳过):   {already_translated}")
    print(f"  - backup独有条目:    {not_found}")
    print(f"  - backup总条目数:    {total_backup}")
    print(f"  - 目标文件总条目数:  {total_current}")
    print(f"  - 剩余未翻译条目:   {remaining}")


if __name__ == '__main__':
    main()
