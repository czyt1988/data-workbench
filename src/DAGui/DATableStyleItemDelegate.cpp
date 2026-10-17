#include "DATableStyleItemDelegate.h"
#include "DATableStyleManager.h"
#include "DATableHighlightOverlay.h"
#include "Models/DAAbstractCacheWindowTableModel.h"
#include <QPainter>
#include <QPalette>

namespace DA
{

namespace
{
// 瞬态高亮覆盖背景的不透明度：语义色以半透明叠在①层静态样式之上，
// 保证黑/白前景文字仍可读（混合规则集中在 delegate 层③，overlay 只给不透明语义色）
constexpr int c_highlightAlpha = 160;
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
    if (noStyle && noOverlay) {
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

    // ② 持久装饰（条件格式）插入点：003 在此按列缓存的规则列表求值，
    //    命中后叠加背景/前景（在①之后、③之前），不保留独立虚钩子（D2）

    // ③ 瞬态装饰：查找命中/校验违规，语义色半透明覆盖背景
    if (mOverlay) {
        const DATableHighlightOverlay::Level level = mOverlay->levelAt(actualRow, actualCol);
        if (level != DATableHighlightOverlay::Level::None) {
            QColor hlColor = DATableHighlightOverlay::colorForLevel(level);
            hlColor.setAlpha(c_highlightAlpha);
            painter->fillRect(opt.rect, hlColor);
        }
    }

    // ④ 数据条插入点：003 在背景之上、文字之下绘制（按列 min/max 归一化条长，
    //    文字层用去掉背景的方式复用基类绘制，避免重写文字绘制）

    // ⑤ 绘制文字（基类 CE_ItemViewItem 路径）；批注角标插入点：005 在本行之后
    //    对有批注的单元格画右上角小三角（与数据条/命中色互不影响）
    QStyledItemDelegate::paint(painter, opt, index);
}

}  // end of namespace DA
