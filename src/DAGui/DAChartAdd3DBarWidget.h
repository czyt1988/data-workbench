#ifndef DACHARTADD3DBARWIDGET_H
#define DACHARTADD3DBARWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChart3DAddItemWidget.h"
#include "DAData.h"
#include "qwt_point_3d.h"
namespace Ui
{
class DAChartAdd3DBarWidget;
}
namespace DA
{
class DADataManager;

// 3D柱状图数据选择widget
class DAGUI_API DAChartAdd3DBarWidget : public DAAbstractChart3DAddItemWidget
{
    Q_OBJECT
public:
    // 数据模式
    enum BarDataMode
    {
        Series1D,  ///< 1D series模式：选一列值
        Grid2D     ///< 2D grid模式：选DataFrame作为z矩阵
    };
    Q_ENUM(BarDataMode)

public:
    explicit DAChartAdd3DBarWidget(QWidget* parent = nullptr);
    ~DAChartAdd3DBarWidget();
    // 设置datamanager
    virtual void setDataManager(DADataManager* dmgr) override;
    // 创建3D柱状图item
    virtual Qwt3DPlotItem* create3DPlotItem() override;
    // 获取当前数据模式
    BarDataMode getDataMode() const;
    // 获取推荐的名称
    QString getNameHint() const;

private Q_SLOTS:
    void onComboBoxModeCurrentIndexChanged(int index);
    void onComboBoxDataframeCurrentIndexChanged(int index);
    void onComboBoxSeriesCurrentIndexChanged(int index);

private:
    // 从单列提取1D series数据
    bool extractSeriesData(QVector< QwtPoint3D >& points) const;
    // 从DataFrame提取2D grid数据
    bool extractGridData(QVector< double >& xCoords,
                         QVector< double >& yCoords,
                         QVector< QVector< double > >& zMatrix) const;
    // 刷新DataFrame下拉框
    void refreshDataframeCombo();
    // 刷新列下拉框
    void refreshSeriesCombo();
    // 获取当前选中的DataFrame
    DAData getCurrentDataframe() const;

private:
    Ui::DAChartAdd3DBarWidget* ui;
    DADataManager* mDataMgr { nullptr };
};
}  // end DA
#endif  // DACHARTADD3DBARWIDGET_H
