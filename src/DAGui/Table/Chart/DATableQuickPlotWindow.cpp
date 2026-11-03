#include "Table/Chart/DATableQuickPlotWindow.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "Table/Chart/DAChartQuickPlot.h"
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
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>
#include <utility>

namespace DA
{

namespace
{
// 贴入框架时的推荐尺寸（与原浮窗尺寸一致，图表区面积基本不变）
constexpr int kRecommendedWidth = 560;
constexpr int kRecommendedHeight = 430;

// 图型按钮携带的枚举值
const char* const kTypeValueProperty = "daQuickPlotType";

// 忙碌期间的守卫：禁用内容交互，退出作用域时恢复（异常/提前 return 也能恢复）
class QuickPlotBusyGuard
{
public:
    explicit QuickPlotBusyGuard(QWidget* widget) : mWidget(widget)
    {
        if (mWidget) {
            mWidget->setEnabled(false);
        }
    }
    ~QuickPlotBusyGuard()
    {
        if (mWidget) {
            mWidget->setEnabled(true);
        }
    }
    QuickPlotBusyGuard(const QuickPlotBusyGuard&)            = delete;
    QuickPlotBusyGuard& operator=(const QuickPlotBusyGuard&) = delete;

private:
    QPointer< QWidget > mWidget;
};
}  // namespace

DATableQuickPlotWindow::DATableQuickPlotWindow(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("DATableQuickPlotWindow"));
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(380, 300);
    buildUi();
    hide();
}

DATableQuickPlotWindow::~DATableQuickPlotWindow()
{
}

/**
 * @brief 贴入框架时的推荐尺寸（宿主 attach 用）
 */
QSize DATableQuickPlotWindow::recommendedSize()
{
    return QSize(kRecommendedWidth, kRecommendedHeight);
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

    // 标题栏（列名摘要 + 关闭）由承载框架 DATableChildWindow 绘制，
    // 标题内容经 titleChanged 交给宿主设置

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
    mAppendAction            = transferMenu->addAction(tr("Append to current chart"));  // cn:追加到当前图表
    mTransferButton->setMenu(transferMenu);
    // 没有当前图表时「追加到当前图表」置灰（实施方案 §6.1）
    connect(transferMenu, &QMenu::aboutToShow, this, [ this ]() {
        if (mAppendAction) {
            mAppendAction->setEnabled(mAppendAvailability ? mAppendAvailability() : true);
        }
    });
    footerLayout->addWidget(updateButton);
    footerLayout->addWidget(mRefreshButton);
    footerLayout->addStretch(1);
    footerLayout->addWidget(mTransferButton);
    root->addWidget(footer);
    connect(updateButton, &QPushButton::clicked, this, &DATableQuickPlotWindow::onUpdateClicked);
    connect(mRefreshButton, &QPushButton::clicked, this, &DATableQuickPlotWindow::onRefreshClicked);
    connect(mTransferButton, &QToolButton::clicked, this, &DATableQuickPlotWindow::onTransferClicked);
    connect(newFigureAction, &QAction::triggered, this, &DATableQuickPlotWindow::onTransferNewFigure);
    connect(mAppendAction, &QAction::triggered, this, &DATableQuickPlotWindow::onTransferAppendCurrent);
    // 缩放手柄由承载框架 DATableChildWindow 提供（QSizeGrip 只作用于顶层窗口，
    // 子控件浮层用它缩不了自己——这也是本类原先「无法调整大小」的根因）
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
        const bool wantCategory = (mSpec.type == DATableChartType::Bar);
        int currentIndex        = 0;
        QStringList texts;
        texts.reserve(options.size());
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
            texts.append(text);
            if (wantCategory) {
                if (option.asCategory && option.column == mSpec.categoryColumn) {
                    currentIndex = i;
                }
            } else if (!option.asCategory && option.column == mSpec.xColumn) {
                currentIndex = i;
            }
        }
        // 本函数也会在下拉框自己的 currentIndexChanged 处理中被调用（onXAxisChanged）：
        // 条目没变时只挪 currentIndex、不动模型——在信号处理中清空下拉模型会把它的
        // 弹出层一起销毁，返回后即 use-after-free
        bool sameItems = (mXAxisCombo->count() == texts.size());
        for (int i = 0; sameItems && i < texts.size(); ++i) {
            sameItems = (mXAxisCombo->itemText(i) == texts.at(i) && mXAxisCombo->itemData(i).toInt() == i);
        }
        mIgnoreTypeSignals = true;
        if (!sameItems) {
            mXAxisCombo->clear();
            for (int i = 0; i < texts.size(); ++i) {
                mXAxisCombo->addItem(texts.at(i), i);
            }
        }
        mXAxisCombo->setCurrentIndex(currentIndex);
        mIgnoreTypeSignals = false;
    }
}

