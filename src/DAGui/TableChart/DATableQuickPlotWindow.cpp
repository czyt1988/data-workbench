#include "DATableQuickPlotWindow.h"
#include "DATableChartPlanner.h"
#include "DAChartQuickPlot.h"
#include "DAChartWidget.h"
#include "DALogCategory.h"
// Qt
#include <QButtonGroup>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>
#include <utility>

namespace DA
{

namespace
{
// 浮窗默认尺寸与边距
constexpr int kDefaultWidth = 560;
constexpr int kDefaultHeight = 430;
constexpr int kAnchorOffset = 12;

// 图型按钮携带的枚举值
const char* const kTypeValueProperty = "daQuickPlotType";
}  // namespace

DATableQuickPlotWindow::DATableQuickPlotWindow(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("DATableQuickPlotWindow"));
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
    buildUi();
    hide();
}

DATableQuickPlotWindow::~DATableQuickPlotWindow()
{
}

/**
 * @brief 无法出图原因的展示文案
 */
QString DATableQuickPlotWindow::rejectReasonText(DATableChartRejectReason reason)
{
    switch (reason) {
    case DATableChartRejectReason::None:
        return QString();
    case DATableChartRejectReason::EmptySelection:
        return tr("Select one or more columns first");  // cn:请先选择一列或多列
    case DATableChartRejectReason::NotTableData:
        return tr("The current data is not a table and cannot be plotted");  // cn:当前数据不是表格型数据，无法快速绘图
    case DATableChartRejectReason::NoNumericColumn:
        return tr("The selection has no numeric column");  // cn:当前选区没有数值列
    case DATableChartRejectReason::OnlyTimeColumn:
        return tr("Select a numeric column together with the time column");  // cn:请再选一列数值列
    case DATableChartRejectReason::UnsupportedCombination:
        return tr("This combination of columns is not supported yet. "
                  "Select one category column plus one numeric column, or only numeric columns");  // cn:当前列组合暂不支持，请选择「一个分类列 + 一个数值列」或纯数值列
    }
    return QString();
}

/**
 * @brief 图型显示名
 */
QString DATableQuickPlotWindow::chartTypeText(DATableChartType type)
{
    switch (type) {
    case DATableChartType::Line:
        return tr("Line");  // cn:折线
    case DATableChartType::Scatter:
        return tr("Scatter");  // cn:散点
    case DATableChartType::Histogram:
        return tr("Histogram");  // cn:直方图
    case DATableChartType::Boxplot:
        return tr("Box plot");  // cn:箱线图
    case DATableChartType::Bar:
        return tr("Bar");  // cn:柱状
    case DATableChartType::MultiLine:
        return tr("Multi-series line");  // cn:多序列折线
    case DATableChartType::None:
        break;
    }
    return QString();
}

/**
 * @brief 聚合口径显示名
 */
QString DATableQuickPlotWindow::aggregationText(DATableChartAggregation aggregation)
{
    switch (aggregation) {
    case DATableChartAggregation::Mean:
        return tr("Mean");  // cn:平均值
    case DATableChartAggregation::Median:
        return tr("Median");  // cn:中位数
    case DATableChartAggregation::Sum:
        return tr("Sum");  // cn:求和
    case DATableChartAggregation::Count:
        return tr("Count");  // cn:计数
    case DATableChartAggregation::Std:
        return tr("Std");  // cn:标准差
    case DATableChartAggregation::Var:
        return tr("Var");  // cn:方差
    case DATableChartAggregation::Min:
        return tr("Min");  // cn:最小值
    case DATableChartAggregation::Max:
        return tr("Max");  // cn:最大值
    }
    return QString();
}

/**
 * @brief 搭建浮窗界面
 */
