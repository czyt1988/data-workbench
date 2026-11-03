#include "Chart/DADataStatsWidget.h"
#include "DAChartOperateWidget.h"
#include "DAChartOperateWidgetCommands.h"
#include "DAChartPlotRenderer.h"
#include "Table/Chart/DAChartSeriesSampler.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "DALogCategory.h"
#include "DAPyScripts.h"
#include "DAPybind11QtCaster.hpp"
#include "Chart/DAStatsPlotCoordinator.h"
#include "DAWaitCursorScoped.h"

#include "qwt_axis.h"
#include "qwt_plot_item.h"
#include "qwt_scale_draw.h"

#include <pybind11/pybind11.h>

#include <QAction>
#include <QActionGroup>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUndoStack>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

namespace
{

// 箱线图须长度（IQR 倍数），与统计绘图管线 / 快速绘图一致
constexpr double kBoxplotWhisker = 1.5;

// 帕累托图分箱数：分箱标签较长，10 个箱在统计窗口里可读性最好
constexpr int kParetoBins = 10;

// 统计键的规范顺序（表格行序 / 统计结果行序）
const char* const kStatKeyOrder[] = {
    "count", "missing", "unique", "sum",    "mean", "std",    "var", "sem",
    "min",   "q05",     "q1",     "median", "q3",   "q95",    "max", "range",
    "iqr",   "mad",     "mode",   "cv",     "skew", "kurt",
};
constexpr int kStatKeyOrderCount = sizeof(kStatKeyOrder) / sizeof(kStatKeyOrder[ 0 ]);

// 统计项动作定义（Command → 统计键），顺序即动作创建顺序
struct StatActionDef
{
    DADataStatsWidget::Command cmd;
    const char* statKey;
};
const StatActionDef kStatActionDefs[] = {
    { DADataStatsWidget::Command::Count, "count" },
    { DADataStatsWidget::Command::Mean, "mean" },
    { DADataStatsWidget::Command::Std, "std" },
    { DADataStatsWidget::Command::Min, "min" },
    { DADataStatsWidget::Command::Max, "max" },
    { DADataStatsWidget::Command::Missing, "missing" },
    { DADataStatsWidget::Command::Unique, "unique" },
    { DADataStatsWidget::Command::Sum, "sum" },
    { DADataStatsWidget::Command::Var, "var" },
    { DADataStatsWidget::Command::Sem, "sem" },
    { DADataStatsWidget::Command::Range, "range" },
    { DADataStatsWidget::Command::Iqr, "iqr" },
    { DADataStatsWidget::Command::Mad, "mad" },
    { DADataStatsWidget::Command::Mode, "mode" },
    { DADataStatsWidget::Command::Cv, "cv" },
    { DADataStatsWidget::Command::Skew, "skew" },
    { DADataStatsWidget::Command::Kurt, "kurt" },
};
constexpr int kStatActionDefCount = sizeof(kStatActionDefs) / sizeof(kStatActionDefs[ 0 ]);

// 分位数菜单项的统计键（与 rebuildPercentileMenu 的显示文案一一对应）
const char* const kPercentileKeys[] = { "q05", "q1", "median", "q3", "q95" };
constexpr int kPercentileKeyCount = sizeof(kPercentileKeys) / sizeof(kPercentileKeys[ 0 ]);

// 默认勾选的统计集（与旧版工具栏的默认统计集一致）
const char* const kDefaultStatKeys[] = { "count", "mean", "std", "min", "q1", "median", "q3", "max" };
constexpr int kDefaultStatKeyCount = sizeof(kDefaultStatKeys) / sizeof(kDefaultStatKeys[ 0 ]);

// 检验组（「检验」面板勾选）：开关 → 打开时追加的统计键
// 显示名走 groupDisplayName 的字面量 tr（lupdate 提取），key 只做身份标识
struct StatGroupDef
{
    DADataStatsWidget::Command cmd;
    const char* key;
    QList< const char* > statKeys;
};
const StatGroupDef kStatGroups[] = {
    { DADataStatsWidget::Command::TestNormality, "normality", { "norm_stat", "norm_p", "norm_verdict", "norm_test" } },
    { DADataStatsWidget::Command::TestConfidence, "ci", { "ci_low", "ci_high" } },
    { DADataStatsWidget::Command::TestOutliers, "outliers", { "outliers", "outlier_low", "outlier_high" } },
};
constexpr int kStatGroupCount = sizeof(kStatGroups) / sizeof(kStatGroups[ 0 ]);

// 图表资源（统计类图型统一放在 DAGui 的 ChartType 资源前缀下）
const char* const kIconBoxplot    = ":/DAGui/ChartType/icon/chart-type/stats-boxplot.svg";
const char* const kIconPareto     = ":/DAGui/ChartType/icon/chart-type/stats-pareto.svg";
const char* const kIconRate       = ":/DAGui/ChartType/icon/chart-type/chart-bar.svg";
const char* const kIconCumulative = ":/DAGui/ChartType/icon/chart-type/stats-ecdfplot.svg";
const char* const kIconRefresh    = ":/DAGui/icon/refresh.svg";
const char* const kIconPercentile = ":/DAGui/icon/stats/statsPercentile.svg";
const char* const kIconTransfer   = ":/DAGui/icon/stats/statsTransferToFigure.svg";

// 统计项动作图标（DAGui/icon/stats/ 下，200×200 统一规格）
QString statIconPath(const QString& key)
{    if (key == "count") return QStringLiteral(":/DAGui/icon/stats/statsCount.svg");
    if (key == "missing") return QStringLiteral(":/DAGui/icon/stats/statsMissing.svg");
    if (key == "unique") return QStringLiteral(":/DAGui/icon/stats/statsUnique.svg");
    if (key == "sum") return QStringLiteral(":/DAGui/icon/stats/statsSum.svg");
    if (key == "mean") return QStringLiteral(":/DAGui/icon/stats/statsMean.svg");
    if (key == "std") return QStringLiteral(":/DAGui/icon/stats/statsStd.svg");
    if (key == "var") return QStringLiteral(":/DAGui/icon/stats/statsVar.svg");
    if (key == "sem") return QStringLiteral(":/DAGui/icon/stats/statsSem.svg");
    if (key == "min") return QStringLiteral(":/DAGui/icon/stats/statsMin.svg");
    if (key == "max") return QStringLiteral(":/DAGui/icon/stats/statsMax.svg");
    if (key == "range") return QStringLiteral(":/DAGui/icon/stats/statsRange.svg");
    if (key == "iqr") return QStringLiteral(":/DAGui/icon/stats/statsIqr.svg");
    if (key == "mad") return QStringLiteral(":/DAGui/icon/stats/statsMad.svg");
    if (key == "mode") return QStringLiteral(":/DAGui/icon/stats/statsMode.svg");
    if (key == "cv") return QStringLiteral(":/DAGui/icon/stats/statsCv.svg");
    if (key == "skew") return QStringLiteral(":/DAGui/icon/stats/statsSkew.svg");
    if (key == "kurt") return QStringLiteral(":/DAGui/icon/stats/statsKurt.svg");
    return QString::fromLatin1(kIconPercentile);
}

// 检验组动作图标（正态性 / 置信区间 / 离群点）
QString groupIconPath(int index)
{
    switch (index) {
    case 0: return QStringLiteral(":/DAGui/icon/stats/statsNormality.svg");
    case 1: return QStringLiteral(":/DAGui/icon/stats/statsConfidence.svg");
    default: return QStringLiteral(":/DAGui/icon/stats/statsOutliers.svg");
    }
}

// pybind11::dict 里取浮点列表
QVector< double > dictToDoubleVector(const pybind11::dict& d, const char* key)
{
    QVector< double > res;
    if (!d.contains(key)) {
        return res;
    }
    pybind11::object obj = d[ key ];
    if (obj.is_none() || !pybind11::isinstance< pybind11::list >(obj)) {
        return res;
    }
    try {
        for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(obj)) {
            res.append(item.is_none() ? qQNaN() : pybind11::cast< double >(item));
        }
    } catch (const std::exception& e) {
        qWarning() << "DADataStatsWidget: cannot read double list" << key << e.what();
        res.clear();
    }
    return res;
}

