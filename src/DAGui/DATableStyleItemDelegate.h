#ifndef DATABLESTYLEITEMDELEGATE_H
#define DATABLESTYLEITEMDELEGATE_H
#include "DAGuiAPI.h"
#include <QStyledItemDelegate>

namespace DA
{
class DATableStyleManager;

/**
 * @brief 表格样式自定义 delegate
 *
 * paint() 从 manager 查 resolveCellStyle 叠加到 QStyleOptionViewItem 后调父类 paint。
 * 预留 paintConditionalFormat 虚函数（空实现）供后期条件格式（数据条等）覆写。
 */
class DAGUI_API DATableStyleItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    DATableStyleItemDelegate(DATableStyleManager* mgr, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

protected:
    // 预留：条件格式扩展点，后期在子类覆写实现数据条/色阶等基于规则的绘制
    virtual void paintConditionalFormat(QPainter* painter, QStyleOptionViewItem& opt,
                                        const QModelIndex& index, int actualRow, int actualCol) const;

private:
    DATableStyleManager* mStyleManager;  // 非拥有
};
}  // end of namespace DA
#endif  // DATABLESTYLEITEMDELEGATE_H
