#ifndef DAABSTRACTPLOTITEMSETTINGWIDGET_H
#define DAABSTRACTPLOTITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
namespace QIM
{
class QImPlotItemNode;
}


namespace DA
{
/**
 * @brief chart设置的基类封装了基本操作
 */
class DAGUI_API DAAbstractPlotItemSettingWidget : public QWidget
{
    Q_OBJECT
public:
    DAAbstractPlotItemSettingWidget(QWidget* parent = nullptr);
    ~DAAbstractPlotItemSettingWidget();
    // 设置plotitem
    void setPlotItem(QIM::QImPlotItemNode* item);
    QIM::QImPlotItemNode* getPlotItem() const;
    // 判断是否有item
    bool isHaveItem() const;
    // 判断当前item是否是对应的类型，如果没有item也返回false
    bool checkItemType(int type) const;

    // setPlotItem之后调用的虚函数,通过重写此函数可以执行一些设置item之后的界面更新
    virtual void updateUI(QIM::QImPlotItemNode* item);

protected:
    QPointer< QIM::QImPlotItemNode > mPlotItem { nullptr };
};
}  // end DA

#endif  // DAABSTRACTCHARTITEMSETTINGWIDGET_H
