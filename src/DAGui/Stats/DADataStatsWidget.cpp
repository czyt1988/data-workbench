#include "DADataStatsWidget.h"
#include "DAChartSeriesSampler.h"
#include "DAChartWidget.h"
#include "DAChartPlotRenderer.h"
#include "DAStatsPlotCoordinator.h"
#include "DAPyScripts.h"
#include "DAPybind11QtCaster.hpp"
#include "DAWaitCursorScoped.h"
#include "DALogCategory.h"

#include "qwt_axis.h"
#include "qwt_plot_item.h"
#include "qwt_scale_draw.h"

#include <pybind11/pybind11.h>

#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVariantList>
#include <QVariantMap>
#include <QVBoxLayout>

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

namespace {

// 箱线图须长度（IQR 倍数），与统计绘图管线 / 快速绘图一致
constexpr double kBoxplotWhisker = 1.5;

// 默认统计集（固定顺序，始终显示；median 即中位数 50%）
constexpr const char* kDefaultStatKeys[] = {
    "count", "mean", "std", "min", "q1", "median", "q3", "max",
};
constexpr int kDefaultStatKeyCount = sizeof(kDefaultStatKeys) / sizeof(kDefaultStatKeys[ 0 ]);

// 可选统计集（「统计量」菜单勾选，顺序即菜单顺序）
constexpr const char* kOptionalStatKeys[] = {
    "missing", "unique", "sum", "var",  "sem",  "mad",   "range", "iqr",
    "mode",     "cv",    "q05", "q95",  "skew", "kurt",
};
constexpr int kOptionalStatKeyCount = sizeof(kOptionalStatKeys) / sizeof(kOptionalStatKeys[ 0 ]);

// 分析开关组（「分析」菜单勾选）：开关 → 打开时追加的统计键
// 显示名走 groupDisplayName 的字面量 tr（lupdate 提取），key 只做身份标识
struct StatGroupDef
{
    const char* key;   ///< 开关标识
    QList< const char* > statKeys;
};
const StatGroupDef kStatGroups[] = {
    { "normality", { "norm_stat", "norm_p", "norm_verdict", "norm_test" } },
    { "ci", { "ci_low", "ci_high" } },
    { "outliers", { "outliers", "outlier_low", "outlier_high" } },
};
constexpr int kStatGroupCount = sizeof(kStatGroups) / sizeof(kStatGroups[ 0 ]);

// pybind11::dict 里取字符串列表
QStringList dictToStringList(const pybind11::dict& d, const char* key)
{
    QStringList res;
    if (!d.contains(key)) {
        return res;
    }
    pybind11::object obj = d[ key ];
    if (obj.is_none() || !pybind11::isinstance< pybind11::list >(obj)) {
        return res;
    }
    try {
        for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(obj)) {
            if (!item.is_none()) {
                res.append(pybind11::cast< QString >(item));
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "DADataStatsWidget: cannot read string list" << key << e.what();
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
    buildUi();
}

DADataStatsWidget::~DADataStatsWidget()
{
}

/**
 * @brief 构建界面：工具栏 + 分割窗口（左表格 / 右箱线图）+ 提示条
 */
void DADataStatsWidget::buildUi()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);

    // 工具栏：统计量 ▾ | 分析 ▾ …… 刷新
    QWidget* bar = new QWidget(this);
    QHBoxLayout* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(4);
    mStatsButton = new QToolButton(bar);
    mStatsButton->setText(tr("Statistics"));  // cn:统计量
    mStatsButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    mStatsButton->setPopupMode(QToolButton::InstantPopup);
    mStatsMenu = new QMenu(mStatsButton);
    mStatsButton->setMenu(mStatsMenu);
    mAnalysisButton = new QToolButton(bar);
    mAnalysisButton->setText(tr("Analysis"));  // cn:分析
    mAnalysisButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    mAnalysisButton->setPopupMode(QToolButton::InstantPopup);
    mAnalysisMenu = new QMenu(mAnalysisButton);
    mAnalysisButton->setMenu(mAnalysisMenu);
    mRefreshButton = new QToolButton(bar);
    mRefreshButton->setText(tr("Refresh"));  // cn:刷新
    mRefreshButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    mRefreshButton->setToolTip(tr("Recompute with the current data"));  // cn:按当前数据重新计算
    barLayout->addWidget(mStatsButton);
    barLayout->addWidget(mAnalysisButton);
    barLayout->addStretch(1);
    barLayout->addWidget(mRefreshButton);
    root->addWidget(bar);
    connect(mRefreshButton, &QToolButton::clicked, this, &DADataStatsWidget::onRefreshClicked);

    // 分割窗口：左统计表格 / 右箱线图
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

    rebuildMenus();
}

/**
 * @brief 重建「统计量/分析」菜单（勾选状态保留在 mEnabledStats / mEnabledGroups）
 */
void DADataStatsWidget::rebuildMenus()
{
    mStatsMenu->clear();
    for (int i = 0; i < kOptionalStatKeyCount; ++i) {
        const QString key = QString::fromLatin1(kOptionalStatKeys[ i ]);
        QAction* act = mStatsMenu->addAction(statDisplayName(key));
        act->setCheckable(true);
        act->setChecked(mEnabledStats.contains(key));
        act->setData(i);  // 记录目录下标，槽里反查
        connect(act, &QAction::toggled, this, &DADataStatsWidget::onOptionalStatToggled);
    }

    mAnalysisMenu->clear();
    for (int i = 0; i < kStatGroupCount; ++i) {
        const QString key = QString::fromLatin1(kStatGroups[ i ].key);
        QAction* act = mAnalysisMenu->addAction(groupDisplayName(i));
        act->setCheckable(true);
        act->setChecked(mEnabledGroups.contains(key));
        act->setData(i);
        connect(act, &QAction::toggled, this, &DADataStatsWidget::onAnalysisGroupToggled);
    }
}

/**
 * @brief 分析开关组的显示名（字面量 tr 便于 lupdate 提取翻译）
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

void DADataStatsWidget::onOptionalStatToggled(bool on)
{
    QAction* act = qobject_cast< QAction* >(sender());
    if (!act) {
        return;
    }
    const int index = act->data().toInt();
    if (index < 0 || index >= kOptionalStatKeyCount) {
        return;
    }
    const QString key = QString::fromLatin1(kOptionalStatKeys[ index ]);
    if (on) {
        mEnabledStats.insert(key);
    } else {
        mEnabledStats.remove(key);
    }
    if (mHasResult) {
        refresh();
    }
}

void DADataStatsWidget::onAnalysisGroupToggled(bool on)
{
    QAction* act = qobject_cast< QAction* >(sender());
    if (!act) {
        return;
    }
    const int index = act->data().toInt();
    if (index < 0 || index >= kStatGroupCount) {
        return;
    }
    const QString key = QString::fromLatin1(kStatGroups[ index ].key);
    if (on) {
        mEnabledGroups.insert(key);
    } else {
        mEnabledGroups.remove(key);
    }
    if (mHasResult) {
        refresh();
    }
}

void DADataStatsWidget::onRefreshClicked()
{
    if (mRequest.isValid()) {
        refresh();
    }
}

/**
 * @brief 当前应计算的统计键（默认集在前保持固定顺序，其后按勾选集）
 */
QStringList DADataStatsWidget::activeStatKeys() const
{
    QStringList keys;
    for (int i = 0; i < kDefaultStatKeyCount; ++i) {
        keys.append(QString::fromLatin1(kDefaultStatKeys[ i ]));
    }
    for (int i = 0; i < kOptionalStatKeyCount; ++i) {
        const QString key = QString::fromLatin1(kOptionalStatKeys[ i ]);
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
 * @brief 统计键 → 表格第一列显示名
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
 * @brief 按当前请求与勾选状态重算并刷新表格与箱线图
 *
 * 内存小表整表精确计算（列按位置传参，重名列消歧）；
 * 惰性/超大表按 DAChartSeriesSampler 采样后走值列表口径，并标注近似。
 */
void DADataStatsWidget::refresh()
{
    DAWaitCursorScoped waitCursor;

    // 数值列（请求里按视图顺序）
    QList< int > numericIdx;
    for (int i = 0; i < mRequest.columns.size(); ++i) {
        if (mRequest.columns.at(i).isNumeric()) {
            numericIdx.append(i);
        }
    }
    if (numericIdx.isEmpty()) {
        clearAll();
        return;
    }

    QStringList labels;
    for (int idx : numericIdx) {
        labels.append(columnDisplayName(mRequest.columns.at(idx)));
    }

    QVariantMap args;
    args[ QStringLiteral("whis") ] = kBoxplotWhisker;
    QVariantList statKeysArg;
    const QStringList activeKeys = activeStatKeys();
    for (const QString& k : activeKeys) {
        statKeysArg.append(k);
    }
    args[ QStringLiteral("stats") ] = statKeysArg;

    pybind11::gil_scoped_acquire gil;
    pybind11::dict result;
    bool approximate                       = false;
    DAChartSeriesSampler::SampleBudget budget;
    DAChartSeriesSampler::SampleInfo mergedInfo;

    if (DAChartSeriesSampler::canAggregateExactly(mRequest)) {
        // 内存小表：整表精确计算
        QVariantList columnsArg;
        for (int idx : numericIdx) {
            columnsArg.append(mRequest.columns.at(idx).dataIndex);
        }
        QVariantList labelsArg;
        for (const QString& l : labels) {
            labelsArg.append(l);
        }
        args[ QStringLiteral("columns") ] = columnsArg;
        args[ QStringLiteral("labels") ]  = labelsArg;
        result                           = DAPyScripts::getStatistics().computeDescribe(mRequest.data.toDataFrame(), args);
    } else {
        // 惰性/超大表：采样后按值列表计算（与快速绘图同一采样口径）
        QVariantMap columns;
        QStringList keys;
        for (int i = 0; i < numericIdx.size(); ++i) {
            DAChartSeriesSampler::SampleInfo info;
            const QVector< double > values = DAChartSeriesSampler::sampleValues(
                mRequest, mRequest.columns.at(numericIdx.at(i)).dataIndex,
                DAChartSeriesSampler::kMaxHistogramValues, &info, &budget);
            mergedInfo.totalRows       = info.totalRows;
            mergedInfo.sampled         = true;
            mergedInfo.sampledRows    += info.sampledRows;
            mergedInfo.droppedMissing += info.droppedMissing;
            mergedInfo.limited         = mergedInfo.limited || info.limited;
            const QString key = QStringLiteral("__v%1").arg(i);
            keys.append(key);
            QVariantList valuesArg;
            valuesArg.reserve(values.size());
            for (double v : values) {
                valuesArg.append(v);
            }
            columns[ key ] = valuesArg;
        }
        QVariantList labelsArg;
        for (const QString& l : labels) {
            labelsArg.append(l);
        }
        // 注意：传给 Python 的列表一律用 QVariantList——QStringList 值的 QVariant
        // 在 QtCaster 里没有对应分支，会退化成 toString() 的空串（列名丢失）
        QVariantList keysArg;
        for (const QString& k : keys) {
            keysArg.append(k);
        }
        args[ QStringLiteral("columns") ] = keysArg;
        args[ QStringLiteral("labels") ]  = labelsArg;
        result                             = DAPyScripts::getStatistics().computeDescribeFromLists(columns, args);
        approximate                        = true;
    }

    if (result.is_none() || pybind11::len(result) == 0) {
        clearAll();
        daWarning << tr("Unable to get statistics for the selected columns");  // cn:无法获取所选列的统计信息
        return;
    }

    // ---- 表格：Python 返回统计行（key / values / is_int / is_text）----
    const QStringList resultLabels = dictToStringList(result, "labels");
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
    for (const QString& l : resultLabels) {
        headerLabels << l;
    }
    mTable->setHorizontalHeaderLabels(headerLabels);
    mTable->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r) {
        const StatRow& row = rows.at(r);
        QTableWidgetItem* nameItem = new QTableWidgetItem(statDisplayName(row.key));
        nameItem->setData(Qt::UserRole, row.key);  // 记录统计键，retranslate 时重建显示名
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        mTable->setItem(r, 0, nameItem);
        for (int c = 0; c < row.cells.size() && c < resultLabels.size(); ++c) {
            const QString machineValue = row.cells.at(c);
            QTableWidgetItem* item = new QTableWidgetItem(
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

    // ---- 箱线图：统计管线唯一实现，x 轴标注列名 ----
    mChart->detachItems(QwtPlotItem::Rtti_PlotItem, true);
    resetChartAxes(mChart);
    pybind11::str boxKey("boxplot");
    if (result.contains(boxKey)) {
        pybind11::object boxObj = result[ boxKey ];
        if (!boxObj.is_none() && pybind11::isinstance< pybind11::dict >(boxObj)) {
            auto boxDict = pybind11::cast< pybind11::dict >(boxObj);
            DAChartPlotRenderer renderer(mChart);
            DAStatsPlotCoordinator::renderBoxplotResult(boxDict, renderer, tr("Boxplot"));  // cn:箱线图
        }
    }
    mChart->replot();

    mHasResult   = true;
    mDataChanged = false;

    // 提示条：样本受限 > 采样近似
    if (mergedInfo.limited) {
        mLastMessage = tr("Sampling stopped early due to the data size limit (%1 of %2 rows sampled)")
                           .arg(mergedInfo.sampledRows)
                           .arg(mergedInfo.totalRows);  // cn:数据量达到采样上限提前停止（已采样 %1 / 共 %2 行）
        mNoticeLabel->setText(mLastMessage);
        mNoticeLabel->show();
    } else if (approximate) {
        mLastMessage = tr("Statistics are based on a sampled subset (%1 of %2 rows)")
                           .arg(mergedInfo.sampledRows)
                           .arg(mergedInfo.totalRows);  // cn:统计基于采样数据（%1 / 共 %2 行）
        mNoticeLabel->setText(mLastMessage);
        mNoticeLabel->show();
    } else {
        mLastMessage.clear();
        mNoticeLabel->hide();
    }
}

/**
 * @brief 清空表格与图表（无数据/出错时）
 */
void DADataStatsWidget::clearAll()
{
    mHasResult    = false;
    mDataChanged  = false;
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
 * @brief 翻译：重建工具栏菜单，表格首列与文本统计值重翻译（数值不重算）
 */
void DADataStatsWidget::retranslateUi()
{
    mStatsButton->setText(tr("Statistics"));  // cn:统计量
    mAnalysisButton->setText(tr("Analysis"));  // cn:分析
    mRefreshButton->setText(tr("Refresh"));  // cn:刷新
    mRefreshButton->setToolTip(tr("Recompute with the current data"));  // cn:按当前数据重新计算
    rebuildMenus();
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
