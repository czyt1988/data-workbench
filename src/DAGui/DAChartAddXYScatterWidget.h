#ifndef DACHARTADDXYSCATTERWIDGET_H
#define DACHARTADDXYSCATTERWIDGET_H
#include "DAGuiAPI.h"
// DAData
#include "DAData.h"
// DAUtil
#include "DAAutoincrementSeries.h"
// DAGui
#include "DAAbstractChartAddItemWidget.h"
// qwt
class QwtSymbol;

namespace Ui
{
class DAChartAddXYScatterWidget;
}

namespace DA
{

class DADataManager;
DA_IMPL_FORWARD_DECL(DAChartAddXYScatterWidget)
/**
 * @brief 添加xy scatter，适用QwtPlotCurve
 */
class DAGUI_API DAChartAddXYScatterWidget : public DAAbstractChartAddItemWidget
{
    Q_OBJECT
    DA_IMPL(DAChartAddXYScatterWidget)
public:
    explicit DAChartAddXYScatterWidget(QWidget* parent = nullptr);
    ~DAChartAddXYScatterWidget();
    //
    void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
    // 判断x是否是自增
    bool isXAutoincrement() const;
    // 判断y是否是自增
    bool isYAutoincrement() const;
    // 接口实现
    virtual QwtPlotItem* createPlotItem() override;
    // 注意，此函数是工程函数
    QwtSymbol* createSymbol() const;
private slots:
    void onComboBoxXCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onComboBoxYCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onGroupBoxXAutoincrementClicked(bool on);
    void onGroupBoxYAutoincrementClicked(bool on);

protected:
    // 获取x自增
    bool getXAutoIncFromUI(DAAutoincrementSeries< double >& v);
    // 获取y自增
    bool getYAutoIncFromUI(DAAutoincrementSeries< double >& v);
    // 获取为vector pointf
    bool getToVectorPointFFromUI(QVector< QPointF >& res);
    // 尝试获取x值得自增内容
    bool tryGetXSelfInc(double& base, double& step);
    bool tryGetYSelfInc(double& base, double& step);

private:
    Ui::DAChartAddXYScatterWidget* ui;
};
}
#endif  // DACHARTADDXYSCATTERWIDGET_H
