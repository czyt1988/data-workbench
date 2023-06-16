#ifndef DACHARTADDXYSERIESWIDGET_H
#define DACHARTADDXYSERIESWIDGET_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DAAbstractChartAddItemWidget.h"

namespace Ui
{
class DAChartAddXYSeriesWidget;
}

namespace DA
{

class DADataManager;
DA_IMPL_FORWARD_DECL(DAChartAddXYSeriesWidget)
/**
 * @brief 添加xyseries，适用QwtPlotCurve
 */
class DAGUI_API DAChartAddXYSeriesWidget : public DAAbstractChartAddItemWidget
{
    Q_OBJECT
    DA_IMPL(DAChartAddXYSeriesWidget)
public:
    explicit DAChartAddXYSeriesWidget(QWidget* parent = nullptr);
    ~DAChartAddXYSeriesWidget();
    //
    void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
    //判断x是否是自增
    bool isXAutoincrement() const;
    //判断y是否是自增
    bool isYAutoincrement() const;
    //接口实现
    virtual QwtPlotItem* createPlotItem() override;

private slots:
    void onComboBoxXCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onComboBoxYCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onGroupBoxXAutoincrementClicked(bool on);
    void onGroupBoxYAutoincrementClicked(bool on);

protected:
    //获取为vector pointf
    bool getToVectorPointF(QVector< QPointF >& res);
    //尝试获取x值得自增内容
    bool tryGetXSelfInc(double& base, double& step);
    bool tryGetYSelfInc(double& base, double& step);

private:
    Ui::DAChartAddXYSeriesWidget* ui;
};
}
#endif  // DACHARTADDXYSERIESWIDGET_H