// 清空后把坐标轴恢复成普通线性轴（上一次可能是类别轴）
void resetChartAxes(DAChartWidget* chart)
{
    if (!chart) {
        return;
    }
    const int axes[] = { QwtAxis::XBottom, QwtAxis::YLeft };
    for (int axis : axes) {
        chart->setAxisScaleDraw(axis, new QwtScaleDraw());
        chart->setAxisAutoScale(axis, true);
        chart->setAxisTitle(axis, QString());
    }
}

// 重名列的显示名：name、name (2)、name (3)…（与快速绘图一致）
QString columnDisplayName(const DA::DATableChartColumn& column)
{
    if (column.occurrence <= 0) {
        return column.name;
    }
    return QStringLiteral("%1 (%2)").arg(column.name).arg(column.occurrence + 1);
}

}  // namespace

//===================================================
// DADataStatsWidget
//===================================================

DADataStatsWidget::DADataStatsWidget(QWidget* parent) : QWidget(parent)
{
    for (int i = 0; i < kDefaultStatKeyCount; ++i) {
        mEnabledStats.insert(QString::fromLatin1(kDefaultStatKeys[ i ]));
    }
    buildUi();
}

DADataStatsWidget::~DADataStatsWidget()
{
}

/**
 * @brief 构建界面：分割窗口（左表格 / 右统计图）+ 提示条
 *
 * 工具栏已移除，全部命令以 QAction 暴露给 Ribbon 的「统计」上下文标签页。
 */
void DADataStatsWidget::buildUi()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);

    // 分割窗口：左统计表格 / 右统计图
    mSplitter = new QSplitter(Qt::Horizontal, this);
    mTable    = new QTableWidget(mSplitter);
    mTable->setColumnCount(1);
    mTable->setHorizontalHeaderLabels({ tr("Statistic") });  // cn:统计量
    mTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);  // 表头列宽可拖动调整
    mTable->horizontalHeader()->setDefaultSectionSize(100);
    mTable->horizontalHeader()->setStretchLastSection(false);
    mTable->verticalHeader()->setVisible(false);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mChart = new DAChartWidget(mSplitter);
    mChart->setMinimumHeight(240);
    mSplitter->addWidget(mTable);
    mSplitter->addWidget(mChart);
    mSplitter->setStretchFactor(0, 2);
    mSplitter->setStretchFactor(1, 3);
    mSplitter->setChildrenCollapsible(false);
    root->addWidget(mSplitter, 1);

    // 提示条（近似/样本受限/数据变化）
    mNoticeLabel = new QLabel(this);
    mNoticeLabel->setWordWrap(true);
    mNoticeLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));
    mNoticeLabel->hide();
    root->addWidget(mNoticeLabel);

    buildActions();
}

/**
 * @brief 构建 Ribbon 面板使用的全部命令动作
 *
 * 动作由本窗口持有（勾选状态即统计参数状态，窗口关闭后保留），Ribbon 面板只负责
 * 按布局取用：统计项动作 / 分位数下拉 / 检验动作 / 刷新 / 绘图类型 / 帕累托选项 /
 * 转入绘图区。
 */
