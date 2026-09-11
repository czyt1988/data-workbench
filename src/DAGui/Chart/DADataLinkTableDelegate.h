#ifndef DADATALINKTABLEDELEGATE_H
#define DADATALINKTABLEDELEGATE_H
#include <QStyledItemDelegate>
#include "DAGuiAPI.h"

namespace DA
{
/**
 * @brief 数据联动表的单元格绘制代理
 *
 * 在曲线名单元格左侧绘制与曲线同色的色块
 */
class DAGUI_API DADataLinkTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DADataLinkTableDelegate(QObject* parent = nullptr);
    ~DADataLinkTableDelegate() override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    static constexpr int s_colorBlockSize   = 12;
    static constexpr int s_colorBlockMargin = 4;
};
}  // namespace DA
#endif  // DADATALINKTABLEDELEGATE_H
