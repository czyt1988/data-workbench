#!/usr/bin/env bash
# ============================================================================
# ui_widget_inventory.sh - 界面控件通用化盘点（六组数字）
#
# 用途:
#   给「界面控件通用化」方案提供可复跑的基线/验收口径。方案见
#   tmp/界面控件通用化方案-v2-2026-10-25.md 第 6.1 节。
#
# 统计范围:
#   仓库内 git 跟踪的 .cpp/.h/.hpp/.ui，排除 src/3rdparty/（预编译第三方库，
#   不在本轮改造范围）。工作树里的 build*/.worktree/tmp 也不在 git 跟踪列表中。
#
# 用法:
#   bash scripts/ui_widget_inventory.sh
#
# 口径说明（改脚本前先读）:
#   * 「调用数」按行计：一行出现一次 QMessageBox::warning 即计 1，不按弹窗实例计；
#     同一行同时出现多个宏的极端情况忽略不计。
#   * 「字段校验类弹窗」按文件标注：图表添加窗口与属性设置面板里的取数/输入校验
#     属于字段校验，其余（删除确认、覆盖确认、保存提示）属于确认/提示类。
#     标注列表见 VALIDATION_FILE_REGEX；人工标注一次，新增文件需同步维护。
#   * 「可勾选列表」按 `ItemIsUserCheckable` 计：它是平铺 QListWidget 勾选实现
#     的最小共同特征；QTreeWidget/QTableView 的分组勾选不算（方案 §5 已排除）。
#   * 「字符串列表 + 增删移」按同一文件内同时出现 QListWidget 与 Add/Remove
#     按钮计，含少量误报（对象列表也会命中），用于观察趋势而非精确断言。
#   * 「自增组」按 .ui 里对象名匹配 groupBox.*Autoincrement 的 QGroupBox 计。
# ============================================================================

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT" || exit 1

# 字段校验类弹窗的文件标注（方案 §5：两个图表添加窗口 25 处 = XYE 13 + OHLC 12）
VALIDATION_FILE_REGEX='^src/DAGui/ChartAddItem/'
TWO_WINDOWS_REGEX='^(src/DAGui/ChartAddItem/DAChartAddXYESeriesWidget\.cpp|src/DAGui/ChartAddItem/DAChartAddOHLCSeriesWidget\.cpp)$'
# 死 UI：未纳入构建（src/DAGui/CMakeLists.txt 有注释说明），统计 live 数时要排除
DEAD_UI_REGEX='DAChartAddCurveWidget\.ui$'

# 本轮关注的 UI 基类（用于第 1 组「界面类」过滤）
UI_BASE_REGEX='^(QWidget|QDialog|QMainWindow|QFrame|QGroupBox|QScrollArea|QSplitter|QTabWidget|QTabBar|QStackedWidget|QAbstractItemView|QListView|QTreeView|QTableView|QGraphicsView|QGraphicsWidget|QGraphicsItem|QwtPlotItem|QwtPlot|QwtWidgetOverlay)$'

msg() { printf '%s\n' "$*"; }
hr() { msg "----------------------------------------------------------------------"; }

# git 跟踪的界面相关源文件（排除 3rdparty）
mapfile -t FILES < <(git ls-files -- '*.cpp' '*.h' '*.hpp' '*.ui' | grep -v '^src/3rdparty/' || true)

if [ "${#FILES[@]}" -eq 0 ]; then
    msg "错误: 没有找到可统计的源文件（请在仓库根目录运行）"
    exit 1
fi

# 统计某个 ERE 的命中行总数
count_lines() {
    local pat=$1
    grep -E -h -c "$pat" "${FILES[@]}" 2>/dev/null | awk '{ s += $1 } END { print s + 0 }'
}

# 统计命中文件数
count_files() {
    local pat=$1
    grep -E -l "$pat" "${FILES[@]}" 2>/dev/null | wc -l | tr -d ' '
}

# 每个文件的命中数（用于找出热点文件）
count_by_file() {
    local pat=$1
    grep -E -c "$pat" "${FILES[@]}" 2>/dev/null | awk -F: '$2 + 0 > 0'
}

msg "界面控件通用化盘点（基线: $(git rev-parse --short HEAD)）"
msg "统计文件数: ${#FILES[@]}（已排除 src/3rdparty/）"
hr