void DADataStatsWidget::buildActions()
{
    // ---- 统计项动作（勾选 = 出现在统计表中）----
    for (int i = 0; i < kStatActionDefCount; ++i) {
        const StatActionDef& def = kStatActionDefs[ i ];
        const QString key        = QString::fromLatin1(def.statKey);
        QAction* act             = new QAction(this);
        act->setCheckable(true);
        act->setChecked(mEnabledStats.contains(key));
        act->setData(key);
        connect(act, &QAction::toggled, this, &DADataStatsWidget::onStatActionToggled);
        mCommands.insert(static_cast< int >(def.cmd), act);
    }

    // ---- 分位数下拉（5%/25%/50%/75%/95%）----
    QAction* percentile = new QAction(this);
    percentile->setIcon(QIcon(QString::fromLatin1(kIconPercentile)));
    mPercentileMenu = new QMenu(this);
    percentile->setMenu(mPercentileMenu);
    mCommands.insert(static_cast< int >(Command::Percentile), percentile);
    rebuildPercentileMenu();

    // ---- 检验动作（勾选 = 表格追加对应统计行）----
    for (int i = 0; i < kStatGroupCount; ++i) {
        const StatGroupDef& def = kStatGroups[ i ];
        const QString key       = QString::fromLatin1(def.key);
        QAction* act            = new QAction(this);
        act->setCheckable(true);
        act->setChecked(mEnabledGroups.contains(key));
        act->setData(key);
        act->setIcon(QIcon(groupIconPath(i)));
        connect(act, &QAction::toggled, this, &DADataStatsWidget::onAnalysisActionToggled);
        mCommands.insert(static_cast< int >(def.cmd), act);
    }

    // ---- 刷新 ----
    QAction* refresh = new QAction(this);
    refresh->setIcon(QIcon(QString::fromLatin1(kIconRefresh)));
    connect(refresh, &QAction::triggered, this, &DADataStatsWidget::onRefreshTriggered);
    mCommands.insert(static_cast< int >(Command::Refresh), refresh);

    // ---- 绘图类型（互斥：箱线图 / 帕累托图）----
    mPlotTypeGroup = new QActionGroup(this);
    mPlotTypeGroup->setExclusive(true);
    mPlotBoxplotAction = new QAction(this);
    mPlotBoxplotAction->setCheckable(true);
    mPlotBoxplotAction->setChecked(true);
    mPlotBoxplotAction->setIcon(QIcon(QString::fromLatin1(kIconBoxplot)));
    mPlotBoxplotAction->setData(static_cast< int >(PlotType::Boxplot));
    mPlotParetoAction = new QAction(this);
    mPlotParetoAction->setCheckable(true);
    mPlotParetoAction->setIcon(QIcon(QString::fromLatin1(kIconPareto)));
    mPlotParetoAction->setData(static_cast< int >(PlotType::Pareto));
    mPlotTypeGroup->addAction(mPlotBoxplotAction);
    mPlotTypeGroup->addAction(mPlotParetoAction);
    connect(mPlotTypeGroup, &QActionGroup::triggered, this, &DADataStatsWidget::onPlotTypeActionTriggered);
    mCommands.insert(static_cast< int >(Command::PlotBoxplot), mPlotBoxplotAction);
    mCommands.insert(static_cast< int >(Command::PlotPareto), mPlotParetoAction);

    // ---- 帕累托图显示选项（分布率 / 累计分布）----
    QAction* rate = new QAction(this);
    rate->setCheckable(true);
    rate->setChecked(mParetoShowRate);
    rate->setIcon(QIcon(QString::fromLatin1(kIconRate)));
    connect(rate, &QAction::toggled, this, &DADataStatsWidget::onParetoOptionToggled);
    mCommands.insert(static_cast< int >(Command::ParetoRate), rate);

    QAction* cumulative = new QAction(this);
    cumulative->setCheckable(true);
    cumulative->setChecked(mParetoShowCumulative);
    cumulative->setIcon(QIcon(QString::fromLatin1(kIconCumulative)));
    connect(cumulative, &QAction::toggled, this, &DADataStatsWidget::onParetoOptionToggled);
    mCommands.insert(static_cast< int >(Command::ParetoCumulative), cumulative);

    // ---- 转入绘图区 ----
    QAction* transfer = new QAction(this);
    transfer->setIcon(QIcon(QString::fromLatin1(kIconTransfer)));
    connect(transfer, &QAction::triggered, this, &DADataStatsWidget::onTransferTriggered);
    mCommands.insert(static_cast< int >(Command::TransferToFigure), transfer);

    retranslateUi();
    updateParetoOptionState();
}

/**
 * @brief 重建分位数下拉（勾选状态保留在 mEnabledStats）
 */
void DADataStatsWidget::rebuildPercentileMenu()
{
    if (!mPercentileMenu) {
        return;
    }
    mPercentileMenu->clear();
    for (int i = 0; i < kPercentileKeyCount; ++i) {
        QString label;
        switch (i) {
        case 0: label = tr("5%"); break;  // cn:5%
        case 1: label = tr("25%"); break;  // cn:25%
        case 2: label = tr("50% (Median)"); break;  // cn:50%（中位数）
        case 3: label = tr("75%"); break;  // cn:75%
        default: label = tr("95%"); break;  // cn:95%
        }
        const QString key = QString::fromLatin1(kPercentileKeys[ i ]);
        QAction* act      = mPercentileMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(mEnabledStats.contains(key));
        act->setData(key);
        // 与普通统计项动作同一个槽：统计键存在 action->data()
        connect(act, &QAction::toggled, this, &DADataStatsWidget::onStatActionToggled);
    }
}

/**
 * @brief 取命令动作（未识别返回 nullptr）
 */
QAction* DADataStatsWidget::command(Command c) const
{
    return mCommands.value(static_cast< int >(c), nullptr);
}

/**
 * @brief 分位数菜单
 */
QMenu* DADataStatsWidget::percentileMenu() const
{
    return mPercentileMenu;
}

DADataStatsWidget::PlotType DADataStatsWidget::plotType() const
{
    return mPlotType;
}

/**
 * @brief 切换统计图类型
 *
 * 同步互斥动作的勾选状态；已有请求时立即重算刷新（切换图型不改统计表口径）。
 */
void DADataStatsWidget::setPlotType(PlotType t)
{
    if (mPlotType == t) {
        return;
    }
    mPlotType = t;
    if (mPlotBoxplotAction) {
        const QSignalBlocker blocker(mPlotBoxplotAction);
        mPlotBoxplotAction->setChecked(t == PlotType::Boxplot);
    }
    if (mPlotParetoAction) {
        const QSignalBlocker blocker(mPlotParetoAction);
        mPlotParetoAction->setChecked(t == PlotType::Pareto);
    }
    updateParetoOptionState();
    if (mRequest.isValid()) {
        refresh();
    }
}

bool DADataStatsWidget::isParetoRateVisible() const
{
    return mParetoShowRate;
}

bool DADataStatsWidget::isParetoCumulativeVisible() const
{
    return mParetoShowCumulative;
}

/**
 * @brief 帕累托图的两个显示选项只在帕累托图下可用
 */
void DADataStatsWidget::updateParetoOptionState()
{
    const bool pareto = (mPlotType == PlotType::Pareto);
    if (QAction* rate = command(Command::ParetoRate)) {
        rate->setEnabled(pareto);
    }
    if (QAction* cumulative = command(Command::ParetoCumulative)) {
        cumulative->setEnabled(pareto);
    }
}

void DADataStatsWidget::onPlotTypeActionTriggered(QAction* act)
{
    if (!act) {
        return;
    }
    setPlotType(static_cast< PlotType >(act->data().toInt()));
}

void DADataStatsWidget::onParetoOptionToggled(bool on)
{
    QAction* act = qobject_cast< QAction* >(sender());
    if (!act) {
        return;
    }
    if (act == command(Command::ParetoRate)) {
        mParetoShowRate = on;
    } else if (act == command(Command::ParetoCumulative)) {
        mParetoShowCumulative = on;
    }
    if (mRequest.isValid()) {
        refresh();
    }
}

/**
 * @brief 统计项勾选变化（统计项动作与分位数菜单项共用）
 */
void DADataStatsWidget::onStatActionToggled(bool on)
{
    QAction* act = qobject_cast< QAction* >(sender());
    if (!act) {
        return;
    }
    const QString key = act->data().toString();
    if (key.isEmpty()) {
        return;
    }
    if (on) {
        mEnabledStats.insert(key);
    } else {
        mEnabledStats.remove(key);
    }
    // 全部取消勾选时也要刷新：表格清空，但请求保留，重新勾选即可恢复
    if (mRequest.isValid()) {
        refresh();
    }
}

