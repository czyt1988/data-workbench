#ifndef DADATASTATSWIDGET_H
#define DADATASTATSWIDGET_H

#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QWidget>

class QAction;
class QActionGroup;
class QLabel;
class QMenu;
class QSplitter;
class QTableWidget;

// pybind11 结果字典（只在 .cpp 里访问，头文件保持前向声明）
namespace pybind11
{
class dict;
}

namespace DA
{
class DAChartWidget;
class DAChartOperateWidget;

/**
 * @brief 数据统计分析窗口（列描述统计表格 + 统计图）
 *
 * 深度分析一列或几列数据的统计信息：
 * - 左侧统计表格：第一列固定为「统计量」，其后每个选中列一列；表头列宽可
 *   拖动调整。默认统计集为 count/mean/std/min/Q1/median/Q3/max（显式含中位数），
 *   其余统计量由 Ribbon「统计参数」面板勾选后加入。
 * - 右侧统计图：默认箱线图（每列一个箱体，x 轴标注列名，复用统计绘图管线
 *   DAStatsPlotCoordinator::renderBoxplotResult 的唯一实现）；可切换为帕累托图
 *   （列取值分箱后的分布率柱 + 累计分布线）。
 * - 本窗口不再自带工具栏：全部命令以 QAction 形式暴露给 Ribbon 的「统计」上下文
 *   标签页（统计参数 / 检验 / 绘图三个面板），标签页随本 dock 的显示/关闭自动出现
 *   与隐藏；动作的勾选状态由本窗口维护，窗口关闭后仍然保留。
 * - 支持把当前统计图「转入绘图区」（新建 Figure 标签页，一次撤销整个转入）。
 *
 * 数据入口复用快速绘图的 DATableChartRequest（数据集句柄 + 有序选中列 + 角色 +
 * 惰性标记）：内存小表精确计算，惰性/超大表按 DAChartSeriesSampler 采样并标注近似。
 *
 * 本类是纯内容控件（Widget-first 契约），由 APP 侧包成 ADS 浮动 dock 展示。
 */
class DAGUI_API DADataStatsWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 暴露给 Ribbon 的命令动作标识
     *
     * 每个统计项一个动作（勾选 = 出现在统计表中）；分位数把 5%/25%/50%/75%/95%
     * 收在一个菜单按钮里；检验与绘图各自成组。
     */
    enum class Command
    {
        Count,        ///< 数值个数
        Missing,      ///< 缺失值数
        Unique,       ///< 唯一值数
        Sum,          ///< 总和
        Mean,         ///< 均值
        Std,          ///< 标准差
        Var,          ///< 方差
        Sem,          ///< 标准误
        Min,          ///< 最小值
        Max,          ///< 最大值
        Range,        ///< 极差
        Iqr,          ///< 四分位距
        Mad,          ///< 绝对中位差
        Mode,         ///< 众数
        Cv,           ///< 变异系数
        Skew,         ///< 偏度
        Kurt,         ///< 峰度
        Percentile,      ///< 分位数下拉（5%/25%/50%/75%/95%）
        TestNormality,   ///< 正态性检验
        TestConfidence,  ///< 均值 95% 置信区间
        TestOutliers,    ///< 离群点分析
        Refresh,         ///< 按当前数据重算
        PlotBoxplot,     ///< 统计图切换：箱线图
        PlotPareto,      ///< 统计图切换：帕累托图
        ParetoRate,      ///< 帕累托图显示分布率
        ParetoCumulative,  ///< 帕累托图显示累计分布
        TransferToFigure   ///< 转入绘图区
    };
    Q_ENUM(Command)

    /**
     * @brief 右侧统计图的类型
     */
    enum class PlotType
    {
        Boxplot = 0,  ///< 箱线图（默认）
        Pareto        ///< 帕累托图（分布率 + 累计分布）
    };
    Q_ENUM(PlotType)

public:
    explicit DADataStatsWidget(QWidget* parent = nullptr);
    ~DADataStatsWidget() override;

    // 用请求打开/刷新统计窗口；没有数值列时给出用户可见提示并清空界面
    void showForRequest(const DA::DATableChartRequest& request);
    // 绑定的数据已变化：显示刷新提示（不自动重算，点「刷新」按钮重算）
    void notifyDataChanged();
    // 翻译（动作文本、表格首列与文本统计值）
    void retranslateUi();

