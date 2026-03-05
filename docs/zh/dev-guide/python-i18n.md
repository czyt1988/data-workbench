# 脚本的国际化

!!! example "示例"
    脚本的国际化可参考插件`plugins/DataAnalysis/PyScripts/DADataAnalysis`

## 概述

DAWorkBench 的 Python 脚本使用 GNU gettext 标准实现国际化（i18n）。整个流程为：

1. 在代码中使用 `_()` 标记需要翻译的文本
2. 使用 `xgettext` 从代码中提取待翻译文本，生成 `.pot` 模板文件
3. 使用 `msginit` 为每种语言生成 `.po` 翻译文件
4. 手动翻译 `.po` 文件中的文本
5. 使用 `msgfmt` 编译 `.po` 文件为 `.mo` 二进制文件
6. 程序运行时加载对应语言的 `.mo` 文件

文件类型说明：

| 文件类型 | 说明 |
|---------|------|
| `.pot` | 翻译模板文件，从代码中提取的所有待翻译文本集合，不关联具体语言 |
| `.po` | 翻译文件，记录"原文本 -> 目标语言文本"的映射关系 |
| `.mo` | 编译文件，`.po` 的二进制版本，程序运行时加载 |

## 国际化目录结构

脚本国际化的目录结构大致如下

```txt
你的插件脚本目录/              # 库根目录
├── __init__.py              # 对外暴露核心API（包括setup_i18n）
├── i18n/                    # 国际化专用子模块（内部逻辑）
│   ├── __init__.py
│   ├── core.py              # 封装setup_i18n的核心实现
│   └── locale/              # 翻译文件目录（和之前的locale结构一致）
│       ├── en/
│       │   └── LC_MESSAGES/
│       │       ├── message.po
│       │       └── message.mo
│       └── zh_CN/
│           └── LC_MESSAGES/
│               ├── message.po
│               └── message.mo
└── xxx.py                 # 库的其他业务逻辑（使用_()翻译文本）
```

1. 在你的插件脚本目录下创建一个`i18n`文件夹，并创建一个`__init__.py`文件，代表这个目录为国际化目录，`__init__.py`文件可为空

2. `i18n`文件夹下创建一个`core.py`，其内容如下：