void DATableQuickPlotWindow::buildUi()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    // 标题行：列名摘要 + 关闭
    mTitleBar = new QWidget(this);
    mTitleBar->installEventFilter(this);
    QHBoxLayout* titleLayout = new QHBoxLayout(mTitleBar);
    titleLayout->setContentsMargins(2, 0, 0, 0);
    titleLayout->setSpacing(4);
    mTitleLabel = new QLabel(mTitleBar);
    QFont titleFont = mTitleLabel->font();
    titleFont.setBold(true);
    mTitleLabel->setFont(titleFont);
    mTitleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    titleLayout->addWidget(mTitleLabel, 1);
    QToolButton* closeButton = new QToolButton(mTitleBar);
    closeButton->setText(QStringLiteral("\u2715"));
    closeButton->setAutoRaise(true);
    closeButton->setToolTip(tr("Close"));  // cn:关闭
    titleLayout->addWidget(closeButton);
    root->addWidget(mTitleBar);
    connect(closeButton, &QToolButton::clicked, this, &DATableQuickPlotWindow::closeRequested);

    // 图型切换
    mTypeBar = new QWidget(this);
    QHBoxLayout* typeLayout = new QHBoxLayout(mTypeBar);
    typeLayout->setContentsMargins(0, 0, 0, 0);
    typeLayout->setSpacing(2);
    typeLayout->addStretch(1);
    root->addWidget(mTypeBar);

    // 数据口径：聚合 + X 轴
    QWidget* optionBar = new QWidget(this);
    QHBoxLayout* optionLayout = new QHBoxLayout(optionBar);
    optionLayout->setContentsMargins(0, 0, 0, 0);
    optionLayout->setSpacing(4);
    mAggregationLabel = new QLabel(tr("Aggregation:"), optionBar);  // cn:聚合
    mAggregationCombo = new QComboBox(optionBar);
    const QList< DATableChartAggregation > aggregations = DATableChartPlanner::allAggregations();
    for (DATableChartAggregation agg : aggregations) {
        mAggregationCombo->addItem(aggregationText(agg), static_cast< int >(agg));
    }
    mXAxisLabel = new QLabel(tr("X axis:"), optionBar);  // cn:X 轴
    mXAxisCombo  = new QComboBox(optionBar);
    optionLayout->addWidget(mAggregationLabel);
    optionLayout->addWidget(mAggregationCombo);
    optionLayout->addSpacing(8);
    optionLayout->addWidget(mXAxisLabel);
    optionLayout->addWidget(mXAxisCombo);
    optionLayout->addStretch(1);
    root->addWidget(optionBar);
    connect(mAggregationCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DATableQuickPlotWindow::onAggregationChanged);
    connect(mXAxisCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DATableQuickPlotWindow::onXAxisChanged);

    // 图表区
    mChart = new DAChartWidget(this);
    mChart->setMinimumHeight(200);
    root->addWidget(mChart, 1);

    // 说明与错误
    mNoticeLabel = new QLabel(this);
    mNoticeLabel->setWordWrap(true);
    mNoticeLabel->setStyleSheet(QStringLiteral("color: palette(dark);"));
    root->addWidget(mNoticeLabel);
    mErrorLabel = new QLabel(this);
    mErrorLabel->setWordWrap(true);
    mErrorLabel->setStyleSheet(QStringLiteral("color: palette(bright-text);"));
    mErrorLabel->hide();
    root->addWidget(mErrorLabel);

    // 底部：更新选区 / 刷新 / 转入绘图区
    QWidget* footer = new QWidget(this);
    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, 0, 0, 0);
    footerLayout->setSpacing(4);
    QPushButton* updateButton = new QPushButton(tr("Update to current selection"), footer);  // cn:更新为当前选区
    mRefreshButton            = new QPushButton(tr("Refresh"), footer);  // cn:刷新
    mRefreshButton->hide();
    mTransferButton = new QToolButton(footer);
    mTransferButton->setText(tr("Transfer to chart area"));  // cn:转入绘图区
    mTransferButton->setPopupMode(QToolButton::MenuButtonPopup);
    mTransferButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    QMenu* transferMenu = new QMenu(mTransferButton);
    QAction* newFigureAction = transferMenu->addAction(tr("New figure tab"));  // cn:新建绘图标签页
    QAction* appendAction    = transferMenu->addAction(tr("Append to current chart"));  // cn:追加到当前图表
    mTransferButton->setMenu(transferMenu);
    footerLayout->addWidget(updateButton);
    footerLayout->addWidget(mRefreshButton);
    footerLayout->addStretch(1);
    footerLayout->addWidget(mTransferButton);
    root->addWidget(footer);
    connect(updateButton, &QPushButton::clicked, this, &DATableQuickPlotWindow::onUpdateClicked);
    connect(mRefreshButton, &QPushButton::clicked, this, &DATableQuickPlotWindow::onRefreshClicked);
    connect(mTransferButton, &QToolButton::clicked, this, &DATableQuickPlotWindow::onTransferClicked);
    connect(newFigureAction, &QAction::triggered, this, &DATableQuickPlotWindow::onTransferNewFigure);
    connect(appendAction, &QAction::triggered, this, &DATableQuickPlotWindow::onTransferAppendCurrent);
}

/**
 * @brief 按图型计划重建图型切换按钮
 */
