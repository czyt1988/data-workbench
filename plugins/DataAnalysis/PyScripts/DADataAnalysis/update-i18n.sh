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
error() { echo -e "\033[31m[ERROR] $1\033[0m"; exit 1; }

# ===================== 前置检查 =====================
clear
info "===== 前置检查 =====\n"

# 检查gettext工具
for cmd in xgettext msgmerge msginit msgfmt; do
    if ! command -v $cmd &> /dev/null; then
        error "❌ 未安装 $cmd 命令！请先安装：sudo apt install gettext 或 brew install gettext"
    fi
done

# 查找Python文件
PYTHON_FILES=$(find "${PROJECT_ROOT}" -name "*.py" -not -path "*/venv/*" -not -path "*/.git/*" -not -path "*/__pycache__/*")
if [ -z "${PYTHON_FILES}" ]; then
    warn "⚠️  未扫描到任何Python文件！继续执行可能无法提取翻译字符串"
else
    FILE_COUNT=$(echo "${PYTHON_FILES}" | wc -w)
    info "✅ 扫描到Python文件数：${FILE_COUNT} 个"
    echo "前5个文件："
    echo "${PYTHON_FILES}" | head -5
fi
echo ""

# ===================== 生成POT模板 =====================
info "第一步：生成/更新.pot模板"
mkdir -p "$(dirname "${POT_FILE}")"

# 构建xgettext命令
XGETTEXT_CMD="xgettext"

# 添加文件列表（如果存在）
if [ ! -z "${PYTHON_FILES}" ]; then
    XGETTEXT_CMD="${XGETTEXT_CMD} ${PYTHON_FILES}"
else
    warn "⚠️  没有Python文件，POT文件可能为空"
fi

# 执行xgettext
${XGETTEXT_CMD} \
    --language=Python \
    --keyword=_ \
    --keyword=_:1,2c \
    --keyword=pgettext:1c,2 \
    --keyword=npgettext:1c,2,3 \
    --keyword=ngettext:1,2 \
    --from-code="${ENCODING}" \
    --output="${POT_FILE}" \
    --package-name="${DOMAIN}" \
    --package-version="1.0" \
    --copyright-holder="DataAnalysis" \
    --msgid-bugs-address="i18n@example.com" \
    --add-comments=TRANSLATORS: \
    --width=80 \
    --sort-by-file

if [ $? -eq 0 ] && [ -f "${POT_FILE}" ]; then
    info "✅ POT模板生成成功：${POT_FILE}"
    
    # 确保POT文件使用UTF-8编码
    sed -i 's/CHARSET/UTF-8/' "${POT_FILE}"
else
    error "❌ POT文件生成失败：${POT_FILE}"
fi
echo ""

# ===================== 处理PO文件 =====================
info "第二步：同步/初始化PO文件"
for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_DIR="${LOCALE_DIR}/${LANG}/LC_MESSAGES"
    PO_FILE="${PO_DIR}/${DOMAIN}.po"
    mkdir -p "${PO_DIR}"

    if [ -f "${PO_FILE}" ]; then
        info "🔄 同步 ${LANG} PO文件"
        
        # 修正：msgmerge 参数顺序 - 先PO文件，再POT文件
        if msgmerge --update "${PO_FILE}" "${POT_FILE}" --backup=none --no-wrap; then
            info "✅ ${LANG} PO同步完成"
        else
            warn "⚠️  ${LANG} PO同步失败，尝试初始化新文件"
            # 如果同步失败，尝试创建新文件
            msginit --input="${POT_FILE}" \
                    --output="${PO_FILE}" \
                    --locale="${LANG}" \
                    --no-translator \
                    --width=80
        fi
    else
        info "📝 初始化 ${LANG} PO文件"
        
        if msginit --input="${POT_FILE}" \
                   --output="${PO_FILE}" \
                   --locale="${LANG}" \
                   --no-translator \
                   --width=80; then
            info "✅ ${LANG} PO初始化完成"
        else
            error "❌ 初始化 ${LANG} PO失败"
        fi
    fi
    
    # 确保PO文件使用UTF-8编码
    if [ -f "${PO_FILE}" ]; then
        sed -i 's/CHARSET/UTF-8/' "${PO_FILE}"
    fi
done
echo ""

# ===================== 编译MO文件 =====================
info "第三步：编译PO为MO文件"
for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.po"
    MO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.mo"
    
    if [ -f "${PO_FILE}" ]; then
        info "🔨 编译 ${LANG} MO文件"
        
        if msgfmt --check --verbose -o "${MO_FILE}" "${PO_FILE}"; then
            info "✅ ${LANG} MO编译成功"
        else
            warn "⚠️  ${LANG} MO编译失败"
        fi
    else
        warn "⚠️  ${LANG} PO文件不存在，跳过编译"
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
        echo "${LANG}: PO文件不存在"
    fi
done
echo "=========================================="
echo ""

# ===================== 完成 =====================
info "===== 全部操作完成！====="
info "📌 文件路径验证："
info "  - POT文件：${POT_FILE}"

for LANG in "${SUPPORT_LANGUAGES[@]}"; do
    PO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.po"
    MO_FILE="${LOCALE_DIR}/${LANG}/LC_MESSAGES/${DOMAIN}.mo"
    
    if [ -f "${PO_FILE}" ]; then
        PO_SIZE=$(stat -c%s "${PO_FILE}" 2>/dev/null || stat -f%z "${PO_FILE}" 2>/dev/null || echo "未知")
        info "  - ${LANG}.po：${PO_FILE} (${PO_SIZE} bytes)"
    else
        info "  - ${LANG}.po：文件不存在"
    fi
    
    if [ -f "${MO_FILE}" ]; then
        MO_SIZE=$(stat -c%s "${MO_FILE}" 2>/dev/null || stat -f%z "${MO_FILE}" 2>/dev/null || echo "未知")
        info "  - ${LANG}.mo：${MO_FILE} (${MO_SIZE} bytes)"
    else
        info "  - ${LANG}.mo：文件不存在"
    fi
done

echo -e "\n💡 按任意键退出..."
read -n 1 -s -r