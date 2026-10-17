#include "DATableStyleItemDelegate.h"
#include "DATableStyleManager.h"
#include "DATableHighlightOverlay.h"
#include "DATableConditionalFormatStore.h"
#include "DATableValidationStore.h"
#include "Models/DAAbstractCacheWindowTableModel.h"
#include <QPainter>
#include <QPalette>
#include <QComboBox>
#include <QStringList>

namespace DA
{

namespace
{
// 瞬态高亮覆盖背景的不透明度：语义色以半透明叠在①层静态样式之上，
// 保证黑/白前景文字仍可读（混合规则集中在 delegate 层③，overlay 只给不透明语义色）
constexpr int c_highlightAlpha = 160;
// 数据条与单元格内容区边缘的内边距（§1.4-1：条与文字之间留 2px）
constexpr int c_dataBarPadding = 2;
}

/**
 * @brief 构造函数
 * @param mgr 样式管理器（非拥有，可空）
 * @param overlay 瞬态高亮层（非拥有，可空）
 * @param parent 父对象
 */
DATableStyleItemDelegate::DATableStyleItemDelegate(DATableStyleManager* mgr, DATableHighlightOverlay* overlay, QObject* parent)
    : QStyledItemDelegate(parent), mStyleManager(mgr), mOverlay(overlay)
{
}

/**
 * @brief 注入条件格式子存储（②持久装饰 + ④数据条）
 * @param store 条件格式子存储（非拥有，可空；空则跳过②④层）
 */
void DATableStyleItemDelegate::setConditionalFormatStore(DATableConditionalFormatStore* store)
{
    mCfStore = store;
}

/**
 * @brief 注入校验规则子存储（枚举列下拉编辑器，005 T1.25）
 * @param store 校验规则子存储（非拥有，可空；空则枚举列走基类默认编辑器）
 */
void DATableStyleItemDelegate::setValidationStore(DATableValidationStore* store)
{
    mValidationStore = store;
}

/**
 * @brief 创建编辑器：枚举规则列 → QComboBox（005 T1.25 §4.3-6）
 *
 * 列名经模型表头 DisplayRole 取（位置索引即逻辑列，本表无列重排），occurrence 由
 * store 按「第 n 个同名列」消歧（R20）。editable = 规则的「允许其他值」开关；关闭时
 * 只能从列表取值。只读门禁在模型 flags（视图不会为不可编辑索引调用本函数，红线4：
 * 此处不另造判定）。无枚举规则/无 store 走基类默认编辑器。
 * @param parent 编辑器父对象
 * @param option 样式选项
 * @param index 模型索引（窗口行；枚举查询只用列维，无需行换算）
 * @return 编辑器 widget
 */
QWidget* DATableStyleItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (mValidationStore && index.isValid()) {
        const QAbstractItemModel* m = index.model();
        const int col               = index.column();
        if (m && col >= 0 && col < m->columnCount()) {
            QStringList names;
            names.reserve(col + 1);
            for (int c = 0; c <= col; ++c) {
                names.append(m->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString());
            }
            QStringList values;
            bool allowOther = true;
            if (mValidationStore->enumOptionsForColumn(names, col, &values, &allowOther)) {
                QComboBox* combo = new QComboBox(parent);
                combo->setEditable(allowOther);
                combo->addItems(values);
                // 当前格值预填（editable 时保留原文；非 editable 且值不在列表则保持首项，
                // 随后视图的 setEditorData 走 USER 属性同口径覆盖）
                combo->setCurrentText(index.data(Qt::EditRole).toString());
                return combo;
            }
        }
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

/**
 * @brief 绘制单元格（五层装饰管线，层序钉死，勿调换）
 *
 * ① 静态样式：DATableStyleManager 三层 merge（列→行→单元格）后的 bg/fg/font 叠加；
 * ② 持久装饰（条件格式）：【插入点，003 在此求值规则并叠加背景/前景，勿另开虚钩子】；
 * ③ 瞬态装饰：DATableHighlightOverlay 查找命中/校验违规，半透明覆盖背景；
 * ④ 数据条：【插入点，003 在背景之上、文字之下按列 min/max 归一化画条】；
 * ⑤ 绘制文字：沿用基类 QStyle::drawControl(CE_ItemViewItem) 路径；
 *    批注角标【插入点，005 在文字之后画右上角小三角】。
 *
 * 快速路径：样式管理器为空且高亮层无内容 → 直接走父类（10 万行滚动性能保障）。
 * 样式与高亮查询一律用 actualRow（getCacheWindowStartRow 偏移后的数据源绝对行）。
 * @param painter 画笔
 * @param option 样式选项
 * @param index 模型索引
 */
void DATableStyleItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    const bool noStyle   = (!mStyleManager || mStyleManager->isEmpty());
    const bool noOverlay = (!mOverlay || mOverlay->isEmpty());
    const bool noCf      = (!mCfStore || mCfStore->isEmpty());
    if (noStyle && noOverlay && noCf) {
        // 快速路径：无任何装饰走默认
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    // logical row → actualRow（与 DAAbstractCacheWindowTableModel::data() 同样的偏移）
    const DAAbstractCacheWindowTableModel* cacheModel =
        qobject_cast< const DAAbstractCacheWindowTableModel* >(index.model());
    int actualRow = index.row();
    int actualCol = index.column();
    if (cacheModel) {
        actualRow = cacheModel->getCacheWindowStartRow() + index.row();
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);  // 先让父类填充文本/默认字体

    // ① 静态样式（原三层 merge 叠加逻辑，行为与改造前一致）
    if (mStyleManager) {
        DATableCellStyle style = mStyleManager->resolveCellStyle(actualRow, actualCol);
        if (!style.isNull()) {
            if (style.backgroundValid()) {
                // 直接用 painter 填充背景色，不依赖 backgroundBrush（Qt style 在某些情况下
                // 不使用 backgroundBrush 绘制背景，fillRect 更可靠）
                painter->fillRect(opt.rect, style.background());
            }
            if (style.foregroundValid()) {
                opt.palette.setColor(QPalette::Text, style.foreground());
            }
            if (style.fontValid()) {
                opt.font = style.font();
                opt.fontMetrics = QFontMetrics(style.font());
            }
        }
    }

    // ② 持久装饰（条件格式，003）：按列缓存的规则桶求值（红线1：只在 delegate 层，
    //    取值 Qt::EditRole 原始值；红线2：只查本列桶，统计走 store 惰性缓存），
    //    命中后在①结果之上叠加背景/前景/字体（复用 DATableCellStyle 叠加语义，
    //    与①同一代码路径，不另造颜色合并逻辑），列表顺序=优先级、后者覆盖前者
    DATableConditionalFormatStore::EvaluateResult cfResult;
    if (mCfStore) {
        const QVector< DATableConditionalFormatRule >* cfRules = mCfStore->rulesForColumn(actualCol);
        if (cfRules && !cfRules->isEmpty()) {
            DATableConditionalFormatStore::ColumnStats stats;
            bool statsValid = false;
            if (mCfStore->needsColumnStats(actualCol)) {
                // 色阶/数据条才触发统计（惰性计算 + 缓存；纯单元格规则的列不打 Python）
                statsValid = mCfStore->columnStats(actualCol, stats);
            }
            cfResult = DATableConditionalFormatStore::evaluate(*cfRules, index.data(Qt::EditRole), statsValid, stats);
            const DATableCellStyle& cfStyle = cfResult.style;
            if (!cfStyle.isNull()) {
                if (cfStyle.backgroundValid()) {
                    painter->fillRect(opt.rect, cfStyle.background());
                }
                if (cfStyle.foregroundValid()) {
                    opt.palette.setColor(QPalette::Text, cfStyle.foreground());
                }
                if (cfStyle.fontValid()) {
                    opt.font        = cfStyle.font();
                    opt.fontMetrics = QFontMetrics(cfStyle.font());
                }
            }
        }
    }

    // ③ 瞬态装饰：查找命中/校验违规，语义色半透明覆盖背景（③覆盖②，层序钉死）
    if (mOverlay) {
        const DATableHighlightOverlay::Level level = mOverlay->levelAt(actualRow, actualCol);
        if (level != DATableHighlightOverlay::Level::None) {
            QColor hlColor = DATableHighlightOverlay::colorForLevel(level);
            hlColor.setAlpha(c_highlightAlpha);
            painter->fillRect(opt.rect, hlColor);
        }
    }

    // ④ 数据条（003）：背景之上、文字之下（§1.4-1：条长=(v-min)/(max-min) 归一化占
    //    内容区宽度，max==min 退化半格，2px 内边距，不改文字排布）。
    //    选用 painter->fillRect 而非 QStyleOptionProgressBar：progressBar 会带 groove/chunk
    //    样式皮肤且几何不可控，fillRect 与①③层的背景画法一致、几何精确可控
    if (cfResult.hasDataBar) {
        QColor barColor = cfResult.barColor.isValid() ? cfResult.barColor : QColor(0xBD, 0xCD, 0xE8);
        const QRect barArea = opt.rect.adjusted(c_dataBarPadding, c_dataBarPadding, -c_dataBarPadding, -c_dataBarPadding);
        const int barWidth  = qRound(barArea.width() * qBound(0.0, cfResult.barRatio, 1.0));
        if (barWidth > 0 && barArea.width() > 0 && barArea.height() > 0) {
            painter->fillRect(QRect(barArea.left(), barArea.top(), barWidth, barArea.height()), barColor);
        }
    }

    // ⑤ 绘制文字（基类 CE_ItemViewItem 路径，数据条在文字之下不遮字）；
    //    批注角标插入点：005 在本行之后对有批注的单元格画右上角小三角（与数据条/命中色互不影响）
    QStyledItemDelegate::paint(painter, opt, index);
}

}  // end of namespace DA