```python
    # -*- coding: utf-8 -*-
# i18n/core.py
import gettext
import os,sys
import locale
from typing import Optional

# 翻译文件根目录（定位到i18n/locale）
LOCALES_DIR = os.path.join(os.path.dirname(__file__), "locale")
# 库的翻译域（建议用库名，避免和其他库冲突）
DOMAIN = "DADataAnalysis"

def get_system_language() -> str:
    """
    获取系统默认语言
    优先级：环境变量 > 系统locale > 默认值
    """
    # 1. 检查环境变量（最高优先级）
    env_lang = os.environ.get('LANG') or os.environ.get('LC_ALL') or os.environ.get('LC_MESSAGES')
    if env_lang:
        # 处理格式如 "zh_CN.UTF-8" 的情况
        lang_code = env_lang.split('.')[0]
        return _normalize_language_code(lang_code)
    
    # 2. 使用locale模块获取系统语言
    try:
        sys_lang, _ = locale.getdefaultlocale()
        if sys_lang:
            return _normalize_language_code(sys_lang)
    except:
        pass
    
    # 3. 最后尝试获取平台特定的语言
    if sys.platform == "win32":
        try:
            import ctypes
            windll = ctypes.windll.kernel32
            # 获取系统默认UI语言
            lang_id = windll.GetUserDefaultUILanguage()
            # Windows语言ID到语言代码的映射
            win_lang_map = {
                0x0409: "en_US",  # 英语(美国)
                0x0804: "zh_CN",  # 简体中文
                0x0404: "zh_TW",  # 繁体中文
                0x0411: "ja_JP",  # 日语
                0x0407: "de_DE",  # 德语
                0x040C: "fr_FR",  # 法语
                0x0410: "it_IT",  # 意大利语
                0x0C0A: "es_ES",  # 西班牙语
                0x0412: "ko_KR",  # 韩语
                0x0419: "ru_RU",  # 俄语
            }
            return win_lang_map.get(lang_id, "en_US")
        except:
            pass
    
    # 4. 默认值
    return "zh_CN"

def _normalize_language_code(lang_code: str) -> str:
    """
    标准化语言代码
    例如：zh_CN.UTF-8 -> zh_CN, en-US -> en_US
    """
    # 移除编码部分
    lang_code = lang_code.split('.')[0]
    # 标准化分隔符
    lang_code = lang_code.replace('-', '_')
    return lang_code

def get_available_languages() -> list:
    """
    获取可用的语言列表
    返回已经存在翻译文件的语言代码列表
    """
    available = []
    if os.path.exists(LOCALES_DIR):
        for lang_dir in os.listdir(LOCALES_DIR):
            lang_path = os.path.join(LOCALES_DIR, lang_dir, "LC_MESSAGES", f"{DOMAIN}.mo")
            if os.path.exists(lang_path):
                available.append(lang_dir)
    return available


def setup_i18n(
    language: Optional[str] = None,
    install_global: bool = True,
    use_system_language: bool = True
) -> gettext.GNUTranslations:
    """
    初始化库的国际化配置
    
    Args:
        language: 指定语言代码（如 "zh_CN", "en_US"）
        install_global: 是否安装到全局命名空间
        use_system_language: 当language为None时，是否使用系统语言
    
    Returns:
        翻译对象（可用于局部调用）
    """
    # 确定要使用的语言
    if language is None and use_system_language:
        language = get_system_language()
    elif language is None:
        language = "zh_CN"  # 默认中文
    
    # 标准化语言代码
    language = _normalize_language_code(language)
    
    # 检查语言是否可用，如果不可用则尝试回退
    available_langs = get_available_languages()
    if language not in available_langs:
        # 尝试回退到主语言（如 zh_TW -> zh_CN）
        main_lang = language.split('_')[0]
        fallback_lang = None
        for lang in available_langs:
            if lang.startswith(main_lang):
                fallback_lang = lang
                break
        
        if fallback_lang:
            print(f"警告：语言 '{language}' 的翻译文件不存在，使用回退语言 '{fallback_lang}'")
            language = fallback_lang
        else:
            print(f"警告：语言 '{language}' 的翻译文件不存在，使用默认语言 'zh_CN'")
            language = "zh_CN"
    
    try:
        # 加载指定语言的翻译文件
        trans = gettext.translation(
            domain=DOMAIN,
            localedir=LOCALES_DIR,
            languages=[language],
            fallback=True  # 找不到翻译时用原文本
        )
        print(f"已加载语言: {language}")
    except FileNotFoundError:
        # 兜底：空翻译（保证程序不崩溃）
        print(f"错误：无法加载语言 '{language}' 的翻译文件")
        trans = gettext.NullTranslations()
    
    # 根据参数决定是否安装到全局
    if install_global:
        trans.install()  # 把_注入builtins全局命名空间
    
    return trans
```

## 在代码中使用国际化

在需要翻译的Python脚本中，使用 `_()` 函数包裹文本：

```python
# xxx.py
from .i18n.core import setup_i18n

# 初始化国际化
setup_i18n()

# 使用 _() 标记需要翻译的文本
print(_("没有选中数据"))
print(_("数据不是DataFrame类型"))
```

## 生成语言模板文件

通过`xgettext.exe`可以生成语言文件模板`.pot`，这个工具是GNU gettext的组成部分，你可以单独下载，一般你安装了git，它会自带这个工具。

你可以通过everything搜索xgettext，看看是否系统已经有安装这个工具

例如我计算机的xgettext位于此目录下：C:\Program Files\Git\usr\bin\xgettext.exe

那么你可以像下面这样调用xgettext.exe

```bash
"C:\Program Files\Git\usr\bin\xgettext.exe" -d DADataAnalysis -o i18n/locale/DADataAnalysis.pot dataframe_cleaner.py
```

xgettext的参数说明：

