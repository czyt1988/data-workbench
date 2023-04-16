#ifndef DABRUSHSTYLECOMBOBOX_H
#define DABRUSHSTYLECOMBOBOX_H
#include "DACommonWidgetsAPI.h"
#include <QComboBox>
#include <QAbstractItemDelegate>
#include <QBrush>
#include <QGradient>
#include <QPixmap>
namespace DA
{
/**
 * @brief Qt::BrushStyle for QComboBox
 * @todo 目前仅实现常规填充，渐变的还未实现
 */
class DACOMMONWIDGETS_API DABrushStyleComboBox : public QComboBox
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DABrushStyleComboBox)
public:
    //默认构造以ShowNoBrushStyle|ShowPaintStyle构造，基本颜色为黑色
    DABrushStyleComboBox(QWidget* parent = Q_NULLPTR);
    ~DABrushStyleComboBox();
    //画笔样式转换为字符串
    static QString brushStyleToString(Qt::BrushStyle s);
    //显示无画刷
    void setShowNoBrushStyle(bool v);
    bool isShowNoBrushStyle() const;
    //设置画刷颜色，此设置会影响ShowPaintStyle
    bool isColorBrush() const;
    void setBrushColor(const QColor& v);
    QColor getBrushColor() const;
    //重建item
    void rebuildItems();
    //生成icon
    QIcon generateBrushStyleIcon(Qt::BrushStyle s);
    //更新icon
    void updateIcons();
public slots:
    //设置当前的样式
    void setCurrentBrushStyle(Qt::BrushStyle s);
signals:
    /**
     * @brief 画刷样式发生变更
     * @param s
     */
    void currentBrushStyleChanged(Qt::BrushStyle s);
private slots:
    //原信号对应的槽
    void onCurrentIndexChanged(int index);

protected:
    void addItem(Qt::BrushStyle s);

private:
};
}

#endif  // DABRUSHSTYLECOMBOBOX_H