    // 命令动作（Ribbon 面板挂载；未识别的命令返回 nullptr）
    QAction* command(Command c) const;
    // 分位数菜单（挂在分位数动作下，也可单独取用）
    QMenu* percentileMenu() const;

    // 当前绘图类型
    PlotType plotType() const;
    void setPlotType(PlotType t);
    // 帕累托图是否显示分布率 / 累计分布
    bool isParetoRateVisible() const;
    bool isParetoCumulativeVisible() const;

    // 把当前统计图转入绘图区（新建 Figure 标签页，可一次撤销）；绘图区由 APP 注入
    bool transferToFigure(DAChartOperateWidget* chartOperate);

Q_SIGNALS:
    // 用户请求把当前统计图转入绘图区（APP 收到后注入绘图区并调用 transferToFigure）
    void transferToFigureRequested();

private Q_SLOTS:
    // 统计项勾选变化（统计项动作与分位数菜单项共用，统计键存在 action->data()）
    void onStatActionToggled(bool on);
    // 检验组勾选变化（组键存在 action->data()）
    void onAnalysisActionToggled(bool on);
    // 绘图类型切换（互斥动作组）
    void onPlotTypeActionTriggered(QAction* act);
    // 帕累托图显示选项变化
    void onParetoOptionToggled(bool on);
    void onRefreshTriggered();
    void onTransferTriggered();

private:
    /**
     * @brief 一次采样的提示口径（近似/样本受限），供底部提示条使用
     */
    struct SampleNotice
    {
        bool approximate { false };  ///< 是否基于采样
        bool limited { false };      ///< 是否因预算上限提前停止
        int sampledRows { 0 };       ///< 实际参与计算的行数
        int totalRows { 0 };         ///< 数据总行数
    };

    void buildUi();
    void buildActions();
    // 重建分位数下拉（勾选状态保留在 mEnabledStats）
    void rebuildPercentileMenu();
    // 帕累托图选项的可用性（只在帕累托图下可用）
    void updateParetoOptionState();
    // 当前应计算的统计键（按固定顺序过滤勾选集，再追加勾选的检验组）
    QStringList activeStatKeys() const;
    // 统计键的显示名（表格首列文案 / 动作文本）
    QString statDisplayName(const QString& key) const;
    // 统计键的动作提示
    QString statDescription(const QString& key) const;
    // 分析开关组的显示名（字面量 tr 便于 lupdate 提取）
    QString groupDisplayName(int index) const;
    // 分析开关组的动作提示
    QString groupDescription(int index) const;
    // 文本统计值的翻译（yes/no、shapiro/normaltest/none）
    QString statTextValue(const QString& key, const QString& value) const;
    // 数值格式化：整型按整数、浮点保留 6 位有效数字
    static QString formatValue(double v, bool isInt);
    // 计算当前请求的描述统计（表格行 + 箱线图数据）；调用方必须持有 GIL
    bool computeDescribe(pybind11::dict* out, QStringList* labels, SampleNotice* notice);
    // 当前是否有可转入绘图区的有效结果
    bool hasResult() const;
    // 按当前请求与勾选状态重算并刷新表格与统计图
    void refresh();
    // 清空表格与图表（无数据/出错时）
    void clearAll();
    // 把 describe 结果画到指定 chart（调用方必须持有 GIL）
    bool renderPlot(DAChartWidget* chart, const pybind11::dict& describeResult);
    // 帕累托图：首个数值列的分布率 + 累计分布（调用方必须持有 GIL）
    bool renderPareto(DAChartWidget* chart);

private:
    DA::DATableChartRequest mRequest;
    bool mHasResult { false };
    bool mDataChanged { false };
    QSet< QString > mEnabledStats;   ///< 勾选的统计键
    QSet< QString > mEnabledGroups;  ///< 勾选的检验组
    QString mLastMessage;            ///< 提示文案（近似/样本受限）
    PlotType mPlotType { PlotType::Boxplot };
    bool mParetoShowRate { true };
    bool mParetoShowCumulative { true };

    QHash< int, QAction* > mCommands;  ///< Command → QAction
    QMenu* mPercentileMenu { nullptr };
    QAction* mPlotBoxplotAction { nullptr };
    QAction* mPlotParetoAction { nullptr };
    QActionGroup* mPlotTypeGroup { nullptr };

    QSplitter* mSplitter { nullptr };
    QTableWidget* mTable { nullptr };
    DAChartWidget* mChart { nullptr };
    QLabel* mNoticeLabel { nullptr };
};

}  // namespace DA
#endif  // DADATASTATSWIDGET_H