```bash
xgettext [可选参数] -o 输出文件.pot 待扫描的代码文件1 代码文件2 ...
```

### xgettext核心参数

xgettext核心参数说明如下：

| 参数         | 作用                                                                 |
|--------------|----------------------------------------------------------------------|
| `-d <domain>`| 指定翻译域，必须和脚本中的DOMAIN保持一致否则会找不到翻译 |
| `-o <file>`  | 指定输出的 `.pot` 文件路径（必填，否则输出到标准输出）|
| `-l <lang>`  | 指定代码语言（如 `-l python`，可自动识别Python语法的 `_()` 标记）|
| `-k`         | 指定自定义翻译函数名（默认识别 `_`，若用 `t()` 则加 `-k t`）|
| `--from-code`| 指定代码文件编码（如 `--from-code=utf-8`，避免中文乱码）|

一个通用命令如下所示：

```bash
xgettext -d mylib -l python --from-code=utf-8 -o locale/mylib.pot utils.py i18n/core.py
```

- `-d mylib`：翻译域设为 `mylib`（和你后续的 `.po/.mo` 文件名一致）；
- `-l python`：明确识别Python语法（可选，xgettext通常能自动识别）；
- `--from-code=utf-8`：避免中文原文本乱码；
- `-o locale/mylib.pot`：输出到 `locale` 目录下的 `mylib.pot`；
- 最后跟需要扫描的代码文件（可写多个，或用通配符 `*.py` 扫描所有py文件）。

若项目有多个子目录，用通配符扫描所有 `.py` 文件：

```bash
xgettext -d mylib -l python --from-code=utf-8 -o locale/mylib.pot $(find . -name "*.py")
```

- `find . -name "*.py"`：递归查找当前目录下所有 `.py` 文件；
- `$(...)`：把查找结果作为参数传给 xgettext。

执行命令后，`mylib.pot` 内容如下（自动提取所有 `_()` 文本）：

```pot
msgid ""
msgstr ""
"Content-Type: text/plain; charset=UTF-8\n"
"Generated-By: xgettext 0.21\n"
"Language: Python\n"

msgid "没有选中数据"
msgstr ""

msgid "数据不是DataFrame类型"
msgstr ""

msgid "删除缺失值参数设置"
msgstr ""

msgid "任意一个存在即删除"
msgstr ""

msgid "是否重建索引"
msgstr ""
```

`.pot` 文件内容说明：

- `msgid`：待翻译的原文本（从代码中提取的 `_("xxx")` 里的内容）；
- `msgstr`：空值，供后续生成 `.po` 文件时填写翻译；
- 头部的编码、生成工具等信息自动填充，无需手动修改。

!!! warning "注意"
    中文如果乱码，说明没有加`--from-code=utf-8` 参数，因为 xgettext 默认可能用系统编码（如GBK），导致中文原文本乱码。

!!! tip "Tip"
    `.pot`文件是翻译的模板文件，`.po` 文件是翻译文件，`.mo` 文件是编译文件。 `.pot` 是从代码中提取的所有待翻译文本的集合，**不关联任何具体语言**，仅记录需要翻译哪些文本。

### 批量更新.pot文件

当代码中新增/修改了 `_()` 文本，重新执行 `xgettext` 命令即可覆盖旧的 `.pot` 文件（会保留原有结构，更新 `msgid`）。

## 生成翻译文件

`.pot` 文件是所有语言的翻译模板，接下来需为每个目标语言生成 `.po` 文件，生成翻译文件使用`msginit.exe`工具

```bash
# 为中文生成 .po 文件
mkdir -p locale/zh_CN/LC_MESSAGES
msginit -i locale/mylib.pot -o locale/zh_CN/LC_MESSAGES/mylib.po -l zh_CN

# 为英文生成 .po 文件
mkdir -p locale/en/LC_MESSAGES
msginit -i locale/mylib.pot -o locale/en/LC_MESSAGES/mylib.po -l en
```

`po`文件内容大致如下：

