#!/bin/bash
set -e

# ===================== 核心配置 =====================
DOMAIN="DASystemNodes"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
POT_FILE="${PROJECT_ROOT}/i18n/locale/${DOMAIN}.pot"
LOCALE_DIR="${PROJECT_ROOT}/i18n/locale"
SUPPORT_LANGUAGES=("zh_CN" "en")
ENCODING="utf-8"

# ===================== 辅助函数 =====================
info() { echo -e "\033[32m[INFO] $1\033[0m"; }
warn() { echo -e "\033[33m[WARN] $1\033[0m"; }
error() { echo -e "\033[31m[ERROR] $1\033[0m"; exit 1; }

# ===================== 前置检查 =====================
clear
info "===== 前置检查 =====\n"

# 检查 gettext 工具
for cmd in xgettext msgmerge msginit msgfmt; do
    if ! command -v $cmd &> /dev/null; then
        error "❌ 未安装 $cmd 命令！请先安装：sudo apt install gettext 或使用 Git Bash"
    fi
done

# 查找 Python 文件（排除 i18n/locale 目录和 __pycache__）
PYTHON_FILES=$(find "${PROJECT_ROOT}" -name "*.py" \
    -not -path "*/venv/*" \
    -not -path "*/.git/*" \
    -not -path "*/__pycache__/*" \
    -not -path "*/i18n/locale/*")
if [ -z "${PYTHON_FILES}" ]; then
    warn "⚠️  未扫描到任何 Python 文件！"
else
    FILE_COUNT=$(echo "${PYTHON_FILES}" | wc -w)
    info "✅ 扫描到 Python 文件数：${FILE_COUNT} 个"
fi
echo ""

# ===================== 生成 POT 模板 =====================
info "第一步：生成/更新 .pot 模板"
mkdir -p "$(dirname "${POT_FILE}")"

if [ ! -z "${PYTHON_FILES}" ]; then
    xgettext --language=Python \
        --keyword=_ \
        --keyword=_:1,2c \
        --keyword=pgettext:1c,2 \
        --keyword=npgettext:1c,2,3 \
        --keyword=ngettext:1,2 \
        --from-code="${ENCODING}" \
        --output="${POT_FILE}" \
        --package-name="${DOMAIN}" \
        --package-version="1.0" \
        --copyright-holder="DASystemNodes" \
        --msgid-bugs-address="i18n@example.com" \
        --width=80 \
        --sort-by-file \
        ${PYTHON_FILES} 2>/dev/null || warn "xgettext 执行时出现警告，继续..."
fi

if [ -f "${POT_FILE}" ]; then
    sed -i 's/CHARSET/UTF-8/' "${POT_FILE}"
    info "✅ POT 模板生成成功：${POT_FILE}"
    info "  包含字符串数: $(grep -c '^msgid' "${POT_FILE}" || echo '未知')"
else
    error "❌ POT 文件生成失败"
fi
echo ""

# ===================== 处理 PO 文件 =====================
info "第二步：同步/初始化 PO 文件"
for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_DIR="${LOCALE_DIR}/${LANG}/LC_MESSAGES"
    PO_FILE="${PO_DIR}/${DOMAIN}.po"
    mkdir -p "${PO_DIR}"

    if [ -f "${PO_FILE}" ]; then
        info "🔄 同步 ${LANG} PO 文件"
        if msgmerge --update "${PO_FILE}" "${POT_FILE}" --backup=none --no-wrap; then
            info "✅ ${LANG} PO 同步完成"
        else
            warn "⚠️  ${LANG} PO 同步失败，尝试初始化新文件"
            msginit --input="${POT_FILE}" \
                    --output="${PO_FILE}" \
                    --locale="${LANG}" \
                    --no-translator \
                    --width=80
        fi
    else
        info "📝 初始化 ${LANG} PO 文件"
        if msginit --input="${POT_FILE}" \
                   --output="${PO_FILE}" \
                   --locale="${LANG}" \
                   --no-translator \
                   --width=80; then
            info "✅ ${LANG} PO 初始化完成"
        else
            error "❌ 初始化 ${LANG} PO 失败"
        fi
    fi

    if [ -f "${PO_FILE}" ]; then
        sed -i 's/CHARSET/UTF-8/' "${PO_FILE}"
    fi
done
echo ""

# ===================== 编译 MO 文件 =====================
info "第三步：编译 PO 为 MO 文件"
for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.po"
    MO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.mo"

    if [ -f "${PO_FILE}" ]; then
        info "🔨 编译 ${LANG} MO 文件"
        if msgfmt --check --verbose -o "${MO_FILE}" "${PO_FILE}"; then
            info "✅ ${LANG} MO 编译成功"
        else
            warn "⚠️  ${LANG} MO 编译失败"
        fi
    else
        warn "⚠️  ${LANG} PO 文件不存在，跳过编译"
    fi
done
echo ""

# ===================== 生成报告 =====================
info "第四步：生成翻译统计报告"
echo "=========================================="
for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.po"
    if [ -f "${PO_FILE}" ]; then
        echo -n "${LANG}: "
        msgfmt --statistics -o /dev/null "${PO_FILE}" 2>&1 || echo "无法统计"
    else
        echo "${LANG}: PO 文件不存在"
    fi
done
echo "=========================================="
echo ""

info "===== 全部操作完成！====="
info "💡 提示：运行 update_po.py 可从 # cn: 注释自动填充翻译："
info "   python i18n/update_po.py i18n/locale/zh_CN/LC_MESSAGES/${DOMAIN}.po --py-dir ."

echo -e "\n按任意键退出..."
read -n 1 -s -r
