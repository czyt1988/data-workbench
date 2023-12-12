#ifndef DAPENSTYLECOMBOBOX_H
#define DAPENSTYLECOMBOBOX_H
#include "DACommonWidgetsAPI.h"
#include <QComboBox>
#include <QAbstractItemDelegate>
#define DAPENSTYLECOMBOBOX_USE_DELEGATE 0

namespace DA
{

/**
 * @brief Qt::PenStyle for QComboBox
 */
class DACOMMONWIDGETS_API DAPenStyleComboBox : public QComboBox
{
    Q_OBJECT
#if !DAPENSTYLECOMBOBOX_USE_DELEGATE
    DA_DECLARE_PRIVATE(DAPenStyleComboBox)
#endif
public:
    DAPenStyleComboBox(QWidget* parent = Q_NULLPTR);
    ~DAPenStyleComboBox();

    // 画笔样式转换为字符串
    static QString penStyleToString(Qt::PenStyle s);

    // 生成icon
    QIcon generatePenIcon(Qt::PenStyle s) const;
    // 绘制画笔
    static void drawPenStyle(QPainter* painter, const QRect& rect, const QPen& pen);
    // 刷新所有item
    void updateItems();
    // 是否在样式上显示文字
    void setStyleTextVisible(bool on);
    bool isStyleTextVisible() const;
    // 重建所有items
    void rebuildItems();
public slots:
    // 设置画笔
    void setPen(const QPen& p);
    // 设置绘制的线条的颜色
    void setPenColor(const QColor& c);
    // 设置绘制的画笔宽度
    void setPenLineWidth(int w);
    // 设置当前的画笔样式
    void setCurrentPenStyle(Qt::PenStyle s);

protected:
    // 添加item
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

}  // namespace DA
#endif  // DAPENSTYLECOMBOBOX_H
