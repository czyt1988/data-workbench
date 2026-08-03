#ifndef DACHARTADD3DSURFACEWIDGET_H
#define DACHARTADD3DSURFACEWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChart3DAddItemWidget.h"
#include "DAData.h"
namespace Ui
{
class DAChartAdd3DSurfaceWidget;
}
namespace DA
{
class DADataManager;

// 3D曲面图数据选择widget
class DAGUI_API DAChartAdd3DSurfaceWidget : public DAAbstractChart3DAddItemWidget
{
    Q_OBJECT
public:
    // 数据模式
    enum SurfaceDataMode
    {
        GridMode,    ///< 规则网格模式：选DataFrame作为z矩阵
        ScatterMode  ///< 散点插值模式：选x/y/z三列散点数据
    };
    Q_ENUM(SurfaceDataMode)

public:
    explicit DAChartAdd3DSurfaceWidget(QWidget* parent = nullptr);
    ~DAChartAdd3DSurfaceWidget();
    // 设置datamanager
    virtual void setDataManager(DADataManager* dmgr) override;
    // 创建3D曲面item
    virtual Qwt3DPlotItem* create3DPlotItem() override;
    // 获取当前数据模式
    SurfaceDataMode getDataMode() const;
    // 获取推荐的名称
    QString getNameHint() const;

private Q_SLOTS:
    void onComboBoxModeCurrentIndexChanged(int index);
    void onComboBoxDataframeCurrentIndexChanged(int index);
    void onComboBoxXCurrentIndexChanged(int index);
    void onComboBoxYCurrentIndexChanged(int index);
    void onComboBoxZCurrentIndexChanged(int index);

private:
    // 从DataFrame提取二维矩阵数据（规则网格模式）
    bool extractGridData(QVector< double >& xCoords,
                         QVector< double >& yCoords,
                         QVector< QVector< double > >& zMatrix) const;
    // 从三列散点数据插值为规则网格（散点插值模式）
    bool extractScatterDataAndInterpolate(QVector< double >& xCoords,
                                           QVector< double >& yCoords,
                                           QVector< QVector< double > >& zMatrix) const;
    // 刷新DataFrame下拉框
    void refreshDataframeCombo();
    // 刷新列下拉框（散点模式）
    void refreshColumnCombos();
    // 获取当前选中的DataFrame
    DAData getCurrentDataframe() const;

private:
    Ui::DAChartAdd3DSurfaceWidget* ui;
    DADataManager* mDataMgr { nullptr };
};
}  // end DA
#endif  // DACHARTADD3DSURFACEWIDGET_H
