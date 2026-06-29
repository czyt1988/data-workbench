#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
generate_i18n.py - Generate .pot/.po/.mo files from Python source code.

Scans .py files for _("English")  # cn:中文 patterns and generates
translation files. Does not require xgettext/msgmerge/msgfmt tools.

Usage:
    python generate_i18n.py
"""

import os
import re
import struct
from pathlib import Path


# ===================== Configuration =====================
# Script is at DADataAnalysisNodes/i18n/generate_i18n.py
# Package root is DADataAnalysisNodes/ (parent of i18n/)
SCRIPT_DIR = Path(__file__).parent.resolve()          # .../DADataAnalysisNodes/i18n/
PACKAGE_DIR = SCRIPT_DIR.parent                        # .../DADataAnalysisNodes/
DOMAIN = "DADataAnalysisNodes"
LOCALE_DIR = SCRIPT_DIR / "locale"
SUPPORT_LANGUAGES = ["zh_CN", "en"]

# Header metadata for .po/.mo files
PO_HEADER = f"""# {DOMAIN} translations
# Copyright (C) 2026 DAWorkbench
# This file is distributed under the same license as the {DOMAIN} package.
#
msgid ""
msgstr ""
"Project-Id-Version: {DOMAIN} 1.0\\n"
"Report-Msgid-Bugs-To: i18n@example.com\\n"
"POT-Creation-Date: 2026-06-27 00:00+0000\\n"
"PO-Revision-Date: 2026-06-27 00:00+0000\\n"
"Last-Translator: AI Agent\\n"
"Language-Team: \\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=UTF-8\\n"
"Content-Transfer-Encoding: 8bit\\n"
"""


def extract_translations_from_py(py_file_path):
    """
    Extract translation strings and Chinese translations from a Python file.
    Matches: _("English text")  # cn:Chinese translation
    Handles both double-quoted and single-quoted strings.
    """
    translations = {}
    try:
        with open(py_file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Patterns for _("...") and _('...')
        # (?:[^"\\]|\\.)* matches: any char except " and \, OR an escape sequence
        pattern_double = re.compile(r'_\("((?:[^"\\]|\\.)*)"\)')
        pattern_single = re.compile(r"_\('((?:[^'\\]|\\.)*)'\)")
        pattern_cn = re.compile(r'#\s*cn:\s*(.*)')

        lines = content.split('\n')
        for line in lines:
            # Skip lines with multiple _() calls (ambiguous)
            if line.count('_("') > 1 or line.count("_('") > 1:
                continue

            # Try double-quote pattern first
            match = pattern_double.search(line)
            if not match:
                # Try single-quote pattern
                match = pattern_single.search(line)

            if match:
                msgid = match.group(1).strip()
                # Unescape
                msgid = msgid.replace('\\"', '"').replace("\\'", "'").replace('\\\\', '\\')

                # Look for # cn: comment on the same line
                cn_match = pattern_cn.search(line)
                translation = None
                if cn_match:
                    translation = cn_match.group(1).strip()
                    if translation:
                        translation = translation.replace('\\"', '"').replace("\\'", "'")

                if msgid and msgid not in translations:
                    translations[msgid] = translation
                elif msgid and translation and not translations.get(msgid):
                    translations[msgid] = translation

    except Exception as e:
        print(f"Error reading {py_file_path}: {e}")
    return translations


def find_all_py_files(root_dir):
    """Find all .py files excluding i18n/locale and __pycache__"""
    py_files = []
    for root, dirs, files in os.walk(root_dir):
        # Skip i18n, locale, __pycache__, .git, venv directories
        dirs[:] = [d for d in dirs if d not in ('__pycache__', 'locale', 'i18n', '.git', 'venv')]
        for file in files:
            if file.endswith('.py'):
                py_files.append(os.path.join(root, file))
    return py_files


def generate_pot(translations, pot_file):
    """Generate .pot template file"""
    with open(pot_file, 'w', encoding='utf-8') as f:
        f.write(PO_HEADER)
        for msgid in sorted(translations.keys()):
            f.write(f'\nmsgid "{msgid}"\n')
            f.write('msgstr ""\n')
    print(f"Generated POT: {pot_file} ({len(translations)} entries)")


def generate_po(translations, po_file, lang, fill_translations=True):
    """Generate .po file for a specific language"""
    with open(po_file, 'w', encoding='utf-8') as f:
        # Write header with language
        header = PO_HEADER.replace('"Language-Team: \\n"', f'"Language-Team: {lang}\\n"')
        if lang != "en":
            header += f'"Language: {lang}\\n"\n'
        else:
            header += '"Language: en\\n"\n'
        f.write(header)

        for msgid in sorted(translations.keys()):
            f.write(f'\nmsgid "{msgid}"\n')
            if fill_translations and translations[msgid]:
                f.write(f'msgstr "{translations[msgid]}"\n')
            else:
                f.write('msgstr ""\n')
    print(f"Generated PO: {po_file} ({len(translations)} entries)")


def _unescape_po_string(s):
    """Unescape a .po file string value (\\n -> newline, \\" -> quote, etc.)"""
    result = []
    i = 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            next_char = s[i + 1]
            if next_char == 'n':
                result.append('\n')
            elif next_char == 't':
                result.append('\t')
            elif next_char == '"':
                result.append('"')
            elif next_char == "'":
                result.append("'")
            elif next_char == '\\':
                result.append('\\')
            else:
                result.append(s[i])
                result.append(next_char)
            i += 2
        else:
            result.append(s[i])
            i += 1
    return ''.join(result)


def compile_mo(po_file, mo_file):
    """
    Compile .po file to .mo binary file.
    GNU gettext .mo file format implementation.
    """
    # Parse .po file
    entries = []
    current_msgid = None
    current_msgstr = None
    in_msgid = False
    in_msgstr = False

    with open(po_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if line.startswith('msgid "'):
                if current_msgid is not None:
                    entries.append((current_msgid, current_msgstr))
                current_msgid = line[7:-1]  # Remove 'msgid "' and '"'
                current_msgstr = ""
                in_msgid = True
                in_msgstr = False
            elif line.startswith('msgstr "'):
                current_msgstr = line[8:-1]  # Remove 'msgstr "' and '"'
                in_msgid = False
                in_msgstr = True
            elif line.startswith('"') and (in_msgid or in_msgstr):
                # Continuation line
                content = line[1:-1] if line.endswith('"') else line[1:]
                if in_msgid:
                    current_msgid += content
                elif in_msgstr:
                    current_msgstr += content
            elif line == '' and current_msgid is not None:
                entries.append((current_msgid, current_msgstr))
                current_msgid = None
                current_msgstr = None
                in_msgid = False
                in_msgstr = False

    # Don't forget the last entry
    if current_msgid is not None:
        entries.append((current_msgid, current_msgstr))

    # Unescape all strings (convert \n to actual newlines, etc.)
    entries = [(_unescape_po_string(msgid), _unescape_po_string(msgstr)) for msgid, msgstr in entries]

    # Filter: only include entries with non-empty msgstr (except header)
    mo_entries = []
    for msgid, msgstr in entries:
        if msgid == "":
            # Header entry - always include
            mo_entries.append((msgid, msgstr))
        elif msgstr:
            mo_entries.append((msgid, msgstr))

    if not mo_entries:
        # At least include header
        mo_entries.append(("", PO_HEADER))

    # Sort entries by msgid for binary search (required by gettext)
    mo_entries.sort(key=lambda x: x[0])

    n = len(mo_entries)

    # Encode strings
    encoded = [(msgid.encode('utf-8'), msgstr.encode('utf-8')) for msgid, msgstr in mo_entries]

    # Calculate offsets
    header_size = 28  # 7 * 4 bytes
    table_size = n * 8  # Each table entry: (length, offset) = 2 * 4 bytes

    orig_table_offset = header_size
    trans_table_offset = orig_table_offset + table_size
    strings_start = trans_table_offset + table_size

    # Build string data and tables
    orig_table = []
    trans_table = []
    string_data = bytearray()

    for msgid_bytes, msgstr_bytes in encoded:
        orig_table.append((len(msgid_bytes), strings_start + len(string_data)))
        string_data += msgid_bytes + b'\x00'
        trans_table.append((len(msgstr_bytes), strings_start + len(string_data)))
        string_data += msgstr_bytes + b'\x00'

    # Build .mo file
    mo_data = bytearray()

    # Header
    mo_data += struct.pack('<I', 0x950412de)  # Magic number
    mo_data += struct.pack('<I', 0)            # Version
    mo_data += struct.pack('<I', n)            # Number of strings
    mo_data += struct.pack('<I', orig_table_offset)   # Offset of original table
    mo_data += struct.pack('<I', trans_table_offset)  # Offset of translation table
    mo_data += struct.pack('<I', 0)            # Hash table size (0 = no hash)
    mo_data += struct.pack('<I', 0)            # Hash table offset

    # Original strings table
    for length, offset in orig_table:
        mo_data += struct.pack('<II', length, offset)

    # Translation strings table
    for length, offset in trans_table:
        mo_data += struct.pack('<II', length, offset)

    # String data
    mo_data += string_data

    with open(mo_file, 'wb') as f:
        f.write(mo_data)
    print(f"Compiled MO: {mo_file} ({n} entries, {len(mo_data)} bytes)")


def main():
    print(f"=== i18n generation for {DOMAIN} ===\n")

    # 1. Scan Python files (from package root, excluding i18n/ directory)
    py_files = find_all_py_files(PACKAGE_DIR)
    print(f"Scanning {len(py_files)} Python files...")

    all_translations = {}
    for py_file in py_files:
        translations = extract_translations_from_py(py_file)
        if translations:
            print(f"  {os.path.basename(py_file)}: {len(translations)} entries")
            all_translations.update(translations)

    print(f"\nTotal unique translatable strings: {len(all_translations)}")

    if not all_translations:
        print("No translatable strings found!")
        return

    # 2. Create locale directories
    pot_file = LOCALE_DIR / f"{DOMAIN}.pot"
    LOCALE_DIR.mkdir(parents=True, exist_ok=True)

    # 3. Generate .pot
    generate_pot(all_translations, pot_file)

    # 4. Generate .po for each language
    for lang in SUPPORT_LANGUAGES:
        po_dir = LOCALE_DIR / lang / "LC_MESSAGES"
        po_dir.mkdir(parents=True, exist_ok=True)
        po_file = po_dir / f"{DOMAIN}.po"
        mo_file = po_dir / f"{DOMAIN}.mo"

        # For zh_CN, fill in Chinese translations from # cn: comments
        # For en, leave msgstr empty (fallback to msgid = English)
        fill = (lang == "zh_CN")
        generate_po(all_translations, po_file, lang, fill_translations=fill)

        # 5. Compile .mo
        compile_mo(po_file, mo_file)

    print(f"\n=== Done! ===")
    print(f"Files generated in: {LOCALE_DIR}")


if __name__ == '__main__':
    main()
