#include "Table/Chart/DATableQuickPlotWindow.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "Table/Chart/DATableChartRolePanel.h"
#include "Table/Chart/DAChartQuickPlot.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "DALogCategory.h"
// Qt
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>
#include <utility>

namespace DA
{

namespace
{
// 贴入框架时的推荐尺寸（设置区展开后图表区保底约 260 高，实施方案 D16）
constexpr int kRecommendedWidth  = 640;
constexpr int kRecommendedHeight = 560;

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

// 图型下拉/右键子菜单的固定顺序（聚合类放最后，界面用分隔线分组）
const DATableChartType kTypeOrder[] = { DATableChartType::Line,
                                        DATableChartType::Scatter,
                                        DATableChartType::Histogram,
                                        DATableChartType::Boxplot,
                                        DATableChartType::MultiLine,
                                        DATableChartType::Contour,
                                        DATableChartType::ErrorBar,
                                        DATableChartType::VectorField,
                                        DATableChartType::Surface3D,
                                        DATableChartType::Line3D,
                                        DATableChartType::Bar,
                                        DATableChartType::MultiBar };

bool isAggregateType(DATableChartType type)
{
    return type == DATableChartType::Bar || type == DATableChartType::MultiBar;
}

}  // namespace

DATableQuickPlotWindow::DATableQuickPlotWindow(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("DATableQuickPlotWindow"));
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(420, 360);
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
    case DATableChartType::Contour:
        return tr("Contour");  // cn:等高线
    case DATableChartType::ErrorBar:
        return tr("Error bar");  // cn:误差棒
    case DATableChartType::VectorField:
        return tr("Vector field");  // cn:向量场
    case DATableChartType::MultiBar:
        return tr("Multi-bar");  // cn:多重柱状
    case DATableChartType::Surface3D:
        return tr("3D surface");  // cn:3D 曲面
    case DATableChartType::Line3D:
        return tr("3D line");  // cn:3D 线图
    case DATableChartType::None:
        break;
    }
    return QString();
}

/**
 * @brief 图型显示名（柱状图按规格区分计数/聚合文案）
 */
QString DATableQuickPlotWindow::chartTypeText(DATableChartType type, const DATableChartSpec& spec)
{
    if (type == DATableChartType::Bar) {
        return spec.valueColumns.isEmpty() ? tr("Bar (count)")  // cn:柱状（计数）
                                           : tr("Bar (aggregate)");  // cn:柱状（聚合）
    }
    return chartTypeText(type);
}

/**
 * @brief 图型对选区的要求（置灰时的悬浮提示）
 */