void DATableQuickPlotWindow::rebuildTypeButtons()
{
    if (!mTypeBar) {
        return;
    }
    // 清掉旧按钮（保留末尾的 stretch）；先移出布局再延迟删除，避免在按钮自身
    // 的 clicked 信号处理流程里立刻销毁它
    QHBoxLayout* layout = qobject_cast< QHBoxLayout* >(mTypeBar->layout());
    if (!layout) {
        return;
    }
    const QList< QToolButton* > oldButtons = mTypeBar->findChildren< QToolButton* >();
    for (QToolButton* button : oldButtons) {
        layout->removeWidget(button);
        button->hide();
        button->deleteLater();
    }
    const DATableChartPlan plan = DATableChartPlanner::plan(mRequest);
    mIgnoreTypeSignals           = true;
    for (DATableChartType type : std::as_const(plan.types)) {
        QToolButton* button = new QToolButton(mTypeBar);
        button->setText(chartTypeText(type));
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setProperty(kTypeValueProperty, static_cast< int >(type));
        button->setChecked(type == mSpec.type);
        connect(button, &QToolButton::clicked, this, [this, type]() { onChartTypeClicked(static_cast< int >(type)); });
        layout->insertWidget(layout->count() - 1, button);
    }
    mIgnoreTypeSignals = false;
}

/**
 * @brief 按当前规格刷新数据口径控件（聚合/X 轴）
 */
void DATableQuickPlotWindow::refreshOptionWidgets()
{
    const bool showAggregation = (mSpec.type == DATableChartType::Bar) && !mSpec.valueColumns.isEmpty();
    mAggregationLabel->setVisible(showAggregation);
    mAggregationCombo->setVisible(showAggregation);
    if (showAggregation) {
        mIgnoreTypeSignals = true;
        mAggregationCombo->setCurrentIndex(
            mAggregationCombo->findData(static_cast< int >(mSpec.aggregation)));
        mIgnoreTypeSignals = false;
    }

    const QList< DATableChartXOption > options = DATableChartPlanner::xAxisCandidates(mRequest, mSpec.type);
    const bool showX                           = !options.isEmpty();
    mXAxisLabel->setVisible(showX);
    mXAxisCombo->setVisible(showX);
    if (showX) {
        mIgnoreTypeSignals = true;
        mXAxisCombo->clear();
        const bool wantCategory = (mSpec.type == DATableChartType::Bar);
        int currentIndex        = 0;
        for (int i = 0; i < options.size(); ++i) {
            const DATableChartXOption& option = options.at(i);
            QString text;
            if (option.isRowIndex()) {
                text = tr("Row number");  // cn:行号
            } else {
                text = mRequest.columns.value(option.column).name;
                if (option.asCategory) {
                    text += tr(" (as category)");  // cn:（作为分类）
                }
            }
            mXAxisCombo->addItem(text, i);
            if (wantCategory) {
                if (option.asCategory && option.column == mSpec.categoryColumn) {
                    currentIndex = i;
                }
            } else if (!option.asCategory && option.column == mSpec.xColumn) {
                currentIndex = i;
            }
        }
        mXAxisCombo->setCurrentIndex(currentIndex);
        mIgnoreTypeSignals = false;
    }
}

/**
 * @brief 按当前请求与规格重画
 */
void DATableQuickPlotWindow::renderNow()
{
    mErrorLabel->hide();
    mErrorLabel->clear();
    const QString pendingNotice = mPendingNotice;
    mPendingNotice.clear();

    DATableChartBuildInfo info;
    QString error;
    const bool ok = DAChartQuickPlot::render(mRequest, mSpec, mChart.data(), &info, &error, true, nullptr);
    if (!ok) {
        setError(error);
        mLastMessage = error;
        if (mChart) {
            mChart->replot();
        }
        return;
    }

    QStringList parts;
    if (!pendingNotice.isEmpty()) {
        parts.append(pendingNotice);
    }
    if (info.sampled) {
        parts.append(tr("Based on %1 of %2 sampled rows")
                         .arg(info.sampledRows)
                         .arg(info.totalRows));  // cn:基于 %1 / %2 行采样
    }
    if (info.approximate) {
        parts.append(tr("Aggregation uses the sample"));  // cn:聚合结果基于采样
    }
    if (info.droppedMissing > 0) {
        parts.append(tr("Ignored %1 rows with missing values").arg(info.droppedMissing));  // cn:已忽略 %1 行缺失值
    }
    if (info.displayedCategories > 0 && info.totalCategories > info.displayedCategories) {
        parts.append(tr("Showing top %1 of %2 categories")
                         .arg(info.displayedCategories)
                         .arg(info.totalCategories));  // cn:显示前 %1 类 / 共 %2 类
    }
    if (mDataChanged) {
        parts.append(tr("The data has changed, click Refresh to redraw"));  // cn:数据已变化，点击刷新
    }
    setNotice(parts.join(QStringLiteral("  \u00b7  ")));

    // 标题：列名摘要
    QStringList names;
    for (const DATableChartColumn& column : std::as_const(mRequest.columns)) {
        QString name = column.name;
        if (column.occurrence > 0) {
            name += QStringLiteral(" (%1)").arg(column.occurrence + 1);
        }
        names.append(name);
    }
    mTitleLabel->setText(names.join(QStringLiteral(" + ")));
    mLastMessage.clear();
}

