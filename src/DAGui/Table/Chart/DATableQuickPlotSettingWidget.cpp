#include "Table/Chart/DATableQuickPlotSettingWidget.h"
#include "Table/Chart/DATableQuickPlotWindow.h"
#include "Table/Chart/DATableChartRolePanel.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "DALogCategory.h"
// DAWidgets（通用折叠面板控件，与图表/工作流设置页同一套封装）
#include "DACollapseItem.h"
#include "DACollapseWidget.h"
// Qt
#include <QAction>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>
#include <functional>

namespace DA
{

namespace
{
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

// 角色行/图型行的属性名标签宽度（与 DATableChartRolePanel 内部行同一视觉宽度）
constexpr int kFieldLabelWidth = 68;

// 八向边框分割线颜色（DESIGN.md 普通边框/分隔线色）
constexpr char kHintColorStyle[] = "color: palette(dark);";
}  // namespace

// ============================================================
// DATableQuickPlotSettingWidget
// ============================================================

DATableQuickPlotSettingWidget::DATableQuickPlotSettingWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("DATableQuickPlotSettingWidget"));
    buildUi();
    refreshEmptyState();
}

DATableQuickPlotSettingWidget::~DATableQuickPlotSettingWidget()
{
    disconnectTarget();
}

/**
 * @brief 绑定/解绑目标窗口
 *
 * 同一窗口重复绑定会重新同步一次（宿主每次抬起本页都会调用，保证显示的是最新状态）。
 */
void DATableQuickPlotSettingWidget::setTarget(DATableQuickPlotWindow* window)
{
    if (mTarget == window) {
        if (mTarget) {
            syncFromTarget();
        } else {
            refreshEmptyState();
        }
        return;
    }
    disconnectTarget();
    mTarget = window;
    if (mTarget) {
        // 目标窗口是独立 QWidget：状态变化只回推本页，不反向操控窗口
        connect(mTarget.data(), &DATableQuickPlotWindow::contextChanged, this, &DATableQuickPlotSettingWidget::onTargetContextChanged);
        connect(mTarget.data(), &DATableQuickPlotWindow::statusChanged, this, &DATableQuickPlotSettingWidget::onTargetStatusChanged);
        connect(mTarget.data(), &QObject::destroyed, this, &DATableQuickPlotSettingWidget::onTargetDestroyed);
    }
    syncFromTarget();
}

/**
 * @brief 当前绑定的目标窗口
 */
DATableQuickPlotWindow* DATableQuickPlotSettingWidget::target() const
{
    return mTarget.data();
}

/**
 * @brief 目标窗口关闭/销毁：仅当与当前目标一致时解绑
 */
void DATableQuickPlotSettingWidget::releaseTarget(DATableQuickPlotWindow* window)
{
    if (window && mTarget.data() != window) {
        return;  // 别的表格页的快速绘图窗口，不清理当前绑定
    }
    setTarget(nullptr);
}

/**
 * @brief 从目标窗口重新同步全部界面
 */
void DATableQuickPlotSettingWidget::syncFromTarget()
{
    refreshEmptyState();
    if (!mTarget) {
        return;
    }
    refreshChrome();
    refreshTypeCombo();
    syncRolePanel();
}

/**
 * @brief 搭建界面
 *
 * 结构参考图表/工作流设置页：顶部是被设置对象摘要，中间是通用折叠分组
 * （DACollapseWidget + DACollapseItem），底部固定操作按钮与只读提示；
 * 无目标时整块换成空态提示。
 */