/**
 * @brief 检验组勾选变化
 */
void DADataStatsWidget::onAnalysisActionToggled(bool on)
{
    QAction* act = qobject_cast< QAction* >(sender());
    if (!act) {
        return;
    }
    const QString key = act->data().toString();
    if (key.isEmpty()) {
        return;
    }
    if (on) {
        mEnabledGroups.insert(key);
    } else {
        mEnabledGroups.remove(key);
    }
    if (mRequest.isValid()) {
        refresh();
    }
}

void DADataStatsWidget::onRefreshTriggered()
{
    if (mRequest.isValid()) {
        refresh();
    }
}

void DADataStatsWidget::onTransferTriggered()
{
    if (!hasResult()) {
        daWarning << tr("There is no statistics plot to transfer");  // cn:没有可转入绘图区的统计图
        return;
    }
    emit transferToFigureRequested();
}

/**
 * @brief 当前应计算的统计键（规范顺序过滤勾选集，再追加勾选的检验组）
 */
QStringList DADataStatsWidget::activeStatKeys() const
{
    QStringList keys;
    for (int i = 0; i < kStatKeyOrderCount; ++i) {
        const QString key = QString::fromLatin1(kStatKeyOrder[ i ]);
        if (mEnabledStats.contains(key)) {
            keys.append(key);
        }
    }
    for (int i = 0; i < kStatGroupCount; ++i) {
        if (mEnabledGroups.contains(QString::fromLatin1(kStatGroups[ i ].key))) {
            for (const char* k : kStatGroups[ i ].statKeys) {
                keys.append(QString::fromLatin1(k));
            }
        }
    }
    return keys;
}

/**
 * @brief 统计键 → 表格第一列显示名（同时作为统计项动作文本）
 */
QString DADataStatsWidget::statDisplayName(const QString& key) const
{
    if (key == "count") return tr("Count");  // cn:数值个数
    if (key == "mean") return tr("Mean");  // cn:均值
    if (key == "std") return tr("Std Dev");  // cn:标准差
    if (key == "min") return tr("Min");  // cn:最小值
    if (key == "q1") return tr("Q1 (25%)");  // cn:下四分位数 (25%)
    if (key == "median") return tr("Median (50%)");  // cn:中位数 (50%)
    if (key == "q3") return tr("Q3 (75%)");  // cn:上四分位数 (75%)
    if (key == "max") return tr("Max");  // cn:最大值
    if (key == "missing") return tr("Missing");  // cn:缺失值数
    if (key == "unique") return tr("Unique");  // cn:唯一值数
    if (key == "sum") return tr("Sum");  // cn:总和
    if (key == "var") return tr("Variance");  // cn:方差
    if (key == "sem") return tr("Std Error");  // cn:标准误
    if (key == "mad") return tr("MAD");  // cn:绝对中位差
    if (key == "range") return tr("Range");  // cn:极差
    if (key == "iqr") return tr("IQR");  // cn:四分位距
    if (key == "mode") return tr("Mode");  // cn:众数
    if (key == "cv") return tr("CV");  // cn:变异系数
    if (key == "q05") return tr("5th Percentile");  // cn:5% 分位数
    if (key == "q95") return tr("95th Percentile");  // cn:95% 分位数
    if (key == "skew") return tr("Skewness");  // cn:偏度
    if (key == "kurt") return tr("Kurtosis");  // cn:峰度
    if (key == "outliers") return tr("Outlier Count");  // cn:离群点数
    if (key == "outlier_low") return tr("Lower Fence");  // cn:离群下界
    if (key == "outlier_high") return tr("Upper Fence");  // cn:离群上界
    if (key == "norm_stat") return tr("Normality Statistic");  // cn:正态性统计量
    if (key == "norm_p") return tr("Normality p-value");  // cn:正态性 p 值
    if (key == "norm_verdict") return tr("Normal Distribution");  // cn:正态分布
    if (key == "norm_test") return tr("Normality Test Method");  // cn:正态性检验方法
    if (key == "ci_low") return tr("Mean CI Lower");  // cn:均值置信下限
    if (key == "ci_high") return tr("Mean CI Upper");  // cn:均值置信上限
    return key;
}

/**
 * @brief 统计项动作的提示文案
 */
QString DADataStatsWidget::statDescription(const QString& key) const
{
    if (key == "count") return tr("Number of non-missing values");  // cn:非空取值的个数
    if (key == "mean") return tr("Arithmetic mean of the values");  // cn:取值算术平均
    if (key == "std") return tr("Standard deviation (sample, ddof=1)");  // cn:标准差（样本，ddof=1）
    if (key == "min") return tr("Minimum value");  // cn:最小值
    if (key == "max") return tr("Maximum value");  // cn:最大值
    if (key == "missing") return tr("Number of missing (NaN) values");  // cn:缺失值个数
    if (key == "unique") return tr("Number of distinct values");  // cn:不同取值个数
    if (key == "sum") return tr("Sum of the values");  // cn:取值总和
    if (key == "var") return tr("Variance (sample, ddof=1)");  // cn:方差（样本，ddof=1）
    if (key == "sem") return tr("Standard error of the mean");  // cn:均值标准误
    if (key == "range") return tr("Maximum minus minimum");  // cn:极差（最大值减最小值）
    if (key == "iqr") return tr("Interquartile range (Q3 - Q1)");  // cn:四分位距（Q3-Q1）
    if (key == "mad") return tr("Median absolute deviation");  // cn:绝对中位差
    if (key == "cv") return tr("Coefficient of variation (std / mean)");  // cn:变异系数（标准差/均值）
    if (key == "skew") return tr("Skewness of the distribution");  // cn:分布偏度
    if (key == "kurt") return tr("Excess kurtosis of the distribution");  // cn:分布超额峰度
    if (key == "q05") return tr("5% percentile");  // cn:5% 分位数
    if (key == "q1") return tr("25% percentile (Q1)");  // cn:25% 分位数（Q1）
    if (key == "median") return tr("50% percentile (median)");  // cn:50% 分位数（中位数）
    if (key == "q3") return tr("75% percentile (Q3)");  // cn:75% 分位数（Q3）
    if (key == "q95") return tr("95% percentile");  // cn:95% 分位数
    return QString();
}

/**
 * @brief 检验组的显示名（字面量 tr 便于 lupdate 提取翻译）
 */
QString DADataStatsWidget::groupDisplayName(int index) const
{
    switch (index) {
    case 0: return tr("Normality Test");  // cn:正态性检验
    case 1: return tr("Mean 95% CI");  // cn:均值 95% 置信区间
    case 2: return tr("Outlier Analysis");  // cn:离群点分析
    default: return QString();
    }
}

