#ifndef DADATASTATSWIDGET_H
#define DADATASTATSWIDGET_H

#include "DAGuiAPI.h"
#include "TableChart/DATableChartTypes.h"
#include <QList>
#include <QSet>
#include <QString>
#include <QWidget>

class QLabel;
class QMenu;
class QSplitter;
class QTableWidget;
class QToolButton;

namespace DA
{
class DAChartWidget;

/**
 * @brief 数据统计分析窗口（列描述统计表格 + 箱线图）
 *
 * 深度分析一列或几列数据的统计信息：
 * - 左侧统计表格：第一列固定为「统计量」，其后每个选中列一列；表头列宽可
 *   拖动调整；默认包含 count/mean/std/min/Q1/median/Q3/max（显式含中位数）。
 * - 右侧箱线图：每个选中列一个箱体，x 轴标注列名（复用统计绘图管线
 *   DAStatsPlotCoordinator::renderBoxplotResult 的唯一实现）。
 * - 工具栏可增减统计参数（方差/标准误/众数/极差/IQR/变异系数/绝对中位差/
 *   偏度/峰度/分位数/唯一值/缺失数…），并挂接常用分析开关：
 *   正态性检验（Shapiro-Wilk / D'Agostino K²）、均值 95% 置信区间、离群点分析。
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
    explicit DADataStatsWidget(QWidget* parent = nullptr);
    ~DADataStatsWidget() override;

    // 用请求打开/刷新统计窗口；没有数值列时给出用户可见提示并清空界面
    void showForRequest(const DA::DATableChartRequest& request);
    // 绑定的数据已变化：显示刷新提示（不自动重算，点「刷新」按钮重算）
    void notifyDataChanged();
    // 翻译（重建工具栏菜单、表格首列与文本统计值）
    void retranslateUi();

private Q_SLOTS:
    void onOptionalStatToggled(bool on);
    void onAnalysisGroupToggled(bool on);
    void onRefreshClicked();

private:
    void buildUi();
    void rebuildMenus();
    // 当前应计算的统计键（默认集 + 勾选的可选集 + 勾选的分析组）
    QStringList activeStatKeys() const;
    // 统计键的显示名（表格首列文案）
    QString statDisplayName(const QString& key) const;
    // 分析开关组的显示名（字面量 tr 便于 lupdate 提取）
    QString groupDisplayName(int index) const;
    // 文本统计值的翻译（yes/no、shapiro/normaltest/none）
    QString statTextValue(const QString& key, const QString& value) const;
    // 数值格式化：整型按整数、浮点保留 6 位有效数字
    static QString formatValue(double v, bool isInt);
    // 按当前请求与勾选状态重算并刷新表格与箱线图
    void refresh();
    // 清空表格与图表（无数据/出错时）
    void clearAll();

private:
    DA::DATableChartRequest mRequest;
    bool mHasResult { false };
    bool mDataChanged { false };
    QSet< QString > mEnabledStats;   ///< 勾选的可选统计键
    QSet< QString > mEnabledGroups;  ///< 勾选的分析开关
    QString mLastMessage;             ///< 提示文案（近似/样本受限）

    QToolButton* mStatsButton { nullptr };
    QToolButton* mAnalysisButton { nullptr };
    QMenu* mStatsMenu { nullptr };
    QMenu* mAnalysisMenu { nullptr };
    QToolButton* mRefreshButton { nullptr };
    QSplitter* mSplitter { nullptr };
    QTableWidget* mTable { nullptr };
    DAChartWidget* mChart { nullptr };
    QLabel* mNoticeLabel { nullptr };
};

}  // namespace DA
#endif  // DADATASTATSWIDGET_H