/**
 * @brief 设置提示文案
 */
void DATableQuickPlotWindow::setNotice(const QString& notice)
{
    mNoticeLabel->setText(notice);
    mNoticeLabel->setVisible(!notice.isEmpty());
}

/**
 * @brief 设置错误文案（浮窗保持打开，不弹模态框）
 */
void DATableQuickPlotWindow::setError(const QString& error)
{
    mErrorLabel->setText(error);
    mErrorLabel->setVisible(!error.isEmpty());
    daWarning << error;
}

/**
 * @brief 打开/刷新浮窗
 */
void DATableQuickPlotWindow::showForRequest(const DATableChartRequest& request, const QRect& anchorInParent)
{
    const DATableChartPlan plan = DATableChartPlanner::plan(request);
    if (!plan.isPlottable()) {
        // 浮窗还没打开时（首次触发）不出现，只把原因交给消息区；
        // 已打开时保持当前请求与图不变，只显示可读原因
        mLastMessage = rejectReasonText(plan.reject);
        if (isVisible()) {
            setError(mLastMessage);
        }
        return;
    }
    mRequest     = request;
    mSpec        = plan.defaultSpec;
    mDataChanged = false;
    mRefreshButton->hide();
    rebuildTypeButtons();
    refreshOptionWidgets();
    renderNow();
    if (!isVisible()) {
        resize(kDefaultWidth, kDefaultHeight);
        placeNearAnchor(anchorInParent);
        show();
    }
    raise();
    setFocus(Qt::OtherFocusReason);
}

/**
 * @brief 按连续性规则换成新选区
 */
void DATableQuickPlotWindow::updateRequest(const DATableChartRequest& request)
{
    const DATableChartPlan plan = DATableChartPlanner::plan(request);
    if (!plan.isPlottable()) {
        // 保持当前已画的请求与规格（转移按钮仍指向屏幕上这张图），只提示新选区不可画
        mPendingNotice.clear();
        mErrorLabel->setText(rejectReasonText(plan.reject));
        mErrorLabel->show();
        mLastMessage = mErrorLabel->text();
        return;
    }

    const DATableChartType previousType = mSpec.type;
    const DATableChartAggregation previousAgg = mSpec.aggregation;
    const int previousTopN = mSpec.categoryTopN;
    const int previousX    = mSpec.xColumn;

    mRequest = request;
    if (plan.types.contains(previousType)) {
        // 保持当前图型，尽量保留聚合/Top N/X 选择
        mSpec = DATableChartPlanner::defaultSpec(request, previousType);
        if (mSpec.type == DATableChartType::Bar && !mSpec.valueColumns.isEmpty()) {
            mSpec.aggregation = previousAgg;
        }
        mSpec.categoryTopN = previousTopN;
        const QList< DATableChartXOption > options = DATableChartPlanner::xAxisCandidates(request, mSpec.type);
        for (const DATableChartXOption& option : options) {
            if (option.column == previousX && !option.asCategory) {
                mSpec.xColumn = previousX;
                break;
            }
        }
        mPendingNotice.clear();
    } else {
        // 新选区不支持当前图型：回落到默认图型并给一句提示
        mSpec          = plan.defaultSpec;
        mPendingNotice = tr("The previous chart type is not available for the new selection, "
                            "switched to the default type");  // cn:新选区不支持原来的图型，已切换为默认图型
    }
    mDataChanged = false;
    mRefreshButton->hide();
    rebuildTypeButtons();
    refreshOptionWidgets();
    renderNow();
}

/**
 * @brief 数据变化提示（不自动重画）
 */
void DATableQuickPlotWindow::notifyDataChanged()
{
    mDataChanged = true;
    mRefreshButton->show();
    QStringList parts;
    parts.append(mNoticeLabel->text());
    parts.append(tr("The data has changed, click Refresh to redraw"));  // cn:数据已变化，点击刷新
    setNotice(parts.join(QStringLiteral("  \u00b7  ")));
}