/**
 * @brief 检验组的动作提示
 */
QString DADataStatsWidget::groupDescription(int index) const
{
    switch (index) {
    case 0:
        return tr("Add Shapiro-Wilk / D'Agostino K2 normality test rows");  // cn:追加 Shapiro-Wilk / D'Agostino K² 正态性检验行
    case 1:
        return tr("Add the 95% confidence interval rows of the mean");  // cn:追加均值的 95% 置信区间行
    case 2:
        return tr("Add 1.5x IQR outlier count and fence rows");  // cn:追加 1.5×IQR 离群点数与上下界行
    default: return QString();
    }
}

/**
 * @brief 文本统计值（机器键）的翻译显示
 */
QString DADataStatsWidget::statTextValue(const QString& key, const QString& value) const
{
    if (key == "norm_verdict") {
        if (value == "yes") return tr("yes");  // cn:是
        if (value == "no") return tr("no");  // cn:否
        return QStringLiteral("\u2014");
    }
    if (key == "norm_test") {
        if (value == "shapiro") return tr("Shapiro-Wilk");  // cn:Shapiro-Wilk
        if (value == "normaltest") return tr("D'Agostino-Pearson K\u00b2");  // cn:D'Agostino-Pearson K²
        return QStringLiteral("\u2014");
    }
    return value;
}

/**
 * @brief 数值格式化：整型按整数、浮点保留 6 位有效数字、NaN 显示 —
 */
QString DADataStatsWidget::formatValue(double v, bool isInt)
{
    if (qIsNaN(v)) {
        return QStringLiteral("\u2014");
    }
    if (isInt) {
        return QString::number(static_cast< qlonglong >(v));
    }
    return QString::number(v, 'g', 6);
}

/**
 * @brief 用请求打开/刷新统计窗口
 */
void DADataStatsWidget::showForRequest(const DA::DATableChartRequest& request)
{
    mRequest     = request;
    mDataChanged = false;
    mHasResult   = false;
    if (!request.isValid() || request.numericCount() == 0) {
        clearAll();
        daWarning << tr("No numeric column in the current selection");  // cn:当前选区没有数值列
        return;
    }
    refresh();
}

/**
 * @brief 绑定的数据已变化：显示刷新提示（不自动重算）
 */
void DADataStatsWidget::notifyDataChanged()
{
    if (!mHasResult) {
        return;
    }
    mDataChanged = true;
    mNoticeLabel->setText(tr("The data has changed. Click Refresh to recompute."));  // cn:数据已变化，点击「刷新」重新计算
    mNoticeLabel->show();
}

/**
 * @brief 当前是否有可转入绘图区的有效结果
 */
bool DADataStatsWidget::hasResult() const
{
    return mHasResult && mRequest.isValid();
}

/**
 * @brief 计算当前请求的描述统计（表格行 + 箱线图数据）
 *
 * 内存小表整表精确计算（列按位置传参，重名列消歧）；
 * 惰性/超大表按 DAChartSeriesSampler 采样后走值列表口径，并经 notice 标注近似。
 */
bool DADataStatsWidget::computeDescribe(pybind11::dict* out, QStringList* labels, SampleNotice* notice)
{
    if (!out) {
        return false;
    }
    *out = pybind11::dict();

    // 数值列（请求里按视图顺序）
    QList< int > numericIdx;
    for (int i = 0; i < mRequest.columns.size(); ++i) {
        if (mRequest.columns.at(i).isNumeric()) {
            numericIdx.append(i);
        }
    }
    if (numericIdx.isEmpty()) {
        return false;
    }

    QStringList columnLabels;
    for (int idx : std::as_const(numericIdx)) {
        columnLabels.append(columnDisplayName(mRequest.columns.at(idx)));
    }

    QVariantMap args;
    args[ QStringLiteral("whis") ] = kBoxplotWhisker;
    QVariantList statKeysArg;
    const QStringList activeKeys = activeStatKeys();
    for (const QString& k : std::as_const(activeKeys)) {
        statKeysArg.append(k);
    }
    args[ QStringLiteral("stats") ] = statKeysArg;

    bool approximate                       = false;
    DAChartSeriesSampler::SampleBudget budget;
    DAChartSeriesSampler::SampleInfo mergedInfo;

    if (DAChartSeriesSampler::canAggregateExactly(mRequest)) {
        // 内存小表：整表精确计算
        QVariantList columnsArg;
        for (int idx : std::as_const(numericIdx)) {
            columnsArg.append(mRequest.columns.at(idx).dataIndex);
        }
        QVariantList labelsArg;
        for (const QString& l : std::as_const(columnLabels)) {
            labelsArg.append(l);
        }
        args[ QStringLiteral("columns") ] = columnsArg;
        args[ QStringLiteral("labels") ]  = labelsArg;
        *out = DAPyScripts::getStatistics().computeDescribe(mRequest.data.toDataFrame(), args);
    } else {
        // 惰性/超大表：采样后按值列表计算（与快速绘图同一采样口径）
        QVariantMap columns;
        QStringList keys;
        for (int i = 0; i < numericIdx.size(); ++i) {
            DAChartSeriesSampler::SampleInfo info;
            const QVector< double > values = DAChartSeriesSampler::sampleValues(
                mRequest,
                mRequest.columns.at(numericIdx.at(i)).dataIndex,
                DAChartSeriesSampler::kMaxHistogramValues,
                &info,
                &budget);
            mergedInfo.totalRows       = info.totalRows;
            mergedInfo.sampled         = true;
            mergedInfo.sampledRows += info.sampledRows;
            mergedInfo.droppedMissing += info.droppedMissing;
            mergedInfo.limited = mergedInfo.limited || info.limited;
            const QString key  = QStringLiteral("__v%1").arg(i);
            keys.append(key);
            QVariantList valuesArg;
            valuesArg.reserve(values.size());
            for (double v : std::as_const(values)) {
                valuesArg.append(v);
            }
            columns[ key ] = valuesArg;
        }
        QVariantList labelsArg;
        for (const QString& l : std::as_const(columnLabels)) {
            labelsArg.append(l);
        }
        // 注意：传给 Python 的列表一律用 QVariantList——QStringList 值的 QVariant
        // 在 QtCaster 里没有对应分支，会退化成 toString() 的空串（列名丢失）
        QVariantList keysArg;
        for (const QString& k : std::as_const(keys)) {
            keysArg.append(k);
        }
        args[ QStringLiteral("columns") ] = keysArg;
        args[ QStringLiteral("labels") ]  = labelsArg;
        *out = DAPyScripts::getStatistics().computeDescribeFromLists(columns, args);
        approximate = true;
    }

    if (labels) {
        *labels = columnLabels;
    }
    if (notice) {
        notice->approximate = approximate;
        notice->limited     = mergedInfo.limited;
        notice->sampledRows = mergedInfo.sampledRows;
        notice->totalRows   = mergedInfo.totalRows;
    }
    return !out->is_none() && pybind11::len(*out) > 0;
}

