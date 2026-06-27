#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动填充 PO 文件翻译脚本
根据 Python 代码中的 # cn: 注释自动更新 PO 文件中的翻译

安装依赖: pip install polib

运行示例:
    python update_po.py ./locale/zh_CN/LC_MESSAGES/DASystemNodes.po --py-dir ../
"""

import re
import os
import argparse
import polib  # 需要安装: pip install polib
from typing import Dict, List, Optional


def extract_translations_from_py(py_file_path: str) -> Dict[str, str]:
    """
    从 Python 文件中提取翻译字符串和对应的中文翻译

    Args:
        py_file_path: Python 文件路径

    Returns:
        字典: {msgid: translation}
    """
    translations = {}

    try:
        with open(py_file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # 匹配格式: _("字符串")  # cn: 翻译
        pattern = r'_\([\'"]([^\n]*?)[\'"]\)\s*(?:\#\s*cn:\s*(.*?))?(?=\n|$)'

        lines = content.split('\n')
        for line_num, line in enumerate(lines, 1):
            if line.count('_("') > 1 or line.count("_('") > 1:
                continue

            matches = re.findall(pattern, line)
            for match in matches:
                msgid = match[0].strip()
                translation = match[1].strip() if match[1] else None

                if translation:
                    msgid = msgid.replace('\\"', '"').replace("\\'", "'")
                    translations[msgid] = translation

    except Exception as e:
        print(f"读取 Python 文件 {py_file_path} 时出错: {e}")

    return translations


def find_all_py_files_with_translations(root_dir: str) -> Dict[str, Dict[str, str]]:
    """
    递归查找目录下所有包含翻译的 Python 文件

    Args:
        root_dir: 根目录路径

    Returns:
        字典: {py_file_path: {msgid: translation}}
    """
    all_translations = {}

    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith('.py'):
                py_file_path = os.path.join(root, file)
                translations = extract_translations_from_py(py_file_path)
                if translations:
                    all_translations[py_file_path] = translations

    return all_translations


def update_po_with_translations(po_file_path: str, translations_dict: Dict[str, str]) -> int:
    """
    使用提取的翻译更新 PO 文件

    Args:
        po_file_path: PO 文件路径
        translations_dict: 翻译字典 {msgid: translation}

    Returns:
        更新的条目数量
    """
    updated_count = 0

    try:
        po = polib.pofile(po_file_path)

        for entry in po:
            msgid = entry.msgid
            if not entry.msgstr and msgid in translations_dict:
                entry.msgstr = translations_dict[msgid]
                updated_count += 1
                print(f"更新: {msgid[:50]}... -> {translations_dict[msgid][:50]}...")

        if updated_count > 0:
            po.save(po_file_path)
            print(f"\n已更新 {updated_count} 个翻译条目")

        return updated_count

    except Exception as e:
        print(f"更新 PO 文件 {po_file_path} 时出错: {e}")
        return 0


def main():
    parser = argparse.ArgumentParser(description='自动填充 PO 文件翻译')
    parser.add_argument('po_file', help='PO 文件路径')
    parser.add_argument('--py-dir', help='Python 文件目录（可选，默认从 PO 文件中提取）')
    parser.add_argument('--py-file', help='单个 Python 文件路径（可选）')
    parser.add_argument('--dry-run', action='store_true', help='只显示将要更新的内容，不实际修改')
    parser.add_argument('--verbose', '-v', action='store_true', help='显示详细信息')

    args = parser.parse_args()

    if not os.path.exists(args.po_file):
        print(f"错误: PO 文件不存在: {args.po_file}")
        return

    all_translations = {}

    if args.py_file:
        if os.path.exists(args.py_file):
            translations = extract_translations_from_py(args.py_file)
            if translations:
                all_translations = {args.py_file: translations}
        else:
            print(f"错误: Python 文件不存在: {args.py_file}")
            return

    elif args.py_dir:
        if os.path.exists(args.py_dir):
            all_translations = find_all_py_files_with_translations(args.py_dir)
        else:
            print(f"错误: 目录不存在: {args.py_dir}")
            return

    if not all_translations:
        print("没有找到任何翻译注释")
        return

    combined_translations = {}
    for py_file, translations in all_translations.items():
        if args.verbose:
            print(f"从 {py_file} 中找到 {len(translations)} 个翻译注释")
        combined_translations.update(translations)

    if args.dry_run:
        print(f"\n[干运行模式] 将更新 {len(combined_translations)} 个翻译")
        return

    print(f"\n开始更新 PO 文件: {args.po_file}")
    updated_count = update_po_with_translations(args.po_file, combined_translations)

    if updated_count == 0:
        print("没有需要更新的翻译条目")


if __name__ == '__main__':
    main()