# ----------------------------------------------------------------------------
# 1. 界面类总数与基类分布
# ----------------------------------------------------------------------------
msg "[1] 界面类总数与基类分布"
TMP_CLASSES=$(mktemp)
grep -E -h -o 'class[[:space:]]+([A-Za-z_][A-Za-z0-9_]*[[:space:]]+)?[A-Za-z_][A-Za-z0-9_]*[[:space:]]*:[[:space:]]*public[[:space:]]+[A-Za-z_:][A-Za-z0-9_:]*' \
    "${FILES[@]}" 2>/dev/null \
    | sed -E 's/.*public[[:space:]]+//' > "$TMP_CLASSES" || true
TOTAL_CLASSES=$(wc -l < "$TMP_CLASSES" | tr -d ' ')
UI_CLASSES=$(grep -E -c "$UI_BASE_REGEX" "$TMP_CLASSES" 2>/dev/null || true)
msg "  全部 class: public <base> 声明: ${TOTAL_CLASSES}"
msg "  界面类（UI 基类）: ${UI_CLASSES}"
msg "  基类分布（Top 15）:"
sort "$TMP_CLASSES" 2>/dev/null | uniq -c | sort -rn | head -15 | sed 's/^/    /'
rm -f "$TMP_CLASSES"
hr

# ----------------------------------------------------------------------------
# 2. QMessageBox 调用
# ----------------------------------------------------------------------------
msg "[2] QMessageBox 调用"
MB_ALL=$(count_lines 'QMessageBox::(warning|question|information|critical)')
MB_FILES=$(count_files 'QMessageBox::(warning|question|information|critical)')
msg "  总计: ${MB_ALL} 处 / ${MB_FILES} 个文件"
for kind in warning question information critical; do
    n=$(count_lines "QMessageBox::$kind")
    f=$(count_files "QMessageBox::$kind")
    msg "    $kind: ${n} 处 / ${f} 个文件"
done

# 字段校验 vs 确认/提示（按文件标注）
VAL=0
OTHER=0
while IFS=: read -r f n; do
    [ -z "${f:-}" ] && continue
    if printf '%s' "$f" | grep -E -q "$VALIDATION_FILE_REGEX"; then
        VAL=$((VAL + n))
    else
        OTHER=$((OTHER + n))
    fi
done < <(count_by_file 'QMessageBox::(warning|question|information|critical)')
msg "  按文件归类: 字段校验 ${VAL} 处 / 确认·提示 ${OTHER} 处"
msg "  两个图表添加窗口（方案口径 25 处 = 13+12）:"
while IFS=: read -r f n; do
    [ -z "${f:-}" ] && continue
    if printf '%s' "$f" | grep -E -q "$TWO_WINDOWS_REGEX"; then
        msg "    ${f}: ${n}"
    fi
done < <(count_by_file 'QMessageBox::(warning|question|information|critical)')
msg "  自增浮点解析弹窗（should be zero after Wave 1）: $(count_lines 'floating-point arithmetic number') 处"
msg "  字段校验热点文件:"
count_by_file 'QMessageBox::(warning|question|information|critical)' \
    | grep -E "$VALIDATION_FILE_REGEX" \
    | sort -t: -k2,2nr | head -8 | sed 's/^/    /'
hr

# ----------------------------------------------------------------------------
# 3. QFormLayout
# ----------------------------------------------------------------------------
msg "[3] QFormLayout"
msg "  ${#FILES[@]} 个文件中: $(count_lines 'QFormLayout') 处 / $(count_files 'QFormLayout') 个文件"
hr

# ----------------------------------------------------------------------------
# 4. QFileDialog
# ----------------------------------------------------------------------------
msg "[4] QFileDialog"
msg "  总计: $(count_lines 'QFileDialog') 处 / $(count_files 'QFileDialog') 个文件"
msg "  静态便捷函数（getOpenFileName/getExistingDirectory/getSaveFileName 等）:"
count_by_file 'QFileDialog::get[A-Za-z]+' | sort -t: -k2,2nr | head -12 | sed 's/^/    /'
hr

# ----------------------------------------------------------------------------
# 5. 直接 include DAWidgets 头文件的项目文件
# ----------------------------------------------------------------------------
msg "[5] 直接 include DAWidgets 头文件的项目文件"
DW_HEADERS=$(cd src/3rdparty/DAWidgets/src 2>/dev/null && ls *.h 2>/dev/null || true)
if [ -z "$DW_HEADERS" ]; then
    msg "  （未找到 src/3rdparty/DAWidgets/src，跳过头文件名单）"