void DATableQuickPlotSettingWidget::buildUi()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 顶部标题：功能名 + 当前列摘要（与子窗口标题同一文案）
    QWidget* header        = new QWidget(this);
    QVBoxLayout* headerLay = new QVBoxLayout(header);
    headerLay->setContentsMargins(10, 8, 10, 6);
    headerLay->setSpacing(2);
    QLabel* titleLabel = new QLabel(tr("Quick plot"), header);  // cn:快速绘图
    QFont titleFont    = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    headerLay->addWidget(titleLabel);
    mSummaryLabel = new QLabel(header);
    mSummaryLabel->setWordWrap(true);
    mSummaryLabel->setStyleSheet(QString::fromLatin1(kHintColorStyle));
    headerLay->addWidget(mSummaryLabel);
    root->addWidget(header);

    // 空态（未打开快速绘图时）
    mEmptyLabel = new QLabel(this);
    mEmptyLabel->setWordWrap(true);
    mEmptyLabel->setAlignment(Qt::AlignCenter);
    mEmptyLabel->setContentsMargins(12, 20, 12, 20);
    mEmptyLabel->setStyleSheet(QString::fromLatin1(kHintColorStyle));
    mEmptyLabel->setText(tr("Open a quick plot from the data table to configure it here"));  // cn:请先在数据表格中打开快速绘图，再在此处设置
    root->addWidget(mEmptyLabel, 1);

    // 内容态：可滚动的设置分组 + 固定底栏
    mBody                = new QWidget(this);
    QVBoxLayout* bodyLay = new QVBoxLayout(mBody);
    bodyLay->setContentsMargins(0, 0, 0, 0);
    bodyLay->setSpacing(0);

    QScrollArea* scroll = new QScrollArea(mBody);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget* scrollContent     = new QWidget(scroll);
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(10, 0, 10, 6);
    contentLayout->setSpacing(6);

    DACollapseWidget* collapse = new DACollapseWidget(scrollContent);
    // 分组：图表（图型；角色行在下一个分组，随图型动态生成）
    DACollapseItem* chartGroup = collapse->addItem(tr("Chart"), QStringLiteral("chart"));  // cn:图表
    {
        QWidget* row        = new QWidget(chartGroup);
        QHBoxLayout* rowLay = new QHBoxLayout(row);
        rowLay->setContentsMargins(0, 0, 0, 0);
        rowLay->setSpacing(6);
        QLabel* label = new QLabel(tr("Type"), row);  // cn:图型
        label->setMinimumWidth(kFieldLabelWidth);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rowLay->addWidget(label);
        mTypeCombo = new QComboBox(row);
        mTypeCombo->setObjectName(QStringLiteral("DAQuickPlotTypeCombo"));
        mTypeCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        mTypeCombo->setToolTip(tr("Chart type (unavailable types are disabled with the reason)"));  // cn:图型（不满足当前选区的置灰并给出原因）
        buildTypeComboItems();
        rowLay->addWidget(mTypeCombo, 1);
        chartGroup->contentLayout()->addWidget(row);
    }
    chartGroup->setExpanded(true);
    // 分组：数据角色（按图型动态生成，复用快速绘图既有的轻量角色面板）
    DACollapseItem* roleGroup = collapse->addItem(tr("Data roles"), QStringLiteral("roles"));  // cn:数据角色
    mRolePanel                = new DATableChartRolePanel(roleGroup);
    roleGroup->contentLayout()->addWidget(mRolePanel);
    roleGroup->setExpanded(true);
    contentLayout->addWidget(collapse);
    contentLayout->addStretch(1);
    scroll->setWidget(scrollContent);
    bodyLay->addWidget(scroll, 1);

    // 底栏：只读状态 + 操作按钮（绘图窗口只保留绘图区，操作入口都放这里）
    QWidget* footer     = new QWidget(mBody);
    QVBoxLayout* footerLay = new QVBoxLayout(footer);
    footerLay->setContentsMargins(10, 4, 10, 8);
    footerLay->setSpacing(4);

    mNoticeLabel = new QLabel(footer);
    mNoticeLabel->setWordWrap(true);
    mNoticeLabel->setStyleSheet(QString::fromLatin1(kHintColorStyle));
    footerLay->addWidget(mNoticeLabel);

    mErrorLabel = new QLabel(footer);
    mErrorLabel->setWordWrap(true);
    mErrorLabel->setStyleSheet(QStringLiteral("color: palette(bright-text);"));
    footerLay->addWidget(mErrorLabel);

    QHBoxLayout* firstRow = new QHBoxLayout();
    firstRow->setContentsMargins(0, 0, 0, 0);
    firstRow->setSpacing(4);
    mUpdateButton = new QPushButton(tr("Update to current selection"), footer);  // cn:更新为当前选区
    mUpdateButton->setToolTip(tr("Re-bind the chart to the current column selection"));  // cn:按当前表格选区重新绑定绘图
    mRefreshButton = new QPushButton(tr("Refresh"), footer);  // cn:刷新
    mRefreshButton->setToolTip(tr("Redraw with the current data"));  // cn:按当前数据重绘
    firstRow->addWidget(mUpdateButton, 1);
    firstRow->addWidget(mRefreshButton);
    footerLay->addLayout(firstRow);

    mTransferButton = new QToolButton(footer);
    mTransferButton->setText(tr("Transfer to chart area"));  // cn:转入绘图区
    mTransferButton->setToolTip(tr("Create a formal chart from this quick plot"));  // cn:把当前快速绘图转成正式图表
    mTransferButton->setPopupMode(QToolButton::MenuButtonPopup);
    mTransferButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    mTransferButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QMenu* transferMenu      = new QMenu(mTransferButton);
    QAction* newFigureAction = transferMenu->addAction(tr("New figure tab"));  // cn:新建绘图标签页
    mAppendAction            = transferMenu->addAction(tr("Append to current chart"));  // cn:追加到当前图表
    mTransferButton->setMenu(transferMenu);
    connect(transferMenu, &QMenu::aboutToShow, this, [ this ]() {
        if (mAppendAction && mTarget) {
            mAppendAction->setEnabled(mTarget->isAppendToCurrentAvailable());
        }
    });
    connect(newFigureAction, &QAction::triggered, this, &DATableQuickPlotSettingWidget::onTransferNewFigure);
    connect(mAppendAction, &QAction::triggered, this, &DATableQuickPlotSettingWidget::onTransferAppendCurrent);
    footerLay->addWidget(mTransferButton);
    bodyLay->addWidget(footer);
    root->addWidget(mBody, 1);

    // 接线：本页编辑 -> 目标窗口
    connect(mTypeCombo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DATableQuickPlotSettingWidget::onTypeComboChanged);
    connect(mRolePanel, &DATableChartRolePanel::specEdited, this, &DATableQuickPlotSettingWidget::onRolePanelEdited);
    connect(mUpdateButton, &QPushButton::clicked, this, &DATableQuickPlotSettingWidget::onUpdateToSelectionClicked);
    connect(mRefreshButton, &QPushButton::clicked, this, &DATableQuickPlotSettingWidget::onRefreshClicked);
    connect(mTransferButton, &QToolButton::clicked, this, &DATableQuickPlotSettingWidget::onTransferNewFigure);
}