/**
 * @brief 把 describe 结果画到指定图表
 *
 * 箱线图走统计绘图管线的唯一实现；帕累托图按首个数值列的分布重算。
 */
bool DADataStatsWidget::renderPlot(DAChartWidget* chart, const pybind11::dict& describeResult)
{
    if (!chart) {
        return false;
    }
    if (mPlotType == PlotType::Pareto) {
        return renderPareto(chart);
    }
    if (describeResult.is_none()) {
        return false;
    }
    pybind11::str boxKey("boxplot");
    if (!describeResult.contains(boxKey)) {
        return false;
    }
    pybind11::object boxObj = describeResult[ boxKey ];
    if (boxObj.is_none() || !pybind11::isinstance< pybind11::dict >(boxObj)) {
        return false;
    }
    DAChartPlotRenderer renderer(chart);
    renderer.enableLegend(false);
    return DAStatsPlotCoordinator::renderBoxplotResult(
        pybind11::cast< pybind11::dict >(boxObj), renderer, tr("Boxplot"));  // cn:箱线图
}

/**
 * @brief 帕累托图：首个数值列的取值分箱 → 按频数降序 → 分布率柱 + 累计分布线
 *
 * 两条序列共用 0~1 的纵轴（分布率与累计分布都是比例），不需要第二纵轴。
 */
bool DADataStatsWidget::renderPareto(DAChartWidget* chart)
{
    if (!chart) {
        return false;
    }
    // 首个数值列（多列选区时帕累托图针对第一列数值列）
    int columnIndex = -1;
    for (int i = 0; i < mRequest.columns.size(); ++i) {
        if (mRequest.columns.at(i).isNumeric()) {
            columnIndex = i;
            break;
        }
    }
    if (columnIndex < 0) {
        return false;
    }
    const DA::DATableChartColumn& column = mRequest.columns.at(columnIndex);

    DAChartSeriesSampler::SampleInfo info;
    DAChartSeriesSampler::SampleBudget budget;
    const QVector< double > values = DAChartSeriesSampler::sampleValues(
        mRequest, column.dataIndex, DAChartSeriesSampler::kMaxHistogramValues, &info, &budget);
    if (values.isEmpty()) {
        return false;
    }

    QVariantList valuesArg;
    valuesArg.reserve(values.size());
    for (double v : std::as_const(values)) {
        valuesArg.append(v);
    }
    QVariantMap args;
    args[ QStringLiteral("bins") ] = kParetoBins;
    pybind11::dict result          = DAPyScripts::getStatistics().computePareto(valuesArg, args);
    if (result.is_none() || pybind11::len(result) == 0) {
        return false;
    }

    const QVector< double > lower      = dictToDoubleVector(result, "bin_lower");
    const QVector< double > upper      = dictToDoubleVector(result, "bin_upper");
    const QVector< double > rates      = dictToDoubleVector(result, "rates");
    const QVector< double > cumulative = dictToDoubleVector(result, "cumulative");
    const int binCount                 = qMin(lower.size(), rates.size());
    if (binCount <= 0 || upper.size() < binCount || cumulative.size() < binCount) {
        return false;
    }

    QStringList labels;
    labels.reserve(binCount);
    for (int i = 0; i < binCount; ++i) {
        labels.append(QStringLiteral("[%1, %2)").arg(formatValue(lower.at(i), false), formatValue(upper.at(i), false)));
    }

    DAChartPlotRenderer renderer(chart);

    // 分布率柱（主蓝）
    if (mParetoShowRate) {
        DA::DABarChartData barData;
        barData.categories = labels;
        barData.values     = rates.mid(0, binCount);
        QVariantMap style;
        style[ QStringLiteral("title") ]     = tr("Distribution Rate");  // cn:分布率
        style[ QStringLiteral("color") ]     = QColor(0x52, 0x80, 0xC1);
        style[ QStringLiteral("fillColor") ] = QColor(0x52, 0x80, 0xC1, 150);
        renderer.renderBarChart(barData, style);
    }

    // 累计分布折线（橙红，与柱体区分）
    if (mParetoShowCumulative) {
        QPolygonF curve;
        curve.reserve(binCount);
        for (int i = 0; i < binCount; ++i) {
            curve.append(QPointF(static_cast< double >(i), cumulative.at(i)));
        }
        QVariantMap style;
        style[ QStringLiteral("title") ]      = tr("Cumulative Distribution");  // cn:累计分布
        style[ QStringLiteral("color") ]      = QColor(0xCE, 0x60, 0x43);
        style[ QStringLiteral("curveStyle") ] = QStringLiteral("Lines");
        style[ QStringLiteral("symbol") ]     = QStringLiteral("Ellipse");
        style[ QStringLiteral("symbolSize") ] = 6;
        renderer.renderCurve(curve, style);
    }

    // 只画累计线时柱状图不会设置类别轴，这里补一次
    if (!mParetoShowRate) {
        QVector< double > positions;
        positions.reserve(binCount);
        for (int i = 0; i < binCount; ++i) {
            positions.append(static_cast< double >(i));
        }
        renderer.setXBottomCategoryScale(positions, labels, -0.5, binCount - 0.5);
    }

    renderer.enableLegend(mParetoShowRate && mParetoShowCumulative);
    renderer.setAxisLabel(QwtAxis::XBottom, columnDisplayName(column));
    renderer.setAxisLabel(QwtAxis::YLeft, tr("Proportion"));  // cn:比例
    return true;
}

/**
 * @brief 按当前请求与勾选状态重算并刷新表格与统计图
 */