/**
 * @brief 按当前请求与规格重画
 *
 * 只做真正的取数与图元构建，不在里面重入事件派发：本函数可能由菜单/按钮的
 * action 触发链进入，此时同一条调用栈上还有 Qt 自己的控件事件处理，
 * 一旦重入派发（例如 QCoreApplication::processEvents）就可能把正在处理事件的
 * 控件（如刚触发动作的 QMenu）当场销毁，返回后即 use-after-free。需要「先出
 * 界面再取数」的地方走 scheduleRender()。
 */
void DATableQuickPlotWindow::renderNow()
{
    mRenderPending = false;
    if (!isVisible() || !mChart) {
        // 排队期间浮窗已被关闭/销毁：丢弃这次重绘
        return;
    }
    mErrorLabel->hide();
    mErrorLabel->clear();
    const QString pendingNotice = mPendingNotice;
    mPendingNotice.clear();

    QuickPlotBusyGuard busyGuard(this);

    DATableChartBuildInfo info;
    QString error;
    const bool ok = DAChartQuickPlot::render(mRequest, mSpec, mChart.data(), &info, &error, true, nullptr);
    if (!ok) {
        setNotice(QString());
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
    if (info.sampleLimited) {
        parts.append(tr("Sample limited: data fetching stopped at the budget limit"));  // cn:样本受限：取数触及预算上限，图表只使用受限样本
    }
    if (mDataChanged) {
        parts.append(tr("The data has changed, click Refresh to redraw"));  // cn:数据已变化，点击刷新
    }
    setNotice(parts.join(QStringLiteral("  \u00b7  ")));

    // 标题：列名摘要（承载框架标题栏显示）
    Q_EMIT titleChanged(columnSummary());
    mLastMessage.clear();
}

/**
 * @brief 排一次重绘：先把忙碌提示刷到界面上，真正的取数留到下一轮事件循环
 *
 * 这样既保留了「点开浮窗立刻看到正在取数」的观感，又保证重活永远不会在调用方
 * （菜单动作、图型按钮、下拉框）自己的事件处理栈上执行——在那里同步取数时若
 * 重入事件派发，正在处理点击的控件会被延迟删除事件当场销毁。
 * 多次连续请求（例如快速连点图型）合并成一次绘制。
 */
void DATableQuickPlotWindow::scheduleRender()
{
    if (mRenderPending) {
        return;
    }
    mRenderPending = true;
    setNotice(tr("Loading data..."));  // cn:正在取数…
    QTimer::singleShot(0, this, &DATableQuickPlotWindow::renderNow);
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
 * @brief 打开/刷新内容（位置与尺寸由承载框架负责）
 */
void DATableQuickPlotWindow::showForRequest(const DATableChartRequest& request)
{
    const DATableChartPlan plan = DATableChartPlanner::plan(request);
    if (!plan.isPlottable()) {
        // 未打开时（首次触发）不出现，只把原因交给消息区；
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
    // 标题先上（列名摘要即时可见），取数绘制完成后 renderNow 再刷一次
    Q_EMIT titleChanged(columnSummary());
    // 先让内容出来（带「正在取数…」提示），取数绘制交给下一轮事件循环
    show();
    raise();
    setFocus(Qt::OtherFocusReason);
    scheduleRender();
}

/**
 * @brief 绑定列的名称摘要（titleChanged 载荷）
 */
QString DATableQuickPlotWindow::columnSummary() const
{
    QStringList names;
    for (const DATableChartColumn& column : std::as_const(mRequest.columns)) {
        QString name = column.name;
        if (column.occurrence > 0) {
            name += QStringLiteral(" (%1)").arg(column.occurrence + 1);
        }
        names.append(name);
    }
    return names.join(QStringLiteral(" + "));
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
    scheduleRender();
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
 * @brief 设置「追加到当前图表」可用性提供者
 */
void DATableQuickPlotWindow::setAppendAvailabilityProvider(const AppendAvailabilityProvider& provider)
{
    mAppendAvailability = provider;
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
    scheduleRender();
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
    scheduleRender();
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
    scheduleRender();
}

void DATableQuickPlotWindow::onUpdateClicked()
{
    Q_EMIT currentSelectionRequested();
}

void DATableQuickPlotWindow::onRefreshClicked()
{
    mDataChanged = false;
    mRefreshButton->hide();
    scheduleRender();
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

void DATableQuickPlotWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        Q_EMIT closeRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

}  // namespace DA