else
    DW_PATTERN=$(printf '%s\n' "$DW_HEADERS" | sed 's/\.h$//' | paste -sd'|' -)
    DW_INC="#include[[:space:]]*[<\"](${DW_PATTERN})\.h[>\"]"
    DW_FILES=$(count_files "$DW_INC")
    DW_LINES=$(count_lines "$DW_INC")
    msg "  ${DW_FILES} 个文件 / ${DW_LINES} 处 include（DAWidgets 公开头文件 ${DW_HEADERS//$'\n'/ }）"
    msg "  热点文件:"
    count_by_file "$DW_INC" | sort -t: -k2,2nr | head -12 | sed 's/^/    /'
fi
hr

# ----------------------------------------------------------------------------
# 6. 本方案关注的四类形态
# ----------------------------------------------------------------------------
msg "[6] 四类形态计数"

# 6a 自增组（.ui 中的 QGroupBox）
AUTOINC=$(count_lines 'name="groupBox[A-Za-z]*Autoincrement"')
AUTOINC_FILES=$(count_files 'name="groupBox[A-Za-z]*Autoincrement"')
LIVE_AUTOINC=0
LIVE_AUTOINC_FILES=0
while IFS=: read -r f n; do
    [ -z "${f:-}" ] && continue
    if ! printf '%s' "$f" | grep -E -q "$DEAD_UI_REGEX"; then
        LIVE_AUTOINC=$((LIVE_AUTOINC + n))
        LIVE_AUTOINC_FILES=$((LIVE_AUTOINC_FILES + 1))
    fi
done < <(count_by_file 'name="groupBox[A-Za-z]*Autoincrement"')
msg "  a) 自增 QGroupBox: ${AUTOINC} 个 / ${AUTOINC_FILES} 个 .ui 文件（live: ${LIVE_AUTOINC} 个 / ${LIVE_AUTOINC_FILES} 个；DAChartAddCurveWidget.ui 是未纳入构建的死文件）"
count_by_file 'name="groupBox[A-Za-z]*Autoincrement"' | sed 's/^/      /'

# 6b 平铺勾选列表（QListWidget + setCheckState / ItemIsUserCheckable）
CHK_FILES=0
CHK_HITS=""
for f in "${FILES[@]}"; do
    if grep -E -q 'QListWidget' "$f" 2>/dev/null \
        && grep -E -q 'setCheckState|ItemIsUserCheckable' "$f" 2>/dev/null; then
        CHK_FILES=$((CHK_FILES + 1))
        CHK_HITS+="$f"$'\n'
    fi
done
msg "  b) 平铺勾选列表（QListWidget + setCheckState/ItemIsUserCheckable）: ${CHK_FILES} 个文件"
printf '%s' "$CHK_HITS" | sed '/^$/d' | sed 's/^/      /'

# 6c 字符串列表 + 增删移（同一文件同时有 QListWidget 与按钮增删）
SLIST_FILES=0
SLIST_HITS=""
for f in "${FILES[@]}"; do
    if grep -E -q 'QListWidget' "$f" 2>/dev/null \
        && grep -E -q '(btnAdd|_btn_add|pushButtonAdd|toolButtonAdd|Add|Remove)' "$f" 2>/dev/null; then
        SLIST_FILES=$((SLIST_FILES + 1))
        SLIST_HITS+="$f"$'\n'
    fi
done
msg "  c) 手写字符串/对象列表 + 增删按钮: ${SLIST_FILES} 个文件"
printf '%s' "$SLIST_HITS" | sed '/^$/d' | head -20 | sed 's/^/      /'

# 6d 预览对话框右侧面板（QTableWidget + 预览标题的组合件候选）
PREVIEW_FILES=$(count_files 'QTableWidget')
msg "  d) 预览表格（QTableWidget）: ${PREVIEW_FILES} 个文件（含非预览用途，仅作趋势）"
hr

# ----------------------------------------------------------------------------
# 6.1 验收指标速查（方案 §6.1）
# ----------------------------------------------------------------------------
msg "[验收指标速查]"
msg "  手写 line edit + browse（QFileDialog 静态便捷函数）: $(count_lines 'QFileDialog::get[A-Za-z]+') 处"
msg "  DAFormEditorRegistry folder hack: $(count_lines 'findChild<[[:space:]]*QToolButton') 处"
msg "  自增 QGroupBox（live）: ${LIVE_AUTOINC_FILES} 个 .ui 文件 / ${LIVE_AUTOINC} 组"