void DADataStatsWidget::refresh()
{
    DAWaitCursorScoped waitCursor;

    const QStringList activeKeys = activeStatKeys();
    if (activeKeys.isEmpty()) {
        clearAll();
        mLastMessage = tr("Select at least one statistic in the ribbon");  // cn:请在 Ribbon 的统计参数中选择至少一个统计量
        mNoticeLabel->setText(mLastMessage);
        mNoticeLabel->show();
        return;
    }

    pybind11::dict result;
    QStringList resultLabels;
    SampleNotice notice;
    QString noticeText;
    pybind11::gil_scoped_acquire gil;

    if (!computeDescribe(&result, &resultLabels, &notice)) {
        clearAll();
        daWarning << tr("Unable to get statistics for the selected columns");  // cn:无法获取所选列的统计信息
        return;
    }

    // ---- 表格：Python 返回统计行（key / values / is_int / is_text）----
    struct StatRow
    {
        QString key;
        bool isInt  = false;
        bool isText = false;
        QVector< QString > cells;  // 数字=格式化文本；文本=机器键
    };
    QList< StatRow > rows;
    pybind11::str statsKey("stats");
    if (result.contains(statsKey)) {
        pybind11::object statsObj = result[ statsKey ];
        if (!statsObj.is_none() && pybind11::isinstance< pybind11::list >(statsObj)) {
            for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(statsObj)) {
                if (!pybind11::isinstance< pybind11::dict >(item)) {
                    continue;
                }
                auto sd      = pybind11::cast< pybind11::dict >(item);
                StatRow row;
                row.key    = sd[ "key" ].cast< QString >();
                row.isInt  = sd[ "is_int" ].cast< bool >();
                row.isText = sd[ "is_text" ].cast< bool >();
                pybind11::object valuesObj = sd[ "values" ];
                if (!valuesObj.is_none() && pybind11::isinstance< pybind11::list >(valuesObj)) {
                    for (pybind11::handle v : pybind11::reinterpret_borrow< pybind11::iterable >(valuesObj)) {
                        if (v.is_none()) {
                            row.cells.append(QStringLiteral("\u2014"));
                        } else if (row.isText) {
                            row.cells.append(pybind11::cast< QString >(v));
                        } else {
                            row.cells.append(formatValue(pybind11::cast< double >(v), row.isInt));
                        }
                    }
                }
                rows.append(row);
            }
        }
    }
    if (rows.isEmpty() || resultLabels.isEmpty()) {
        clearAll();
        daWarning << tr("Unable to get statistics for the selected columns");  // cn:无法获取所选列的统计信息
        return;
    }

    // 表头：统计量 | 列名…（每列列宽可拖动调整）
    // 记录旧表头：表头没变时保留用户手动调整的列宽，变了才重算列宽
    QStringList oldHeader;
    for (int c = 0; c < mTable->columnCount(); ++c) {
        if (const QTableWidgetItem* h = mTable->horizontalHeaderItem(c)) {
            oldHeader << h->text();
        } else {
            oldHeader << QString();
        }
    }
    mTable->clear();
    const int columnCount = 1 + resultLabels.size();
    mTable->setColumnCount(columnCount);
    QStringList headerLabels;
    headerLabels.reserve(columnCount);
    headerLabels << tr("Statistic");  // cn:统计量
    for (const QString& l : std::as_const(resultLabels)) {
        headerLabels << l;
    }
    mTable->setHorizontalHeaderLabels(headerLabels);
    mTable->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r) {
        const StatRow& row         = rows.at(r);
        QTableWidgetItem* nameItem = new QTableWidgetItem(statDisplayName(row.key));
        nameItem->setData(Qt::UserRole, row.key);  // 记录统计键，retranslate 时重建显示名
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        mTable->setItem(r, 0, nameItem);
        for (int c = 0; c < row.cells.size() && c < resultLabels.size(); ++c) {
            const QString machineValue = row.cells.at(c);
            QTableWidgetItem* item     = new QTableWidgetItem(
                row.isText ? statTextValue(row.key, machineValue) : machineValue);
            if (row.isText) {
                item->setData(Qt::UserRole + 1, machineValue);  // 记录机器键，retranslate 时重翻译
                item->setTextAlignment(Qt::AlignCenter);
            } else {
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            mTable->setItem(r, c + 1, item);
        }
    }
    // 表头（列集）变化时才重算列宽，否则保留用户手动拖动的列宽
    if (oldHeader != headerLabels) {
        mTable->resizeColumnsToContents();
    }

    // ---- 统计图 ----
    mChart->detachItems(QwtPlotItem::Rtti_PlotItem, true);
    resetChartAxes(mChart);
    const bool plotOk = renderPlot(mChart, result);
    mChart->replot();

    mHasResult   = true;
    mDataChanged = false;

    // 提示条：无法绘图 > 帕累托选项全关 > 样本受限 > 采样近似
    if (!plotOk) {
        noticeText = tr("The selected columns cannot be plotted");  // cn:当前选区无法绘图
    } else if (mPlotType == PlotType::Pareto && !mParetoShowRate && !mParetoShowCumulative) {
        noticeText = tr("Select distribution rate or cumulative distribution in the ribbon");  // cn:请在 Ribbon 中选择分布率或累计分布
    } else if (notice.limited) {
        noticeText = tr("Sampling stopped early due to the data size limit (%1 of %2 rows sampled)")
                         .arg(notice.sampledRows)
                         .arg(notice.totalRows);  // cn:数据量达到采样上限提前停止（已采样 %1 / 共 %2 行）
    } else if (notice.approximate) {
        noticeText = tr("Statistics are based on a sampled subset (%1 of %2 rows)")
                         .arg(notice.sampledRows)
                         .arg(notice.totalRows);  // cn:统计基于采样数据（%1 / 共 %2 行）
    }
    mLastMessage = noticeText;
    if (mLastMessage.isEmpty()) {
        mNoticeLabel->hide();
    } else {
        mNoticeLabel->setText(mLastMessage);
        mNoticeLabel->show();
    }
}

/**
 * @brief 清空表格与图表（无数据/出错时）
 */
void DADataStatsWidget::clearAll()
{
    mHasResult   = false;
    mDataChanged = false;
    mLastMessage.clear();
    mNoticeLabel->hide();
    mTable->clear();
    mTable->setColumnCount(1);
    mTable->setRowCount(0);
    mTable->setHorizontalHeaderLabels({ tr("Statistic") });  // cn:统计量
    mChart->detachItems(QwtPlotItem::Rtti_PlotItem, true);
    resetChartAxes(mChart);
    mChart->replot();
}

/**
 * @brief 把当前统计图转入绘图区（新建 Figure 标签页）
 *
 * 「创建 figure + 建 chart + 挂图元」打包成新图自己的撤销宏，一次 Ctrl+Z
 * 撤销整个转入；图元在目标 chart 上按当前设置重新构建（不复用统计窗口的 chart）。
 */
