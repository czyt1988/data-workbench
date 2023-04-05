#ifndef DAPENSTYLECOMBOBOX_H
#define DAPENSTYLECOMBOBOX_H
#include "DACommonWidgetsAPI.h"
#include <QComboBox>
#include <QAbstractItemDelegate>
#define DAPENSTYLECOMBOBOX_USE_DELEGATE 0

namespace DA
{
#if !DAPENSTYLECOMBOBOX_USE_DELEGATE
DA_IMPL_FORWARD_DECL(DAPenStyleComboBox)
#endif
/**
 * @brief Qt::PenStyle for QComboBox
 */
class DACOMMONWIDGETS_API DAPenStyleComboBox : public QComboBox
{
    Q_OBJECT
#if !DAPENSTYLECOMBOBOX_USE_DELEGATE
    DA_IMPL(DAPenStyleComboBox)
#endif
public:
    DAPenStyleComboBox(QWidget* parent = Q_NULLPTR);
    ~DAPenStyleComboBox();
    //画笔样式转换为字符串
    static QString penStyleToString(Qt::PenStyle s);
    //设置绘制的线条的颜色
    void setPenColor(const QColor& c);
    //设置绘制的画笔宽度
    void setPenLineWidth(int w);
    //生成icon
    QIcon generatePenIcon(Qt::PenStyle s) const;
    //绘制画笔
    static void drawPenStyle(QPainter* painter, const QRect& rect, const QPen& pen);
    //刷新所有item
    void updateItems();
    //添加item
    void addItem(Qt::PenStyle s);
private slots:
    void onCurrentIndexChanged(int index);
signals:
    /**
     * @brief 画笔样式改变
     * @param s
     */
    void currentPenStyleChanged(Qt::PenStyle s);
};
#if DAPENSTYLECOMBOBOX_USE_DELEGATE
/**
 * @brief PenStyleCombox绘图代理
 */
class DAPenStyleComboBoxItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    DAPenStyleComboBoxItemDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    //设置绘制的线条的颜色
    void setPenColor(const QColor& c);
    QColor getPenColor() const;
    //设置绘制的画笔宽度
    void setPenLineWidth(int w);
    int getPenLineWidth() const;

private:
    void drawBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void drawFocus(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
    QColor _penColor;
    int _penWidth;
};
#endif
}  // namespace DA
#endif  // DAPENSTYLECOMBOBOX_H