```po
msgid ""
msgstr ""
"Content-Type: text/plain; charset=UTF-8\n"
"Language: zh_CN\n"  # 绑定具体语言

msgid "没有选中数据"
msgstr "No data selected"  # 手动填写的英文翻译

msgid "删除缺失值后，是否重建索引"
msgstr "Whether to rebuild the index after removing missing values"
```

`po`文件记录"原文本->目标语言文本"的映射关系，是人工维护翻译的核心文件，最终会被 `msgfmt` 编译为 `.mo` 二进制文件供程序加载。

生成po文件后，你需要进行手动翻译并保存

### 更新翻译文件

当代码中新增/修改了 `_()` 文本时：

- 第一步：重新执行 `xgettext` 生成**新的 `.pot` 文件**（更新 `msgid` 列表）；
- 第二步：用 `msgmerge` 命令将新 `.pot` 的变更同步到已有 `.po` 文件（保留原有翻译，新增未翻译的 `msgid`）：

  ```bash
  # 同步中文 .po 文件（保留已翻译内容，新增新的 msgid）
  msgmerge -U locale/zh_CN/LC_MESSAGES/mylib.po locale/mylib.pot
  ```

- 第三步：手动补充新 `msgid` 的翻译，重新编译 `.mo`。

## 生成编译文件

`.po` 是文本文件，程序运行时无法直接加载，需要用 `msgfmt` 编译为二进制的 `.mo` 文件（gettext 识别的格式）。

核心命令：`msgfmt`

```bash
# 语法：msgfmt -o 输出的.mo文件路径 待编译的.po文件路径
```

例如：

编译中文 .po 文件

```bash
msgfmt -o locale/zh_CN/LC_MESSAGES/mylib.mo locale/zh_CN/LC_MESSAGES/mylib.po
```

编译英文 .po 文件

```bash
msgfmt -o locale/en/LC_MESSAGES/mylib.mo locale/en/LC_MESSAGES/mylib.po
```

如果有多个语言（如 ja、fr），可写一个批量脚本（`compile_po.sh`），避免重复执行命令：

```bash
#!/bin/bash
# 遍历所有 .po 文件，编译为同名 .mo 文件
find locale -name "*.po" -exec sh -c 'msgfmt -o "${0%.po}.mo" "$0"' {} \;
echo "所有 .po 文件已编译为 .mo 文件！"
```

- 执行脚本：

  ```bash
  # 赋予执行权限
  chmod +x compile_po.sh
  # 运行脚本
  ./compile_po.sh
  ```

最终你的文件结构如下：

```txt
你的插件脚本目录/              # 库根目录
├── __init__.py              # 对外暴露核心API（包括setup_i18n）
├── i18n/                    # 国际化专用子模块（内部逻辑）
│   ├── __init__.py
│   ├── core.py              # 封装setup_i18n的核心实现
│   └── locale/              # 翻译文件目录（和之前的locale结构一致）
│       ├── 域名.pot          # 翻译模板文件
│       ├── en/
│       │   └── LC_MESSAGES/
│       │       ├── message.po
│       │       └── message.mo
│       └── zh_CN/
│           └── LC_MESSAGES/
│               ├── message.po
│               └── message.mo
└── xxx.py                 # 库的其他业务逻辑（使用_()翻译文本）
```

## 自动化脚本

为了避免重复执行命令，我们可以编写一个自动化脚本，实现批量生成 `.pot`、`.po`、`.mo` 文件。