bool DADataStatsWidget::transferToFigure(DAChartOperateWidget* chartOperate)
{
    if (!chartOperate) {
        daWarning << tr("The chart area is not available");  // cn:绘图区不可用
        return false;
    }
    if (!hasResult()) {
        daWarning << tr("There is no statistics plot to transfer");  // cn:没有可转入绘图区的统计图
        return false;
    }

    DAWaitCursorScoped waitCursor;

    DAFigureWidget* figure = chartOperate->createFigure();
    if (!figure) {
        daWarning << tr("Failed to create a figure");  // cn:创建绘图失败
        return false;
    }
    QUndoStack* stack = figure->getUndoStack();
    if (stack) {
        stack->beginMacro(tr("statistics plot"));  // cn:统计绘图
        stack->push(new DAChartOperateWidgetCommandDetachFigure(chartOperate, figure, true));
    }
    DAChartWidget* chart = figure->createChart_();
    if (!chart) {
        if (stack) {
            stack->endMacro();
            stack->undo();
        }
        chartOperate->removeFigure(figure, true);
        daWarning << tr("Failed to create a chart");  // cn:创建子图失败
        return false;
    }

    // 图元构建：统计图重新渲染到目标 chart（箱线图复用 describe 结果）
    bool plotOk = false;
    {
        pybind11::gil_scoped_acquire gil;
        pybind11::dict describe;
        QStringList labels;
        SampleNotice notice;
        computeDescribe(&describe, &labels, &notice);
        plotOk = renderPlot(chart, describe);
    }
    if (!plotOk) {
        if (stack) {
            stack->endMacro();
            stack->undo();  // 回滚整个宏：chart + figure 一起撤掉
        }
        chartOperate->removeFigure(figure, true);
        daWarning << tr("The selected columns cannot be plotted");  // cn:当前选区无法绘图
        return false;
    }

    // 登记新图元的撤销命令（与快速绘图「转入绘图区」同一口径）
    const QwtPlotItemList items = chart->itemList();
    for (QwtPlotItem* item : std::as_const(items)) {
        if (item && item->rtti() != QwtPlotItem::Rtti_PlotLegend) {
            figure->addItem_(chart, item, true);
        }
    }
    if (stack) {
        stack->endMacro();
    }
    chart->replot();
    chartOperate->setCurrentFigure(figure);
    daInfo << tr("The statistics plot has been transferred to the chart area");  // cn:统计图已转入绘图区
    return true;
}

/**
 * @brief 翻译：动作文本/提示、分位数菜单、表格首列与文本统计值（数值不重算）
 */
void DADataStatsWidget::retranslateUi()
{
    // 统计项动作（图标 + 文本 + 提示）
    for (int i = 0; i < kStatActionDefCount; ++i) {
        const StatActionDef& def = kStatActionDefs[ i ];
        const QString key        = QString::fromLatin1(def.statKey);
        if (QAction* act = command(def.cmd)) {
            act->setText(statDisplayName(key));
            act->setToolTip(statDescription(key));
            act->setIcon(QIcon(statIconPath(key)));
        }
    }
    // 分位数动作与菜单
    if (QAction* percentile = command(Command::Percentile)) {
        percentile->setText(tr("Percentile"));  // cn:分位数
        percentile->setToolTip(tr("Show the selected percentile rows"));  // cn:显示选中的分位数行
        percentile->setIcon(QIcon(QString::fromLatin1(kIconPercentile)));
    }
    rebuildPercentileMenu();
    // 检验动作
    for (int i = 0; i < kStatGroupCount; ++i) {
        if (QAction* act = command(kStatGroups[ i ].cmd)) {
            act->setText(groupDisplayName(i));
            act->setToolTip(groupDescription(i));
        }
    }
    // 刷新
    if (QAction* refresh = command(Command::Refresh)) {
        refresh->setText(tr("Refresh"));  // cn:刷新
        refresh->setToolTip(tr("Recompute with the current data"));  // cn:按当前数据重新计算
        refresh->setIcon(QIcon(QString::fromLatin1(kIconRefresh)));
    }
    // 绘图类型
    if (QAction* act = command(Command::PlotBoxplot)) {
        act->setText(tr("Boxplot"));  // cn:箱线图
        act->setToolTip(tr("Plot one box per selected column"));  // cn:每个选中列一个箱体
        act->setIcon(QIcon(QString::fromLatin1(kIconBoxplot)));
    }
    if (QAction* act = command(Command::PlotPareto)) {
        act->setText(tr("Pareto"));  // cn:帕累托图
        act->setToolTip(tr("Distribution rate and cumulative distribution of the first numeric column"));  // cn:首个数值列的分布率与累计分布
        act->setIcon(QIcon(QString::fromLatin1(kIconPareto)));
    }
    // 帕累托图选项
    if (QAction* act = command(Command::ParetoRate)) {
        act->setText(tr("Distribution Rate"));  // cn:分布率
        act->setToolTip(tr("Show the distribution rate bars"));  // cn:显示分布率柱
        act->setIcon(QIcon(QString::fromLatin1(kIconRate)));
    }
    if (QAction* act = command(Command::ParetoCumulative)) {
        act->setText(tr("Cumulative"));  // cn:累计分布
        act->setToolTip(tr("Show the cumulative distribution curve"));  // cn:显示累计分布折线
        act->setIcon(QIcon(QString::fromLatin1(kIconCumulative)));
    }
    // 转入绘图区
    if (QAction* act = command(Command::TransferToFigure)) {
        act->setText(tr("Transfer to Chart Area"));  // cn:转入绘图区
        act->setToolTip(tr("Create a new figure with the current statistics plot"));  // cn:用当前统计图新建一个绘图
        act->setIcon(QIcon(QString::fromLatin1(kIconTransfer)));
    }

    if (!mHasResult) {
        mTable->setHorizontalHeaderLabels({ tr("Statistic") });  // cn:统计量
        return;
    }
    // 表头：首列翻译，列名保持
    QStringList headerLabels;
    headerLabels << tr("Statistic");  // cn:统计量
    for (int c = 1; c < mTable->columnCount(); ++c) {
        if (const QTableWidgetItem* h = mTable->horizontalHeaderItem(c)) {
            headerLabels << h->text();
        }
    }
    mTable->setHorizontalHeaderLabels(headerLabels);
    // 首列统计名与文本统计值重翻译（UserRole 存统计键、UserRole+1 存机器值）
    for (int r = 0; r < mTable->rowCount(); ++r) {
        QTableWidgetItem* nameItem = mTable->item(r, 0);
        if (!nameItem) {
            continue;
        }
        const QString key = nameItem->data(Qt::UserRole).toString();
        if (!key.isEmpty()) {
            nameItem->setText(statDisplayName(key));
        }
        if (key == "norm_verdict" || key == "norm_test") {
            for (int c = 1; c < mTable->columnCount(); ++c) {
                QTableWidgetItem* item = mTable->item(r, c);
                if (!item) {
                    continue;
                }
                const QString machineValue = item->data(Qt::UserRole + 1).toString();
                if (!machineValue.isEmpty()) {
                    item->setText(statTextValue(key, machineValue));
                }
            }
        }
    }
    if (!mLastMessage.isEmpty()) {
        mNoticeLabel->setText(mLastMessage);
    }
}
