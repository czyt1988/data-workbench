#ifndef DAWORDWRAPITEMDELEGATE_H
#define DAWORDWRAPITEMDELEGATE_H
#include <QStyledItemDelegate>
namespace DA
{
class DAWordWrapItemDelegate : public QStyledItemDelegate
{
public:
    /**
     * @brief 换行模式
     */
    enum WrapMode
    {
        NoWrap,       ///< 不换行
        Wrap,         ///<有换行符时换行(默认)
        WrapAnywhere  ///< 无论如何无法显示完整就换行
    };

public:
    DAWordWrapItemDelegate(QObject* parent = nullptr);
    //设置换行模式
    void setWrapMode(WrapMode m);
    WrapMode getWrapMode() const;
    //绘制
    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    WrapMode mWrapMode;
};
}

#endif  // DAWORDWRAPITEMDELEGATE_H
