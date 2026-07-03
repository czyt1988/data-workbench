#include "DATableStyleItemDelegate.h"
#include "DATableStyleManager.h"
#include "Models/DAAbstractCacheWindowTableModel.h"
#include <QPainter>
#include <QPalette>

namespace DA
{

/**
 * @brief 构造函数
 * @param mgr 样式管理器（非拥有）
 * @param parent 父对象
 */
DATableStyleItemDelegate::DATableStyleItemDelegate(DATableStyleManager* mgr, QObject* parent)
    : QStyledItemDelegate(parent), mStyleManager(mgr)
{
}

/**
 * @brief 绘制单元格
 *
 * 快速路径：manager 为空时直接走父类。
 * 否则把 logical row 转为 actualRow（经 getCacheWindowStartRow 偏移），
 * 从 manager 查 resolveCellStyle，叠加到 QStyleOptionViewItem 后调父类 paint。
 * @param painter 画笔
 * @param option 样式选项
 * @param index 模型索引
 */
void DATableStyleItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    if (!mStyleManager || mStyleManager->isEmpty()) {
        // 快速路径：无样式走默认
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

    DATableCellStyle style = mStyleManager->resolveCellStyle(actualRow, actualCol);
    if (!style.isNull()) {
        if (style.backgroundValid()) {
            opt.backgroundBrush = style.background();
        }
        if (style.foregroundValid()) {
            opt.palette.setColor(QPalette::Text, style.foreground());
        }
        if (style.fontValid()) {
            opt.font = style.font();
            opt.fontMetrics = QFontMetrics(style.font());
        }
    }

    // 预留条件格式扩展点
    paintConditionalFormat(painter, opt, index, actualRow, actualCol);

    QStyledItemDelegate::paint(painter, opt, index);
}

/**
 * @brief 条件格式扩展点（空实现）
 *
 * 后期在子类覆写实现数据条/色阶等基于规则的绘制。
 * @param painter 画笔
 * @param opt 样式选项（可修改）
 * @param index 模型索引
 * @param actualRow 真实行号
 * @param actualCol 真实列号
 */
void DATableStyleItemDelegate::paintConditionalFormat(QPainter* painter, QStyleOptionViewItem& opt,
                                                      const QModelIndex& index, int actualRow, int actualCol) const
{
    Q_UNUSED(painter);
    Q_UNUSED(opt);
    Q_UNUSED(index);
    Q_UNUSED(actualRow);
    Q_UNUSED(actualCol);
    // 第一版空实现，后期条件格式在子类覆写
}

}  // end of namespace DA