```bash
#!/bin/bash
set -e

# ===================== 核心配置 =====================
DOMAIN="DADataAnalysis"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
POT_FILE="${PROJECT_ROOT}/i18n/locale/${DOMAIN}.pot"
LOCALE_DIR="${PROJECT_ROOT}/i18n/locale"
SUPPORT_LANGUAGES=("zh_CN" "en")
ENCODING="utf-8"

# ===================== 辅助函数 =====================
info() { echo -e "\033[32m[INFO] $1\033[0m"; }
warn() { echo -e "\033[33m[WARN] $1\033[0m"; }
error() { echo -e "\033[31m[ERROR] $1\033[0m"; }

# 检查文件编码
check_po_encoding() {
    local po_file="$1"
    if [ -f "$po_file" ]; then
        # 尝试多种编码检测
        if file -b "$po_file" | grep -qi "utf-8"; then
            return 0
        elif file -b "$po_file" | grep -qi "ascii"; then
            return 0
        else
            warn "PO文件可能不是UTF-8编码: $po_file"
            return 1
        fi
    fi
    return 0
}

# 备份PO文件
backup_po_file() {
    local po_file="$1"
    local backup_dir="${LOCALE_DIR}/backup"
    mkdir -p "$backup_dir"
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_file="${backup_dir}/$(basename "$po_file").backup.${timestamp}"
    cp "$po_file" "$backup_file"
    info "已备份原文件: $backup_file"
}

# ===================== 前置检查 =====================
clear
info "===== 前置检查 =====\n"

# 检查gettext工具
for cmd in xgettext msgmerge msginit msgfmt; do
    if ! command -v $cmd &> /dev/null; then
        error "未安装 $cmd 命令！请先安装：sudo apt install gettext 或 brew install gettext"
    fi
done

# 查找Python文件
PYTHON_FILES=$(find "${PROJECT_ROOT}" -name "*.py" -not -path "*/venv/*" -not -path "*/.git/*" -not -path "*/__pycache__/*")
if [ -z "${PYTHON_FILES}" ]; then
    warn "未扫描到任何Python文件！继续执行可能无法提取翻译字符串"
else
    FILE_COUNT=$(echo "${PYTHON_FILES}" | wc -w)
    info "扫描到Python文件数：${FILE_COUNT} 个"
fi
echo ""

# ===================== 生成POT模板 =====================
info "第一步：生成/更新.pot模板"
mkdir -p "$(dirname "${POT_FILE}")"

# 执行xgettext
xgettext --language=Python \
    --keyword=_ \
    --keyword=ngettext:1,2 \
    --from-code="${ENCODING}" \
    --output="${POT_FILE}" \
    --package-name="${DOMAIN}" \
    --package-version="1.0" \
    --copyright-holder="DataAnalysis" \
    --msgid-bugs-address="i18n@example.com" \
    --width=80 \
    --sort-by-file \
    ${PYTHON_FILES} 2>/dev/null || warn "xgettext执行时出现警告，继续..."

if [ -f "${POT_FILE}" ]; then
    # 确保POT文件使用UTF-8编码
    sed -i 's/CHARSET/UTF-8/' "${POT_FILE}" 2>/dev/null || true
    info "POT模板生成成功：${POT_FILE}"
    info "  包含字符串数: $(grep -c '^msgid' "${POT_FILE}" || echo "未知")"
else
    error "POT文件生成失败"
fi
echo ""

# ===================== 处理PO文件 =====================
info "第二步：同步/初始化PO文件"

for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_DIR="${LOCALE_DIR}/${LANG}/LC_MESSAGES"
    PO_FILE="${PO_DIR}/${DOMAIN}.po"
    mkdir -p "${PO_DIR}"
    
    # 备份现有PO文件
    if [ -f "${PO_FILE}" ]; then
        backup_po_file "${PO_FILE}"
        
        # 检查文件编码
        if ! check_po_encoding "${PO_FILE}"; then
            warn "${LANG} PO文件编码可能有问题，尝试修复..."
            # 尝试转换为UTF-8
            if iconv -f GBK -t UTF-8 "${PO_FILE}" > "${PO_FILE}.utf8" 2>/dev/null; then
                mv "${PO_FILE}.utf8" "${PO_FILE}"
                info "  已转换${LANG} PO文件为UTF-8编码"
            fi
        fi
    fi
    
    if [ -f "${PO_FILE}" ]; then
        info "同步 ${LANG} PO文件"
        
        # 使用临时文件进行合并，避免直接覆盖
        PO_TEMP="${PO_FILE}.temp"
        cp "${PO_FILE}" "${PO_TEMP}"
        
        if msgmerge --quiet --update "${PO_TEMP}" "${POT_FILE}" --backup=none --no-wrap 2>/dev/null; then
            mv "${PO_TEMP}" "${PO_FILE}"
            info "${LANG} PO同步完成"
            
            # 统计翻译情况
            total_msg=$(grep -c '^msgid' "${PO_FILE}" || echo "0")
            translated_msg=$(grep -c '^msgstr' "${PO_FILE}" | grep -v 'msgstr ""' || echo "0")
            info "  统计: ${translated_msg}/${total_msg} 已翻译"
        else
            warn "${LANG} PO同步失败，保留原文件"
            rm -f "${PO_TEMP}"
        fi
        
    else
        info "初始化 ${LANG} PO文件"
        
        if msginit --input="${POT_FILE}" \
                   --output="${PO_FILE}" \
                   --locale="${LANG}" \
                   --no-translator \
                   --width=80 2>/dev/null; then
            info "${LANG} PO初始化完成"
        else
            warn "初始化 ${LANG} PO失败，创建空文件"
            # 创建空的PO文件
            cat > "${PO_FILE}" << EOF
# ${DOMAIN} translations for ${LANG}
# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER
# This file is distributed under the same license as the ${DOMAIN} package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: ${DOMAIN} 1.0\\n"
"Report-Msgid-Bugs-To: i18n@example.com\\n"
"POT-Creation-Date: $(date +%Y-%m-%d %H:%M%z)\\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"
"Language-Team: ${LANG}\\n"
"Language: ${LANG}\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=UTF-8\\n"
"Content-Transfer-Encoding: 8bit\\n"
EOF
        fi
    fi
    
    # 确保PO文件使用UTF-8编码
    if [ -f "${PO_FILE}" ]; then
        sed -i 's/CHARSET/UTF-8/' "${PO_FILE}" 2>/dev/null || true
    fi
done
echo ""

# ===================== 编译MO文件 =====================
info "第三步：编译PO为MO文件"
for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.po"
    MO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.mo"
    
    if [ -f "${PO_FILE}" ]; then
        info "编译 ${LANG} MO文件"
        
        if msgfmt --check --verbose -o "${MO_FILE}" "${PO_FILE}" 2>&1 | tee "/tmp/msgfmt_${LANG}.log"; then
            info "${LANG} MO编译成功"
        else
            warn "${LANG} MO编译失败，查看 /tmp/msgfmt_${LANG}.log 获取详情"
        fi
    else
        warn "${LANG} PO文件不存在，跳过编译"
    fi
done
echo ""

# ===================== 完成 =====================
info "===== 全部操作完成！====="
info "翻译状态："

for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.po"
    MO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.mo"
    
    if [ -f "${PO_FILE}" ]; then
        total=$(grep -c '^msgid' "${PO_FILE}" 2>/dev/null || echo "0")
        translated=$(grep -c '^msgstr' "${PO_FILE}" 2>/dev/null | grep -v 'msgstr ""' || echo "0")
        info "  ${LANG}: ${translated}/${total} 已翻译"
    fi
done

echo -e "\n按任意键退出..."
read -n 1 -s -r
```

把这个脚本保存为 `update_i18n.sh`，放到你的插件目录下

修改前面配置项目，即可一键自动生成国际化文件

!!! tip "注意"
    如果你安装了git bash，用git bash打开脚本，而不是使用Windows自带的cmd，git bash默认已经安装了xgettext和msgfmt这些工具

## 快速参考

国际化操作的完整流程总结：

| 步骤 | 命令 | 说明 |
|------|------|------|
| 1. 提取文本 | `xgettext -d DOMAIN --from-code=utf-8 -o xxx.pot *.py` | 从代码提取待翻译文本 |
| 2. 初始化翻译 | `msginit -i xxx.pot -o xxx.po -l zh_CN` | 为目标语言创建翻译文件 |
| 3. 翻译 | 手动编辑 `.po` 文件 | 填写 `msgstr` 翻译内容 |
| 4. 更新翻译 | `msgmerge -U xxx.po xxx.pot` | 同步新增的待翻译文本 |
| 5. 编译 | `msgfmt -o xxx.mo xxx.po` | 生成程序可加载的二进制文件 |
