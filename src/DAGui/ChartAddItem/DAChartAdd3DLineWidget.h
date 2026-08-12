#ifndef DACHARTADD3DLINEWIDGET_H
#define DACHARTADD3DLINEWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChart3DAddItemWidget.h"
#include "DAData.h"
#include "qwt_point_3d.h"
namespace Ui
{
class DAChartAdd3DLineWidget;
}
namespace DA
{
class DADataManager;

// 3D线图数据选择widget
class DAGUI_API DAChartAdd3DLineWidget : public DAAbstractChart3DAddItemWidget
{
    Q_OBJECT
public:
    explicit DAChartAdd3DLineWidget(QWidget* parent = nullptr);
    ~DAChartAdd3DLineWidget();
    // 设置datamanager
    virtual void setDataManager(DADataManager* dmgr) override;
    // 创建3D线图item
    virtual Qwt3DPlotItem* create3DPlotItem() override;
    // 获取推荐的名称
    QString getNameHint() const;

private Q_SLOTS:
    void onComboBoxDataframeCurrentIndexChanged(int index);

private:
    // 从三列提取3D轨迹数据
    bool extractLineData(QVector< QwtPoint3D >& points) const;
    // 刷新DataFrame下拉框
    void refreshDataframeCombo();
    // 刷新列下拉框
    void refreshColumnCombos();
    // 获取当前选中的DataFrame
    DAData getCurrentDataframe() const;

private:
    Ui::DAChartAdd3DLineWidget* ui;
    DADataManager* mDataMgr { nullptr };
};
}  // end DA
#endif  // DACHARTADD3DLINEWIDGET_H
