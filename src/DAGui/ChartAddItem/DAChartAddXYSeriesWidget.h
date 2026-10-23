#ifndef DACHARTADDXYSERIESWIDGET_H
#define DACHARTADDXYSERIESWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
// DAData
#include "DAData.h"
// DAGui
#include "DAChartSeriesDataResult.h"
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
 * @brief 单条 xy 序列的来源与命名信息（y 支持多条，x 仅一条）
 */
struct DAGUI_API DAChartXYItemInfo
{
    QPair< DAData, QString > x;            ///< x 的 dataframe 与列名，列名空表示自增序列
    QList< QPair< DAData, QString > > y;   ///< y 的 dataframe 与列名列表，列名空表示自增序列
};

/**
 * @brief 添加xy series，适用二维数据绘图的系列获取
 * 这个是一个abstract类，需要重写@sa createPlotItem
 *
 * 取数分为两层：extractSeries()/extractMultiSeries() 不访问界面、不弹窗，
 * 返回带失败原因的结果；getSeries() 是它的薄封装。需要界面提示的调用方
 * 自行使用 describeSeriesError()/warnSeriesError()。
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
    // 根据配置获取单条点列（无界面副作用，失败返回空）
    QVector< QPointF > getSeries() const;
    // 提取单条 xy 序列（无界面副作用），失败原因通过结果回传
    DAChartSeriesResult extractSeries() const;
    // 提取多条 xy 序列（无界面副作用），失败原因通过结果回传
    DAChartMultiSeriesResult extractMultiSeries() const;
    // 把取数失败原因翻译成界面文案
    QString describeSeriesError(const DAChartSeriesDataError& error) const;
    // 弹出取数失败警告（界面层），转换类失败同时写日志
    void warnSeriesError(const DAChartSeriesDataError& error);
    // 获取推荐的名字
    virtual QString getNameHint() const;
    // 设置当前的x到list中
    void setX(const DAData& dataframeData, const QString& seriesName);
    // 设置当前的y到list中
    void setY(const DAData& dataframeData, const QString& seriesName);
    // 移除一个y（按 dataframe+列名），供外部多y场景使用
    void removeY(const DAData& dataframeData, const QString& seriesName);
    // 获取所有已选y（顺序保持添加顺序）
    QList< QPair< DAData, QString > > getYList() const;
    // 获取xy的来源信息（x一条，y多条）
    DAChartXYItemInfo getXYInfo() const;
    // 坐标轴标题推荐：x轴用x列名（自增时"x"），y轴单个y用列名、多个y用"y"（y自增时"y"）
    virtual bool getAxisTitleHint(QString& xTitle, QString& yTitle) const override;
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
    // 从界面收集取数输入；自增数值解析失败时填充 error 并返回 false
    bool buildSeriesSpec(DAChartXYSeriesSpec& spec, DAChartSeriesDataError& error) const;
    // 尝试获取x值的自增内容
    bool tryGetXSelfInc(double& base, double& step) const;
    // 尝试获取y值的自增内容
    bool tryGetYSelfInc(double& base, double& step) const;
    // 获取
    QPair< DAData, QString > getY() const;
    QPair< DAData, QString > getX() const;
    //
    DAPySeries getYSeries() const;
    DAPySeries getXSeries() const;
    // 获取列表中全部y对应的 DAPySeries
    QList< DAPySeries > getYSeriesList() const;

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
