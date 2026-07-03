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
    // 注意：此 hook 在父类 paint 之前调用，适合修改 opt（如 backgroundBrush）的格式。
    // 若需直接绘制（如数据条）并显示在背景之上/文字之下，子类需自行处理分层绘制
    // （例如重写整个 paint，先调父类画背景，再画数据条，再画文字）。
    virtual void paintConditionalFormat(QPainter* painter, QStyleOptionViewItem& opt,
                                        const QModelIndex& index, int actualRow, int actualCol) const;

private:
    DATableStyleManager* mStyleManager;  // 非拥有
};
}  // end of namespace DA
#endif  // DATABLESTYLEITEMDELEGATE_H