QString DATableQuickPlotWindow::chartTypeRequirementText(DATableChartType type)
{
    switch (type) {
    case DATableChartType::Line:
        return tr("Requires at least 1 numeric column");  // cn:需要至少 1 列数值
    case DATableChartType::Scatter:
        return tr("Requires 2 numeric columns");  // cn:需要 2 列数值
    case DATableChartType::Histogram:
        return tr("Requires 1 numeric column");  // cn:需要 1 列数值
    case DATableChartType::Boxplot:
        return tr("Requires at least 1 numeric column");  // cn:需要至少 1 列数值
    case DATableChartType::Bar:
        return tr("Requires 1 category column, or a low-cardinality numeric column");  // cn:需要 1 列分类，或低基数数值列
    case DATableChartType::MultiLine:
        return tr("Requires 3 numeric columns, or 1 time column and 2 numeric columns");  // cn:需要 3 列数值，或 1 列时间 + 2 列数值
    case DATableChartType::Contour:
    case DATableChartType::ErrorBar:
    case DATableChartType::Surface3D:
    case DATableChartType::Line3D:
        return tr("Requires 3 numeric columns");  // cn:需要 3 列数值
    case DATableChartType::VectorField:
        return tr("Requires 4 numeric columns");  // cn:需要 4 列数值
    case DATableChartType::MultiBar:
        return tr("Requires 1 category column and at least 2 numeric columns");  // cn:需要 1 列分类 + 至少 2 列数值
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

    // 工具栏：[图型 ▼] │ [设置] │ [更新为当前选区] [刷新] │ [转入绘图区 ▾]
    QWidget* toolbar        = new QWidget(this);
    QHBoxLayout* toolLayout = new QHBoxLayout(toolbar);
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->setSpacing(4);

    mTypeCombo = new QComboBox(toolbar);
    mTypeCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    mTypeCombo->setMinimumWidth(150);
    mTypeCombo->setToolTip(tr("Chart type (unavailable types are disabled with the reason)"));  // cn:图型（不满足当前选区的置灰并给出原因）
    bool insertedSeparator = false;
    for (DATableChartType type : kTypeOrder) {
        if (!insertedSeparator && isAggregateType(type)) {
            mTypeCombo->insertSeparator(mTypeCombo->count());
            insertedSeparator = true;
        }
        mTypeCombo->addItem(chartTypeText(type), static_cast< int >(type));
    }
    toolLayout->addWidget(mTypeCombo);

    QFrame* separator = new QFrame(toolbar);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    toolLayout->addWidget(separator);

    mSettingsButton = new QToolButton(toolbar);
    mSettingsButton->setText(tr("Settings"));  // cn:设置
    mSettingsButton->setCheckable(true);
    mSettingsButton->setChecked(true);
    mSettingsButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    mSettingsButton->setToolTip(tr("Show or hide the role settings"));  // cn:显示/隐藏角色设置
    toolLayout->addWidget(mSettingsButton);

    QPushButton* updateButton = new QPushButton(tr("Update to current selection"), toolbar);  // cn:更新为当前选区
    mRefreshButton            = new QPushButton(tr("Refresh"), toolbar);  // cn:刷新
    mRefreshButton->hide();
    toolLayout->addWidget(updateButton);
    toolLayout->addWidget(mRefreshButton);
    toolLayout->addStretch(1);

    mTransferButton = new QToolButton(toolbar);
    mTransferButton->setText(tr("Transfer to chart area"));  // cn:转入绘图区
    mTransferButton->setPopupMode(QToolButton::MenuButtonPopup);
    mTransferButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    QMenu* transferMenu      = new QMenu(mTransferButton);
    QAction* newFigureAction = transferMenu->addAction(tr("New figure tab"));  // cn:新建绘图标签页
    mAppendAction            = transferMenu->addAction(tr("Append to current chart"));  // cn:追加到当前图表
    mTransferButton->setMenu(transferMenu);
    connect(transferMenu, &QMenu::aboutToShow, this, [ this ]() {
        if (mAppendAction) {
            mAppendAction->setEnabled(mAppendAvailability ? mAppendAvailability() : true);
        }
    });
    toolLayout->addWidget(mTransferButton);
    root->addWidget(toolbar);

    // 设置区（默认展开，可折叠；图型感知角色行）
    QFrame* settingsFrame = new QFrame(this);
    settingsFrame->setFrameShape(QFrame::StyledPanel);
    settingsFrame->setFrameShadow(QFrame::Plain);
    mSettingsContainer          = settingsFrame;
    QVBoxLayout* settingsLayout = new QVBoxLayout(mSettingsContainer);
    settingsLayout->setContentsMargins(4, 2, 4, 2);
    settingsLayout->setSpacing(2);
    mRolePanel = new DATableChartRolePanel(mSettingsContainer);
    settingsLayout->addWidget(mRolePanel);
    root->addWidget(mSettingsContainer);

    // 图表区：2D 与 3D 堆叠切换（3D 懒创建）
    mStack   = new QStackedWidget(this);
    mChart2D = new DAChartWidget(mStack);
    mChart2D->setMinimumHeight(200);
    mStack->addWidget(mChart2D);
    root->addWidget(mStack, 1);

    // 状态文字行（只读，不放按钮）
    mNoticeLabel = new QLabel(this);
    mNoticeLabel->setWordWrap(true);
    mNoticeLabel->setStyleSheet(QStringLiteral("color: palette(dark);"));
    root->addWidget(mNoticeLabel);
    mErrorLabel = new QLabel(this);
    mErrorLabel->setWordWrap(true);
    mErrorLabel->setStyleSheet(QStringLiteral("color: palette(bright-text);"));
    mErrorLabel->hide();
    root->addWidget(mErrorLabel);

    connect(mTypeCombo,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DATableQuickPlotWindow::onChartTypeChanged);
    connect(mSettingsButton, &QToolButton::toggled, this, &DATableQuickPlotWindow::onSettingsToggled);
    connect(mRolePanel, &DATableChartRolePanel::specEdited, this, &DATableQuickPlotWindow::onRolePanelEdited);
    connect(updateButton, &QPushButton::clicked, this, &DATableQuickPlotWindow::onUpdateClicked);
    connect(mRefreshButton, &QPushButton::clicked, this, &DATableQuickPlotWindow::onRefreshClicked);
    connect(mTransferButton, &QToolButton::clicked, this, &DATableQuickPlotWindow::onTransferClicked);
    connect(newFigureAction, &QAction::triggered, this, &DATableQuickPlotWindow::onTransferNewFigure);
    connect(mAppendAction, &QAction::triggered, this, &DATableQuickPlotWindow::onTransferAppendCurrent);
}

/**
 * @brief 刷新图型下拉的可用状态与文案（不重建条目，避免在自身信号栈上销毁模型）
 */
void DATableQuickPlotWindow::refreshTypeCombo()
{
    if (!mTypeCombo) {
        return;
    }
    const DATableChartPlan plan = DATableChartPlanner::plan(mRequest);
    QStandardItemModel* model   = qobject_cast< QStandardItemModel* >(mTypeCombo->model());
    mIgnoreSignals              = true;
    for (int i = 0; i < mTypeCombo->count(); ++i) {
        const int typeValue = mTypeCombo->itemData(i).toInt();
        if (typeValue <= 0) {
            continue;  // 分隔线
        }
        const DATableChartType type = static_cast< DATableChartType >(typeValue);
        const int typeIndex         = plan.types.indexOf(type);
        DATableChartSpec specForLabel;
        if (typeIndex >= 0 && typeIndex < plan.typeSpecs.size()) {
            specForLabel = plan.typeSpecs.at(typeIndex);
        } else {
            specForLabel = DATableChartPlanner::defaultSpec(mRequest, type);
        }
        const QString label = chartTypeText(type, specForLabel);
        if (mTypeCombo->itemText(i) != label) {
            mTypeCombo->setItemText(i, label);
        }
        mTypeCombo->setItemData(i, chartTypeRequirementText(type), Qt::ToolTipRole);
        if (model) {
            if (QStandardItem* item = model->item(i)) {
                item->setEnabled(plan.types.contains(type));
            }
        }
    }
    const int currentIndex = mTypeCombo->findData(static_cast< int >(mSpec.type));
    if (currentIndex >= 0) {
        mTypeCombo->setCurrentIndex(currentIndex);
    }
    mIgnoreSignals = false;
}

/**
 * @brief 把当前请求/规格同步给设置区
 */
void DATableQuickPlotWindow::refreshSettingsPanel()
{
    if (mRolePanel) {
        mRolePanel->setContext(mRequest, mSpec);
    }
}

/**
 * @brief 3D 图表懒创建（首次切到 3D 才建 GL 上下文）
 */
DAChart3DWidget* DATableQuickPlotWindow::ensureChart3D()
{
    if (!mChart3D) {
        mChart3D = new DAChart3DWidget(mStack);
        mChart3D->setMinimumHeight(200);
        mStack->addWidget(mChart3D);
    }
    return mChart3D;
}

/**
 * @brief 按当前图型切换 2D/3D 图表页
 */
void DATableQuickPlotWindow::ensureChartForType()
{
    if (DAChartQuickPlot::is3DType(mSpec.type)) {
        mStack->setCurrentWidget(ensureChart3D());
    } else if (mChart2D) {
        mStack->setCurrentWidget(mChart2D);
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
    if (!isVisible()) {
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
    bool ok = false;
    if (DAChartQuickPlot::is3DType(mSpec.type)) {
        ok = DAChartQuickPlot::render3D(mRequest, mSpec, ensureChart3D(), &info, &error, false);
    } else if (mChart2D) {
        ok = DAChartQuickPlot::render(mRequest, mSpec, mChart2D.data(), &info, &error, true, nullptr);
    }
    if (!ok) {
        setNotice(QString());
        setError(error);
        mLastMessage = error;
        if (mChart2D) {
            mChart2D->replot();
        }
        return;
    }

    QStringList parts;
    if (!pendingNotice.isEmpty()) {
        parts.append(pendingNotice);
    }
    if (info.sampled) {
        parts.append(tr("Based on %1 of %2 sampled rows").arg(info.sampledRows).arg(info.totalRows));  // cn:基于 %1 / %2 行采样
    }
    if (info.approximate) {
        parts.append(tr("Aggregation uses the sample"));  // cn:聚合结果基于采样
    }
    if (info.droppedMissing > 0) {
        parts.append(tr("Ignored %1 rows with missing values").arg(info.droppedMissing));  // cn:已忽略 %1 行缺失值
    }
    if (info.displayedCategories > 0 && info.totalCategories > info.displayedCategories) {
        parts.append(tr("Showing top %1 of %2 categories").arg(info.displayedCategories).arg(info.totalCategories));  // cn:显示前 %1 类 / 共 %2 类
    }
    if (info.sampleLimited) {
        parts.append(tr("Sample limited: data fetching stopped at the budget limit"));  // cn:样本受限：取数触及预算上限，图表只使用受限样本
    }
    if (mDataChanged) {
        parts.append(tr("The data has changed, click Refresh to redraw"));  // cn:数据已变化，点击刷新
    }
    setNotice(parts.join(QStringLiteral("  \u00b7  ")));

    Q_EMIT titleChanged(windowTitle());
    mLastMessage.clear();
}

/**
 * @brief 排一次重绘：先把忙碌提示刷到界面上，真正的取数留到下一轮事件循环
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
 * @brief 设置错误文案（浮窗保持打开，不弹模态框；关闭按钮始终可用）
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
void DATableQuickPlotWindow::showForRequest(const DATableChartRequest& request, DATableChartType preferredType)
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
    mDataChanged = false;
    mManual      = false;
    mRefreshButton->hide();
    DATableChartType type = DATableChartType::None;
    if (preferredType != DATableChartType::None && plan.types.contains(preferredType)) {
        type = preferredType;
    } else {
        type = plan.types.first();
    }
    const int typeIndex = plan.types.indexOf(type);
    if (typeIndex >= 0 && typeIndex < plan.typeSpecs.size()) {
        mSpec = plan.typeSpecs.at(typeIndex);
    } else {
        mSpec = DATableChartPlanner::defaultSpec(request, type);
    }
    refreshTypeCombo();
    refreshSettingsPanel();
    Q_EMIT titleChanged(windowTitle());
    show();
    raise();
    setFocus(Qt::OtherFocusReason);
    // 先显示再切 3D：DAChart3DWidget 是 QOpenGLWidget，避免在隐藏态提前建 GL 上下文
    ensureChartForType();
    scheduleRender();
}

/**
 * @brief 浮窗标题：列名摘要 +（手动配置）后缀
 */
QString DATableQuickPlotWindow::windowTitle() const
{
    QStringList names;
    for (const DATableChartColumn& column : std::as_const(mRequest.columns)) {
        names.append(DATableChartRolePanel::columnDisplayName(column));
    }
    const QString summary = names.join(QStringLiteral(" + "));
    if (mManual) {
        return tr("%1 (manual)").arg(summary);  // cn:%1（手动配置）
    }
    return summary;
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

    mRequest     = request;
    mManual      = false;
    mDataChanged = false;
    mRefreshButton->hide();
    if (plan.types.contains(previousType)) {
        // 保持当前图型，尽量保留聚合/Top N/X 选择
        mSpec = DATableChartPlanner::defaultSpec(request, previousType);
        if ((mSpec.type == DATableChartType::Bar && !mSpec.valueColumns.isEmpty())
            || mSpec.type == DATableChartType::MultiBar) {
            mSpec.aggregation = previousAgg;
        }
        mSpec.categoryTopN = previousTopN;
        if (previousX >= 0 && previousX < request.columns.size()) {
            const DATableChartColumn& column = request.columns.at(previousX);
            if (column.isNumeric() || column.isDateTime()) {
                mSpec.xColumn = previousX;
            }
        }
        mPendingNotice.clear();
    } else {
        // 新选区不支持当前图型：回落到默认图型并给一句提示
        mSpec          = plan.defaultSpec;
        mPendingNotice = tr("The previous chart type is not available for the new selection, "
                            "switched to the default type");  // cn:新选区不支持原来的图型，已切换为默认图型
    }
    refreshTypeCombo();
    refreshSettingsPanel();
    ensureChartForType();
    Q_EMIT titleChanged(windowTitle());
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
    const DATableChartPlan plan = DATableChartPlanner::plan(mRequest);
    const int typeIndex         = plan.types.indexOf(type);
    if (typeIndex < 0) {
        return;  // 当前选区不支持该图型（下拉项应已置灰）
    }
    const DATableChartAggregation previousAgg = mSpec.aggregation;
    const int previousTopN = mSpec.categoryTopN;
    const int previousX    = mSpec.xColumn;

    DATableChartSpec next;
    if (typeIndex < plan.typeSpecs.size()) {
        next = plan.typeSpecs.at(typeIndex);
    } else {
        next = DATableChartPlanner::defaultSpec(mRequest, type);
    }
    if (!next.isValid()) {
        return;
    }
    if ((type == DATableChartType::Bar && !next.valueColumns.isEmpty())
        || type == DATableChartType::MultiBar) {
        next.aggregation = previousAgg;
    }
    next.categoryTopN = previousTopN;
    if (previousX >= 0 && previousX < mRequest.columns.size()) {
        const DATableChartColumn& column = mRequest.columns.at(previousX);
        if ((next.type == DATableChartType::Line || next.type == DATableChartType::Scatter
             || next.type == DATableChartType::MultiLine)
            && (column.isNumeric() || column.isDateTime())) {
            next.xColumn = previousX;
        }
    }
    mSpec = next;
    refreshTypeCombo();
    refreshSettingsPanel();
    ensureChartForType();
    Q_EMIT titleChanged(windowTitle());
    scheduleRender();
}

void DATableQuickPlotWindow::onChartTypeChanged(int index)
{
    if (mIgnoreSignals || index < 0) {
        return;
    }
    switchToType(static_cast< DATableChartType >(mTypeCombo->itemData(index).toInt()));
}

void DATableQuickPlotWindow::onSettingsToggled(bool on)
{
    mSettingsVisible = on;
    if (mSettingsContainer) {
        mSettingsContainer->setVisible(on);
    }
}

void DATableQuickPlotWindow::onRolePanelEdited()
{
    if (!mRolePanel) {
        return;
    }
    mRequest = mRolePanel->request();
    mSpec    = mRolePanel->spec();
    mManual  = true;
    refreshTypeCombo();
    Q_EMIT titleChanged(windowTitle());
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