/**
 * @brief 切换图型：记住聚合/Top N/X 选择
 */
void DATableQuickPlotWindow::switchToType(DATableChartType type)
{
    if (type == DATableChartType::None || type == mSpec.type) {
        return;
    }
    const DATableChartAggregation previousAgg = mSpec.aggregation;
    const int previousTopN = mSpec.categoryTopN;
    const int previousX    = mSpec.xColumn;

    DATableChartSpec next     = DATableChartPlanner::defaultSpec(mRequest, type);
    next.aggregation          = previousAgg;
    next.categoryTopN         = previousTopN;
    const QList< DATableChartXOption > options = DATableChartPlanner::xAxisCandidates(mRequest, type);
    for (const DATableChartXOption& option : options) {
        if (option.column == previousX && !option.asCategory) {
            next.xColumn = previousX;
            break;
        }
    }
    if (!next.isValid()) {
        return;  // 该图型在当前选区缺必要列（如柱状图没有分类轴）时保持原图型
    }
    mSpec = next;
    refreshOptionWidgets();
    rebuildTypeButtons();
    renderNow();
}

void DATableQuickPlotWindow::onChartTypeClicked(int typeValue)
{
    if (mIgnoreTypeSignals) {
        return;
    }
    switchToType(static_cast< DATableChartType >(typeValue));
}

void DATableQuickPlotWindow::onAggregationChanged(int index)
{
    if (mIgnoreTypeSignals || index < 0) {
        return;
    }
    const int aggregationValue = mAggregationCombo->itemData(index).toInt();
    mSpec.aggregation          = static_cast< DATableChartAggregation >(aggregationValue);
    renderNow();
}

void DATableQuickPlotWindow::onXAxisChanged(int index)
{
    if (mIgnoreTypeSignals || index < 0) {
        return;
    }
    const QList< DATableChartXOption > options = DATableChartPlanner::xAxisCandidates(mRequest, mSpec.type);
    const int optionIndex                      = mXAxisCombo->itemData(index).toInt();
    if (optionIndex < 0 || optionIndex >= options.size()) {
        return;
    }
    DATableChartPlanner::applyXOption(mRequest, mSpec, options.at(optionIndex));
    rebuildTypeButtons();
    refreshOptionWidgets();
    renderNow();
}

void DATableQuickPlotWindow::onUpdateClicked()
{
    Q_EMIT currentSelectionRequested();
}

void DATableQuickPlotWindow::onRefreshClicked()
{
    mDataChanged = false;
    mRefreshButton->hide();
    renderNow();
}

void DATableQuickPlotWindow::onTransferClicked()
{
    onTransferNewFigure();
}

void DATableQuickPlotWindow::onTransferNewFigure()
{
    Q_EMIT transferRequested(mRequest, mSpec, false);
}

void DATableQuickPlotWindow::onTransferAppendCurrent()
{
    Q_EMIT transferRequested(mRequest, mSpec, true);
}

/**
 * @brief 把浮窗放到锚点附近（父控件坐标系），并夹在父控件范围内
 */
void DATableQuickPlotWindow::placeNearAnchor(const QRect& anchorInParent)
{
    QWidget* host = parentWidget();
    if (!host) {
        return;
    }
    QPoint target;
    if (anchorInParent.isValid()) {
        target = anchorInParent.bottomLeft() + QPoint(kAnchorOffset, kAnchorOffset);
    } else {
        target = QPoint(host->width() - width() - 24, 24);
    }
    const int maxX = qMax(0, host->width() - width());
    const int maxY = qMax(0, host->height() - height());
    target.setX(qBound(0, target.x(), maxX));
    target.setY(qBound(0, target.y(), maxY));
    move(target);
}

void DATableQuickPlotWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        Q_EMIT closeRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

/**
 * @brief 标题栏拖动（浮窗是表格页内的子控件，拖动即在父控件里移动）
 */
bool DATableQuickPlotWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == mTitleBar) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            QMouseEvent* me = static_cast< QMouseEvent* >(event);
            if (me->button() == Qt::LeftButton) {
                mDragging   = true;
                mDragOffset = me->pos();
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            if (mDragging) {
                QWidget* host = parentWidget();
                if (!host) {
                    break;
                }
                QMouseEvent* me = static_cast< QMouseEvent* >(event);
                QPoint target   = mTitleBar->mapTo(host, me->pos()) - mDragOffset;
                target.setX(qBound(0, target.x(), qMax(0, host->width() - width())));
                target.setY(qBound(0, target.y(), qMax(0, host->height() - height())));
                move(target);
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            if (mDragging) {
                mDragging = false;
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

}  // namespace DA
