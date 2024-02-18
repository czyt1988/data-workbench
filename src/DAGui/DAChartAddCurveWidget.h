#ifndef DACHARTADDCURVEWIDGET_H
#define DACHARTADDCURVEWIDGET_H
#include <QWidget>
#include "DAAbstractChartAddItemWidget.h"
#include "DAData.h"
#include "DADataManager.h"
class QAbstractButton;
namespace Ui
{
class DAChartAddCurveWidget;
}

namespace DA
{
/**
 * @brief 添加曲线
 */
class DAChartAddCurveWidget : public DAAbstractChartAddItemWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartAddCurveWidget)
    void init();

public:
    explicit DAChartAddCurveWidget(QWidget* parent = nullptr);
    ~DAChartAddCurveWidget();
    virtual QwtPlotItem* createPlotItem() override;
    // 设置datafram
    void setCurrentData(const DAData& d);
    DAData getCurrentData() const;
    // 设置datamanager,会把combox填入所有的dataframe
    void setDataManager(DADataManager* dmgr);
    // 设置仅仅只有symbol，此时会把plot类型设置为no curve，把symbol 勾上
    void setScatterMode(bool on);

private slots:
    // 顶部导航按钮点击槽
    void onNavButtonClicked(QAbstractButton* button);

private:
    Ui::DAChartAddCurveWidget* ui;
};
}

#endif  // DACHARTCREATECURVEWIDGET_H