/**
 * @brief 重建图型下拉条目（12 种图型，聚合类前加分隔线）
 */
void DATableQuickPlotSettingWidget::buildTypeComboItems()
{
    if (!mTypeCombo || mTypeCombo->count() > 0) {
        return;
    }
    mIgnoreTypeSignals   = true;
    bool insertedSeparator = false;
    for (DATableChartType type : kTypeOrder) {
        if (!insertedSeparator && isAggregateType(type)) {
            mTypeCombo->insertSeparator(mTypeCombo->count());
            insertedSeparator = true;
        }
        mTypeCombo->addItem(DATableQuickPlotWindow::chartTypeText(type), static_cast< int >(type));
    }
    mIgnoreTypeSignals = false;
}

/**
 * @brief 按目标窗口的规划刷新图型下拉（不重建条目，避免在自身信号栈上销毁模型）
 */
void DATableQuickPlotSettingWidget::refreshTypeCombo()
{
    if (!mTypeCombo || !mTarget) {
        return;
    }
    const DATableChartPlan plan = mTarget->currentPlan();
    QStandardItemModel* model   = qobject_cast< QStandardItemModel* >(mTypeCombo->model());
    mIgnoreTypeSignals          = true;
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
            specForLabel = DATableChartPlanner::defaultSpec(mTarget->request(), type);
        }
        const QString label = DATableQuickPlotWindow::chartTypeText(type, specForLabel);
        if (mTypeCombo->itemText(i) != label) {
            mTypeCombo->setItemText(i, label);
        }
        mTypeCombo->setItemData(i, DATableQuickPlotWindow::chartTypeRequirementText(type), Qt::ToolTipRole);
        if (model) {
            if (QStandardItem* item = model->item(i)) {
                item->setEnabled(plan.types.contains(type));
            }
        }
    }
    const int currentIndex = mTypeCombo->findData(static_cast< int >(mTarget->spec().type));
    if (currentIndex >= 0) {
        mTypeCombo->setCurrentIndex(currentIndex);
    }
    mIgnoreTypeSignals = false;
}

