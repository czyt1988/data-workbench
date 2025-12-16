#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动填充PO文件翻译脚本
根据Python代码中的# cn:注释自动更新PO文件中的翻译
# 安装依赖
pip install polib

# 运行脚本
python update_po.py your_translation.po --py-dir /path/to/python/files

# 或者指定单个Python文件
python update_po.py your_translation.po --py-file /path/to/file.py

# 干运行模式（只显示不修改）
python update_po.py your_translation.po --py-dir /path/to/python/files --dry-run

例如，本项目你可以这样执行：
python update_po.py ./locale/zh_CN/LC_MESSAGES/DADataAnalysis.po --py-dir ../
"""

import re
import os
import argparse
import polib  # 需要安装: pip install polib
from pathlib import Path
from typing import Dict, List, Tuple, Optional


def extract_translations_from_py(py_file_path: str) -> Dict[str, str]:
    """
    从Python文件中提取翻译字符串和对应的中文翻译
    
    Args:
        py_file_path: Python文件路径
        
    Returns:
        字典: {msgid: translation}
    """
    translations = {}
    
    try:
        with open(py_file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 获取Python文件名，用于后续匹配
        py_filename = os.path.basename(py_file_path)
        
        # 匹配翻译字符串和注释
        # 匹配格式: _("字符串")  # cn: 翻译
        pattern = r'_\([\'"]([^\n]*?)[\'"]\)\s*(?:\#\s*cn:\s*(.*?))?(?=\n|$)'
        
        lines = content.split('\n')
        for line_num, line in enumerate(lines, 1):
            # 跳过包含多个_()的行
            if line.count('_("') > 1 or line.count("_('") > 1:
                continue
            
            # 匹配翻译字符串
            matches = re.findall(pattern, line)
            for match in matches:
                msgid = match[0].strip()
                translation = match[1].strip() if match[1] else None
                
                if translation:
                    # 处理转义字符
                    msgid = msgid.replace('\\"', '"').replace("\\'", "'")
                    translations[msgid] = translation
    
    except Exception as e:
        print(f"读取Python文件 {py_file_path} 时出错: {e}")
    
    return translations


def find_py_files_in_po(po_file_path: str) -> List[str]:
    """
    从PO文件中提取引用的Python文件路径
    
    Args:
        po_file_path: PO文件路径
        
    Returns:
        Python文件路径列表
    """
    py_files = set()
    
    try:
        po = polib.pofile(po_file_path)
        for entry in po:
            for occurrence in entry.occurrences:
                # occurrence格式: /path/to/file.py:line_number
                if occurrence[0].endswith('.py'):
                    py_file_path = occurrence[0]
                    # 提取文件路径（去掉行号）
                    py_file_path = py_file_path.split(':')[0]
                    if os.path.exists(py_file_path):
                        py_files.add(py_file_path)
    
    except Exception as e:
        print(f"读取PO文件 {po_file_path} 时出错: {e}")
    
    return list(py_files)


def update_po_with_translations(po_file_path: str, translations_dict: Dict[str, str]) -> int:
    """
    使用提取的翻译更新PO文件
    
    Args:
        po_file_path: PO文件路径
        translations_dict: 翻译字典 {msgid: translation}
        
    Returns:
        更新的条目数量
    """
    updated_count = 0
    
    try:
        po = polib.pofile(po_file_path)
        
        for entry in po:
            msgid = entry.msgid
            # 只更新空的或需要更新的翻译
            if not entry.msgstr and msgid in translations_dict:
                entry.msgstr = translations_dict[msgid]
                updated_count += 1
                print(f"更新: {msgid[:50]}... -> {translations_dict[msgid][:50]}...")
            elif msgid in translations_dict and entry.msgstr != translations_dict[msgid]:
                # 可选: 如果翻译不同，可以更新或跳过
                # entry.msgstr = translations_dict[msgid]
                # updated_count += 1
                pass
        
        if updated_count > 0:
            po.save(po_file_path)
            print(f"\n已更新 {updated_count} 个翻译条目")
        
        return updated_count
    
    except Exception as e:
        print(f"更新PO文件 {po_file_path} 时出错: {e}")
        return 0


def find_all_py_files_with_translations(root_dir: str) -> Dict[str, Dict[str, str]]:
    """
    递归查找目录下所有包含翻译的Python文件
    
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


def main():
    parser = argparse.ArgumentParser(description='自动填充PO文件翻译')
    parser.add_argument('po_file', help='PO文件路径')
    parser.add_argument('--py-dir', help='Python文件目录（可选，默认从PO文件中提取）')
    parser.add_argument('--py-file', help='单个Python文件路径（可选）')
    parser.add_argument('--dry-run', action='store_true', help='只显示将要更新的内容，不实际修改')
    parser.add_argument('--verbose', '-v', action='store_true', help='显示详细信息')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.po_file):
        print(f"错误: PO文件不存在: {args.po_file}")
        return
    
    # 收集所有翻译
    all_translations = {}
    
    if args.py_file:
        # 处理单个Python文件
        if os.path.exists(args.py_file):
            translations = extract_translations_from_py(args.py_file)
            if translations:
                all_translations = {args.py_file: translations}
            else:
                print(f"警告: Python文件 {args.py_file} 中没有找到翻译注释")
        else:
            print(f"错误: Python文件不存在: {args.py_file}")
            return
    
    elif args.py_dir:
        # 处理目录下的所有Python文件
        if os.path.exists(args.py_dir):
            all_translations = find_all_py_files_with_translations(args.py_dir)
        else:
            print(f"错误: 目录不存在: {args.py_dir}")
            return
    else:
        # 从PO文件中提取Python文件路径
        py_files = find_py_files_in_po(args.po_file)
        if not py_files:
            print("警告: PO文件中没有找到Python文件引用")
            print("请使用 --py-dir 或 --py-file 参数指定Python文件位置")
            return
        
        for py_file in py_files:
            if os.path.exists(py_file):
                translations = extract_translations_from_py(py_file)
                if translations:
                    all_translations[py_file] = translations
    
    if not all_translations:
        print("没有找到任何翻译注释")
        return
    
    # 合并所有翻译
    combined_translations = {}
    for py_file, translations in all_translations.items():
        if args.verbose:
            print(f"从 {py_file} 中找到 {len(translations)} 个翻译注释")
        combined_translations.update(translations)
    
    if args.verbose:
        print(f"\n总共找到 {len(combined_translations)} 个翻译字符串:")
        for msgid, translation in combined_translations.items():
            print(f"  '{msgid[:50]}...' -> '{translation[:50]}...'")
    
    if args.dry_run:
        print(f"\n[干运行模式] 将更新 {len(combined_translations)} 个翻译")
        return
    
    # 更新PO文件
    print(f"\n开始更新PO文件: {args.po_file}")
    updated_count = update_po_with_translations(args.po_file, combined_translations)
    
    if updated_count == 0:
        print("没有需要更新的翻译条目")


if __name__ == '__main__':
    main()