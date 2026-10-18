#include "DATableStatsWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include "DADataTableView.h"
#include "Models/DADataTableModel.h"
#include "DATableDataSource.h"
#include "DATableScan.h"
// qt
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

namespace DA
{

namespace
{
constexpr int c_statsDebounceMs  = 350;       // 与 DADialogFindReplace::debounceIntervalMs 同值，同类交互同一常量
constexpr int c_uniqueLimit      = 1000000;   // 唯一值数阈值（行数超过则不计算）
constexpr int c_maxDetailColumns = 8;         // 多列明细的列数上限
constexpr int c_valueMaxChars    = 32;        // 单格原始值截断
}  // namespace

/**
 * @brief 构造函数
 */
DATableStatsWidget::DATableStatsWidget(QWidget* parent) : QWidget(parent)
{
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    mTextLabel = new QLabel(this);
    lay->addWidget(mTextLabel);
    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(c_statsDebounceMs);
    connect(mDebounceTimer, &QTimer::timeout, this, &DATableStatsWidget::refreshNow);
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    hide();
}

DATableStatsWidget::~DATableStatsWidget()
{
}

/**
 * @brief 绑定数据页
 *
 * 同页重复调用只做立即刷新；换页时断开旧页连接（只断本部件的）、停表、
 * 连新页 tableSelectionChanged 并立即刷新（不走去抖）。
 * @param page 数据页（nullptr = 解绑并隐藏）
 */
void DATableStatsWidget::setPage(DADataOperateOfDataFrameWidget* page)
{
    if (mPage == page) {
        refreshNow();
        return;
    }
    if (mPage) {
        disconnect(mPage, &DADataOperateOfDataFrameWidget::tableSelectionChanged, this,
                   &DATableStatsWidget::onSelectionChanged);
    }
    mDebounceTimer->stop();
    mPage = page;
    if (mPage) {
        connect(mPage, &DADataOperateOfDataFrameWidget::tableSelectionChanged, this,
                &DATableStatsWidget::onSelectionChanged);
    }
    refreshNow();
}

/**
 * @brief 当前绑定的数据页
 */
DADataOperateOfDataFrameWidget* DATableStatsWidget::page() const
{
    return mPage;
}

/**
 * @brief 状态栏那一行的完整文本（未省略）
 * @return 文本（未刷新/无选区为空串）
 */
QString DATableStatsWidget::statsText() const
{
    return m_statsText;
}

/**
 * @brief 明细 tooltip 文本
 * @return 富文本（无明细为空串）
 */
QString DATableStatsWidget::detailsTooltipText() const
{
    return m_detailsTooltip;
}

/**
 * @brief 选区变化：重启去抖
 *
 * start 重启 = 拖动框选期间不连发聚合。
 */
void DATableStatsWidget::onSelectionChanged()
{
    mDebounceTimer->start(c_statsDebounceMs);
}

/**
 * @brief 立即刷新（去抖到期、页面切换、测试）
 *
 * 档位：无选区隐藏；多段只给几何计数；单列全指标；多列只放规模、明细进
 * tooltip（c ≤ 8 且行数 ≤ 100 万才请求）；computeColumnStats 返回 nullopt
 * 明说"不支持"。显示档位由返回值 optional 有无决定，C++ 不判类型。
 */
void DATableStatsWidget::refreshNow()
{
    m_statsText.clear();
    m_detailsTooltip.clear();
    if (!mPage) {
        hide();
        rebuildDisplay();
        return;
    }
    int rowBegin = 0, rowEnd = 0, colBegin = 0, colEnd = 0;
    if (!mPage->selectionBoundingBox(&rowBegin, &rowEnd, &colBegin, &colEnd)) {
        hide();
        rebuildDisplay();
        return;
    }
    show();
    const int segCount = mPage->selectionSegmentCount();
    const int rows     = rowEnd - rowBegin + 1;   // 闭区间转行数
    const int cols     = colEnd - colBegin + 1;
    // 「始终」部分：当前单元格引用 + 选区规模
    int curRow = 0, curCol = 0;
    const bool hasCur = mPage->currentActualCell(&curRow, &curCol);
    QString cellRef;
    if (hasCur && rows == 1 && cols == 1 && curRow == rowBegin && curCol == colBegin) {
        // 单格：引用 + 原始值
        QString value;
        if (DADataTableModel* m = qobject_cast< DADataTableModel* >(mPage->getDataTableView()->model())) {
            const QVariant v = m->actualData(curRow, curCol, Qt::EditRole);
            if (v.isValid()) {
                value = v.toString().left(c_valueMaxChars);
            }
        }
        const QString colName = mPage->getDataTableView()->model()
                                    ->headerData(curCol, Qt::Horizontal)
                                    .toString();
        cellRef = tr("Row %1 · %2: %3").arg(curRow + 1).arg(colName).arg(value);  // cn:第 %1 行 · %2：%3
    }
    QString scaleText;
    if (cols == 1) {
        scaleText = tr("%1 rows × %2 cols").arg(rows).arg(cols);  // cn:%1 行 × %2 列
    } else {
        scaleText = tr("%1 cols selected · %2 rows").arg(cols).arg(rows);  // cn:已选 %1 列 · %2 行
    }
    QStringList mainParts;
    if (!cellRef.isEmpty()) {
        mainParts << cellRef;
    }
    if (segCount > 1) {
        // 多段：几何计数 + 说明，不请求统计
        mainParts.clear();  // 多段不显示单格引用（引用对多段无意义）
        mainParts << tr("%1 segments · %2 rows × %3 cols · %4 cells")
                          .arg(segCount)
                          .arg(rows)
                          .arg(cols)
                          .arg(mPage->selectedCellCount());  // cn:%1 段 · %2 行 × %3 列 · %4 格
        m_statsText = mainParts.join(QStringLiteral(" · "));
        m_detailsTooltip = tr("Aggregation is not available for multi-range selections");  // cn:多段选区不提供聚合
        rebuildDisplay();
        return;
    }
    mainParts << scaleText;
    // 请求构造（半开区间）
    const DA::DATableDataSource* src = mPage->data().tableSource();
    std::optional< DATableStatsResult > res;
    if (cols == 1) {
        // a) 单列：请求全指标
        DATableScanRequest req;
        req.rowBegin    = static_cast< std::size_t >(rowBegin);
        req.rowEnd      = static_cast< std::size_t >(rowEnd) + 1;
        req.columns     = { static_cast< std::size_t >(colBegin) };
        req.uniqueLimit = static_cast< std::size_t >(c_uniqueLimit);
        res             = src ? src->computeColumnStats(req) : std::nullopt;
    } else if (cols <= c_maxDetailColumns && rows <= c_uniqueLimit) {
        // b) 多列在限内：请求明细（状态栏只放规模）
        DATableScanRequest req;
        req.rowBegin    = static_cast< std::size_t >(rowBegin);
        req.rowEnd      = static_cast< std::size_t >(rowEnd) + 1;
        for (int c = colBegin; c <= colEnd; ++c) {
            req.columns.push_back(static_cast< std::size_t >(c));
        }
        req.uniqueLimit = static_cast< std::size_t >(c_uniqueLimit);
        res             = src ? src->computeColumnStats(req) : std::nullopt;
    } else {
        // c) 多列超限：不请求
        m_statsText      = mainParts.join(QStringLiteral(" · "));
        m_detailsTooltip = tr("Selection is too large; per-column details were not computed");  // cn:选区过大，未计算明细
        rebuildDisplay();
        return;
    }
    if (!res.has_value()) {
        // 5) 不支持
        m_statsText = mainParts.join(QStringLiteral(" · ")) + QStringLiteral(" · ")
                       + tr("Statistics are not available for the current data source");  // cn:当前数据源不支持选区统计
        m_detailsTooltip.clear();
        rebuildDisplay();
        return;
    }
    const DATableStatsResult& stats = *res;
    if (cols == 1) {
        // 单列：定档（optional 有无；C++ 不判类型）
        const DATableColumnStats& s = stats[ 0 ];
        QStringList metricParts;
        auto fmtNum = [](const std::optional< double >& v) -> QString {
            return v.has_value() ? QString::number(*v, 'g', 6) : QStringLiteral("—");
        };
        metricParts << tr("Count") + QStringLiteral(" ") + QString::number(static_cast< qulonglong >(s.count));  // cn:计数
        metricParts << tr("Null") + QStringLiteral(" ") + QString::number(static_cast< qulonglong >(s.nullCount));  // cn:空值
        if (s.sum.has_value()) {
            // 数值档
            metricParts << tr("Sum") + QStringLiteral(" ") + fmtNum(s.sum);    // cn:求和
            metricParts << tr("Mean") + QStringLiteral(" ") + fmtNum(s.mean);   // cn:均值
            metricParts << tr("Min") + QStringLiteral(" ") + fmtNum(s.min);    // cn:最小值
            metricParts << tr("Max") + QStringLiteral(" ") + fmtNum(s.max);     // cn:最大值
        } else if (s.uniqueCount != kUniqueNotComputed) {
            // 非数值档：唯一值数（未计算显示 —）
            metricParts << tr("Unique") + QStringLiteral(" ") + QString::number(static_cast< qulonglong >(s.uniqueCount));  // cn:唯一值数
        }
        m_statsText = (mainParts + metricParts).join(QStringLiteral(" · "));
        m_detailsTooltip.clear();
    } else {
        // 多列：状态栏只放规模；明细进 tooltip
        m_statsText = mainParts.join(QStringLiteral(" · "));
        QStringList tip;
        tip << tr("<b>Per-column details</b>");  // cn:<b>逐列明细</b>
        for (int i = 0; i < cols && i < static_cast< int >(stats.size()); ++i) {
            const QString colName = mPage->getDataTableView()
                                        ->model()
                                        ->headerData(colBegin + i, Qt::Horizontal)
                                        .toString()
                                        .toHtmlEscaped();
            const DATableColumnStats& s = stats[ i ];
            QStringList parts;
            auto fmtNum = [](const std::optional< double >& v) -> QString {
                return v.has_value() ? QString::number(*v, 'g', 6) : QStringLiteral("—");
            };
            parts << tr("Count") + QStringLiteral(" ") + QString::number(static_cast< qulonglong >(s.count));
            parts << tr("Null") + QStringLiteral(" ") + QString::number(static_cast< qulonglong >(s.nullCount));
            if (s.sum.has_value()) {
                parts << tr("Sum") + QStringLiteral(" ") + fmtNum(s.sum);
                parts << tr("Mean") + QStringLiteral(" ") + fmtNum(s.mean);
                parts << tr("Min") + QStringLiteral(" ") + fmtNum(s.min);
                parts << tr("Max") + QStringLiteral(" ") + fmtNum(s.max);
            } else if (s.uniqueCount != kUniqueNotComputed) {
                parts << tr("Unique") + QStringLiteral(" ") + QString::number(static_cast< qulonglong >(s.uniqueCount));
            }
            tip << colName + QStringLiteral(": ") + parts.join(QStringLiteral(" · "));
        }
        m_detailsTooltip = tip.join(QStringLiteral("<br/>"));
    }
    rebuildDisplay();
}

/**
 * @brief 组装显示（省略只作用于标签，不改变 statsText）
 */
void DATableStatsWidget::rebuildDisplay()
{
    if (m_statsText.isEmpty()) {
        mTextLabel->clear();
        mTextLabel->setToolTip(QString());
    } else {
        const QString elided = QFontMetrics(mTextLabel->font()).elidedText(m_statsText, Qt::ElideRight, width());
        mTextLabel->setText(elided);
        mTextLabel->setToolTip(m_detailsTooltip.isEmpty() ? m_statsText : m_detailsTooltip);
    }
}

/**
 * @brief 宽度变化时重新省略
 */
void DATableStatsWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    rebuildDisplay();
}

}  // end of namespace DA