/**
 * @brief 刷新标题摘要、提示/错误文字与按钮状态
 */
void DATableQuickPlotSettingWidget::refreshChrome()
{
    if (!mTarget) {
        return;
    }
    mSummaryLabel->setText(mTarget->displayTitle());
    const QString notice = mTarget->noticeText();
    mNoticeLabel->setText(notice);
    mNoticeLabel->setVisible(!notice.isEmpty());
    const QString error = mTarget->errorText();
    mErrorLabel->setText(error);
    mErrorLabel->setVisible(!error.isEmpty());
}

/**
 * @brief 空态与内容态切换
 */
void DATableQuickPlotSettingWidget::refreshEmptyState()
{
    const bool hasTarget = (mTarget && mTarget->isVisible());
    mBody->setVisible(hasTarget);
    mEmptyLabel->setVisible(!hasTarget);
    mSummaryLabel->setVisible(hasTarget);
}

/**
 * @brief 把目标窗口的请求/规格灌给角色设置面板
 */
void DATableQuickPlotSettingWidget::syncRolePanel()
{
    if (!mRolePanel || !mTarget) {
        return;
    }
    mRolePanel->setContext(mTarget->request(), mTarget->spec());
}

/**
 * @brief 断开与当前目标的全部连接
 */
void DATableQuickPlotSettingWidget::disconnectTarget()
{
    if (mTarget) {
        QObject::disconnect(mTarget.data(), nullptr, this, nullptr);
    }
}

void DATableQuickPlotSettingWidget::onTypeComboChanged(int index)
{
    if (mIgnoreTypeSignals || index < 0 || !mTarget) {
        return;
    }
    mTarget->setChartType(static_cast< DATableChartType >(mTypeCombo->itemData(index).toInt()));
}

void DATableQuickPlotSettingWidget::onRolePanelEdited()
{
    if (!mRolePanel || !mTarget) {
        return;
    }
    // 角色面板已经持有最新请求/规格：回写目标时先置标志，避免目标回推时重建本面板的行
    //（否则正在交互的下拉框会被销毁，焦点丢失且下拉收起）
    mApplyingOwnEdit = true;
    mTarget->applyEditedContext(mRolePanel->request(), mRolePanel->spec());
    mApplyingOwnEdit = false;
}

void DATableQuickPlotSettingWidget::onUpdateToSelectionClicked()
{
    if (mTarget) {
        mTarget->requestUpdateToCurrentSelection();
    }
}

void DATableQuickPlotSettingWidget::onRefreshClicked()
{
    if (mTarget) {
        mTarget->refresh();
    }
}

void DATableQuickPlotSettingWidget::onTransferNewFigure()
{
    if (mTarget) {
        mTarget->requestTransfer(false);
    }
}

void DATableQuickPlotSettingWidget::onTransferAppendCurrent()
{
    if (mTarget) {
        mTarget->requestTransfer(true);
    }
}

/**
 * @brief 目标窗口请求/规格变化：整体同步（角色行按新图型重建）
 */
void DATableQuickPlotSettingWidget::onTargetContextChanged()
{
    if (!mTarget) {
        return;
    }
    refreshEmptyState();
    refreshChrome();
    refreshTypeCombo();
    if (!mApplyingOwnEdit) {
        syncRolePanel();
    }
}

/**
 * @brief 目标窗口提示/错误/数据变化：只刷新状态文字与按钮
 */
void DATableQuickPlotSettingWidget::onTargetStatusChanged()
{
    refreshEmptyState();
    refreshChrome();
}

/**
 * @brief 目标窗口销毁：清空绑定
 */
void DATableQuickPlotSettingWidget::onTargetDestroyed()
{
    mTarget = nullptr;
    refreshEmptyState();
}

}  // namespace DA
