#ifndef DACHARTADD3DSURFACEWIDGET_H
#define DACHARTADD3DSURFACEWIDGET_H
#include "DAGuiAPI.h"
#include "Chart/AddItem/DAAbstractChart3DAddItemWidget.h"
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
    // 编程式入口（快速绘图等无界面调用方）：三列散点数据，创建时内部用 scipy 插值成
    // 30×30 网格；scipy 不可用时 create3DPlotItem 返回 nullptr，原因见 programmaticError()
    void setProgrammaticScatterData(const QVector< double >& xs,
                                    const QVector< double >& ys,
                                    const QVector< double >& zs);
    // 是否已设置编程式数据
    bool hasProgrammaticData() const;
    // 编程式路径的失败原因（已翻译；成功时为空）
    QString programmaticError() const;

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
                                           QVector< QVector< double > >& zMatrix);
    // 三列散点向量 → 规则网格（UI 路径与编程式路径共用）
    bool interpolateScatter(const QVector< double >& xs,
                            const QVector< double >& ys,
                            const QVector< double >& zs,
                            QVector< double >& xCoords,
                            QVector< double >& yCoords,
                            QVector< QVector< double > >& zMatrix);
    // 刷新DataFrame下拉框
    void refreshDataframeCombo();
    // 刷新列下拉框（散点模式）
    void refreshColumnCombos();
    // 获取当前选中的DataFrame
    DAData getCurrentDataframe() const;

private:
    Ui::DAChartAdd3DSurfaceWidget* ui;
    DADataManager* mDataMgr { nullptr };
    QVector< double > mProgrammaticX;
    QVector< double > mProgrammaticY;
    QVector< double > mProgrammaticZ;
    QString mProgrammaticError;
};
}  // end DA
#endif  // DACHARTADD3DSURFACEWIDGET_H
