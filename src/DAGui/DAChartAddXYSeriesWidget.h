#ifndef DACHARTADDXYSERIESWIDGET_H
#define DACHARTADDXYSERIESWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
// DAData
#include "DAData.h"
// DAUtil
#include "DAAutoincrementSeries.h"
// DAGui
namespace Ui
{
class DAChartAddXYSeriesWidget;
}

namespace DA
{

class DADataManager;

/**
 * @brief 添加xy series，适用二维数据绘图的系列获取
 */
class DAGUI_API DAChartAddXYSeriesWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartAddXYSeriesWidget)
public:
    explicit DAChartAddXYSeriesWidget(QWidget* parent = nullptr);
    ~DAChartAddXYSeriesWidget();
    //
    void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
    // 判断x是否是自增
    bool isXAutoincrement() const;
    // 判断y是否是自增
    bool isYAutoincrement() const;
    // 根据配置获取数据
    QVector< QPointF > getSeries() const;
    // 设置datafram
    void setCurrentData(const DAData& d);
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
    Ui::DAChartAddXYSeriesWidget* ui;
};
}
#endif  // DACHARTADDXYSERIESWIDGET_H
