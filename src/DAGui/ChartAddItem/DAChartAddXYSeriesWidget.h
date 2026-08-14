#ifndef DACHARTADDXYSERIESWIDGET_H
#define DACHARTADDXYSERIESWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
// DAData
#include "DAData.h"
// DAUtil
#include "DAAutoincrementSeries.hpp"
// DAGui
#include "DAChartSeriesPickerWidget.h"

namespace Ui
{
class DAChartAddXYSeriesWidget;
}

namespace DA
{
class DAPySeriesTableModel;
class DADataManager;

/**
 * @brief 添加xy series，适用二维数据绘图的系列获取
 * 这个是一个abstract类，需要重写@sa createPlotItem
 */
class DAGUI_API DAChartAddXYSeriesWidget : public DAAbstractChartAddItemWidget
{
    Q_OBJECT
public:
    explicit DAChartAddXYSeriesWidget(QWidget* parent = nullptr);
    ~DAChartAddXYSeriesWidget();
    // 判断x是否是自增
    bool isXAutoincrement() const;
    // 判断y是否是自增
    bool isYAutoincrement() const;
    // 设置数据管理器
    virtual void setDataManager(DADataManager* dmgr) override;
    // 根据配置获取数据
    QVector< QPointF > getSeries() const;
    // 获取推荐的名字
    virtual QString getNameHint() const;
    // 设置当前的x到list中
    void setX(const DAData& dataframeData, const QString& seriesName);
    // 设置当前的y到list中
    void setY(const DAData& dataframeData, const QString& seriesName);
private Q_SLOTS:
    void onGroupBoxXAutoincrementClicked(bool on);
    void onGroupBoxYAutoincrementClicked(bool on);

    void onXSeriesChanged();
    void onYSeriesChanged();
    void updateTable();

    void onButtonXRemoveClicked();
    void onButtonYRemoveClicked();
    void onButtonXAddClicked();
    void onButtonYAddClicked();

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
    // 获取
    QPair< DAData, QString > getY() const;
    QPair< DAData, QString > getX() const;
    //
    DAPySeries getYSeries() const;
    DAPySeries getXSeries() const;

private:
    QPair< DAData, QString > getFirstValue(const QList< QPair< DAData, QStringList > >& datas) const;
    DAChartSeriesPickerWidget* ensurePicker(DAChartSeriesPickerWidget*& picker, DAChartSeriesPickerWidget::Role role);
    void showParentGuideDialog();
    void hideParentGuideDialog();

private:
    Ui::DAChartAddXYSeriesWidget* ui;
    DAChartSeriesPickerWidget* mPickerX { nullptr };
    DAChartSeriesPickerWidget* mPickerY { nullptr };
};
}
#endif  // DACHARTADDXYSERIESWIDGET_H
