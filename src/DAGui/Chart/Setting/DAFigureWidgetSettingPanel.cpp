#include "Chart/Setting/DAFigureWidgetSettingPanel.h"
#include "Common/Settings/DAPropertyPanelContainerWidget.h"
#include "DAChart3DWidget.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "DALogCategory.h"
#include "qwt_figure.h"
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>
#include <utility>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAFigureWidgetSettingPanel::DAFigureWidgetSettingPanel(QWidget* parent) : QWidget(parent), mPanel(nullptr)
{
    // 创建DAPropertyPanelWidget并设为自身主布局
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    // 连接propertyValueChanged信号
    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged, this, &DAFigureWidgetSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAFigureWidgetSettingPanel::propertyValueChanged, this, &DAFigureWidgetSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAFigureWidgetSettingPanel::~DAFigureWidgetSettingPanel()
{
}

/**
 * @brief 获取通用属性面板指针
 * @return DAPropertyPanelWidget指针
 */
DAPropertyPanelContainerWidget* DAFigureWidgetSettingPanel::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 设置目标DAFigureWidget
 * @param fig 目标Figure
 */
void DAFigureWidgetSettingPanel::setTarget(DAFigureWidget* fig)
{
    if (mFigure == fig) {
        return;
    }
    // 目标切换：断开旧 figure 的尺寸约束复位通知，避免面板显示上一个 figure 的值
    if (DAFigureWidget* oldFigure = mFigure.data()) {
        disconnect(oldFigure,
                   &DAFigureWidget::figureSizeConstraintsChanged,
                   this,
                   &DAFigureWidgetSettingPanel::updateUI);
        disconnect(oldFigure,
                   &DAFigureWidget::gridLayoutEnabledChanged,
                   this,
                   &DAFigureWidgetSettingPanel::updateUI);
    }
    mFigure = fig;
    if (DAFigureWidget* newFigure = mFigure.data()) {
        // enableGridLayout 只在尺寸塌陷时复位 min/max；复位后由信号驱动刷新，否则面板仍显示旧值
        connect(newFigure,
                &DAFigureWidget::figureSizeConstraintsChanged,
                this,
                &DAFigureWidgetSettingPanel::updateUI);
        // 网格状态可能由存档恢复或其它入口改变，面板据此刷新按钮文案与可编辑性
        connect(newFigure,
                &DAFigureWidget::gridLayoutEnabledChanged,
                this,
                &DAFigureWidgetSettingPanel::updateUI);
    }
    updateUI();
}

/**
 * @brief 获取目标DAFigureWidget
 * @return DAFigureWidget指针
 */
DAFigureWidget* DAFigureWidgetSettingPanel::target() const
{
    return mFigure.data();
}

/**
 * @brief 更新界面显示
 *
 * 从DAFigureWidget和QwtFigure读取尺寸和背景状态写入面板。
 */
void DAFigureWidgetSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    QwtFigure* qwtfig = nullptr;
    if (mFigure) {
        qwtfig = mFigure->figure();
    }

    if (qwtfig) {
        mPanel->setIntValue(PID_MinWidth, qwtfig->minimumWidth());
        mPanel->setIntValue(PID_MinHeight, qwtfig->minimumHeight());
        mPanel->setIntValue(PID_MaxWidth, qwtfig->maximumWidth());
        mPanel->setIntValue(PID_MaxHeight, qwtfig->maximumHeight());
    } else {
        mPanel->setIntValue(PID_MinWidth, 0);
        mPanel->setIntValue(PID_MinHeight, 0);
        mPanel->setIntValue(PID_MaxWidth, 0);
        mPanel->setIntValue(PID_MaxHeight, 0);
    }

    if (mFigure) {
        mPanel->setBrushValue(PID_BackgroundBrush, mFigure->getFaceBrush());
    } else {
        mPanel->setBrushValue(PID_BackgroundBrush, QBrush());
    }

    // 网格布局：启用时显示实际行列数，未启用时保留用户填写的值（启用按钮读取它们）
    const bool gridEnabled = (mFigure && mFigure->isGridLayoutEnabled());
    if (mFigure) {
        const int gridRows  = mFigure->gridRowSizes().size();
        const int gridCols  = mFigure->gridColumnSizes().size();
        const int shownRows = gridEnabled ? gridRows : mPanel->getIntValue(PID_GridRows);
        const int shownCols = gridEnabled ? gridCols : mPanel->getIntValue(PID_GridColumns);
        mPanel->setIntValue(PID_GridRows, shownRows);
        mPanel->setIntValue(PID_GridColumns, shownCols);
    }
    if (mGridToggleButton) {
        mGridToggleButton->setText(gridEnabled ? tr("Exit Grid")  // cn:退出网格
                                               : tr("Enable Grid")  // cn:启用网格
        );
    }
    // 行列数决定网格结构：启用期间不可改，避免静默丢弃已有的单元格登记
    if (DAPropertyItemWidget* rowsItem = mPanel->getPropertyItem(PID_GridRows)) {
        rowsItem->setEnabled(!gridEnabled);
    }
    if (DAPropertyItemWidget* colsItem = mPanel->getPropertyItem(PID_GridColumns)) {
        colsItem->setEnabled(!gridEnabled);
    }
    if (mGridEqualButton) {
        mGridEqualButton->setEnabled(gridEnabled);
    }
}

/**
 * @brief 触发重绘
 */
void DAFigureWidgetSettingPanel::replot()
{
    if (mFigure) {
        mFigure->update();
    }
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - 尺寸组: 最小宽度、最小高度、最大宽度、最大高度
 * - 背景组: 背景画刷
 */
void DAFigureWidgetSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addCollapsibleGroup(tr("Size")  // cn:尺寸
    );
    panel->addIntProperty(PID_MinWidth, tr("Min Width")  // cn:最小宽度
                          ,
                          0, 0, 999999999);
    panel->addIntProperty(PID_MinHeight, tr("Min Height")  // cn:最小高度
                          ,
                          0, 0, 999999999);
    panel->addIntProperty(PID_MaxWidth, tr("Max Width")  // cn:最大宽度
                          ,
                          0, 0, 999999999);
    panel->addIntProperty(PID_MaxHeight, tr("Max Height")  // cn:最大高度
                          ,
                          0, 0, 999999999);
    panel->endGroup();

    panel->addCollapsibleGroup(tr("Background")  // cn:背景
    );
    panel->addBrushProperty(PID_BackgroundBrush, tr("Background Brush")  // cn:背景画刷
    );
    panel->endGroup();

    // 网格布局：启用后子图按行列轨道排布，可拖动分隔条改比例；退出后保持当前几何回到自由布局
    panel->addCollapsibleGroup(tr("Grid Layout")  // cn:网格布局
    );
    mGridToggleButton = new QPushButton(tr("Enable Grid")  // cn:启用网格
                                        ,
                                        mPanel);
    mGridToggleButton->setToolTip(
        tr("Enable the retained grid layout: sub charts follow the row/column tracks. "
           "Existing sub charts are placed into the cells in reading order.")  // cn:启用保留式网格布局：子图按行列轨道排布，已有子图按阅读顺序放进单元格
    );
    connect(mGridToggleButton, &QPushButton::clicked, this, &DAFigureWidgetSettingPanel::onGridToggleClicked);
    panel->addProperty(PID_GridToggle, tr("State")  // cn:状态
                       ,
                       mGridToggleButton);
    panel->addIntProperty(PID_GridRows, tr("Rows")  // cn:行数
                          ,
                          2,
                          1,
                          8);
    panel->addIntProperty(PID_GridColumns, tr("Columns")  // cn:列数
                          ,
                          2,
                          1,
                          8);
    mGridEqualButton = new QPushButton(tr("Equal Tracks")  // cn:等分轨道
                                       ,
                                       mPanel);
    mGridEqualButton->setToolTip(tr("Reset the row/column ratios to equal tracks")  // cn:把行/列比例恢复为等分
    );
    connect(mGridEqualButton, &QPushButton::clicked, this, &DAFigureWidgetSettingPanel::onGridEqualTracksClicked);
    panel->addProperty(PID_GridEqual, tr("Tracks")  // cn:轨道比例
                       ,
                       mGridEqualButton);
    panel->endGroup();
}

/**
 * @brief 转发属性值变化信号
 * @param propertyId 属性ID
 */
void DAFigureWidgetSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAFigureWidgetSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mFigure) {
        return;
    }

    // 行列数只决定下次启用网格时的结构：改动本身不改变图形，等用户点「启用网格」
    if (propertyId == PID_GridRows || propertyId == PID_GridColumns) {
        return;
    }

    QwtFigure* qwtfig = mFigure->figure();
    auto panel        = propertyPanel();

    switch (propertyId) {
    case PID_MinWidth: {
        if (qwtfig) {
            qwtfig->setMinimumWidth(panel->getIntValue(PID_MinWidth));
        }
        break;
    }
    case PID_MinHeight: {
        if (qwtfig) {
            qwtfig->setMinimumHeight(panel->getIntValue(PID_MinHeight));
        }
        break;
    }
    case PID_MaxWidth: {
        if (qwtfig) {
            qwtfig->setMaximumWidth(panel->getIntValue(PID_MaxWidth));
        }
        break;
    }
    case PID_MaxHeight: {
        if (qwtfig) {
            qwtfig->setMaximumHeight(panel->getIntValue(PID_MaxHeight));
        }
        break;
    }
    case PID_BackgroundBrush: {
        mFigure->setFaceBrush(panel->getBrushValue(PID_BackgroundBrush));
        break;
    }
    default:
        break;
    }

    replot();
}

/**
 * @brief 启用/退出网格布局
 *
 * 启用时按面板上的行列数等分轨道，并把 figure 里已有的子图按视觉顺序放进单元格；
 * 退出时成员保持当前几何回到自由布局（不可逆：比例与单元格登记都会丢弃）。
 */
void DAFigureWidgetSettingPanel::onGridToggleClicked()
{
    DAFigureWidget* fig = mFigure.data();
    if (!fig) {
        return;
    }
    if (fig->isGridLayoutEnabled()) {
        if (!fig->disableGridLayout()) {
            daWarning << tr("Failed to leave the grid layout");  // cn:退出网格布局失败
            return;
        }
        updateUI();
        return;
    }
    // 行列数来自面板（改动立即生效在界面上，但只在启用时用于建网格）
    const int rows = qBound(1, mPanel->getIntValue(PID_GridRows), 8);
    const int cols = qBound(1, mPanel->getIntValue(PID_GridColumns), 8);
    mPanel->setIntValue(PID_GridRows, rows);
    mPanel->setIntValue(PID_GridColumns, cols);
    if (!fig->enableGridLayout(rows, cols)) {
        daWarning << tr("Failed to enable the grid layout");  // cn:启用网格布局失败
        updateUI();
        return;
    }
    placeExistingChartsInGrid(fig, rows, cols);
    updateUI();
}

/**
 * @brief 把网格轨道恢复为等分（成员仍留在原单元格）
 */
void DAFigureWidgetSettingPanel::onGridEqualTracksClicked()
{
    DAFigureWidget* fig = mFigure.data();
    if (!fig || !fig->isGridLayoutEnabled()) {
        return;
    }
    const int rows = fig->gridRowSizes().size();
    const int cols = fig->gridColumnSizes().size();
    if (rows <= 0 || cols <= 0) {
        return;
    }
    QList< qreal > equalRows;
    QList< qreal > equalCols;
    equalRows.reserve(rows);
    equalCols.reserve(cols);
    for (int i = 0; i < rows; ++i) {
        equalRows.append(1.0);
    }
    for (int i = 0; i < cols; ++i) {
        equalCols.append(1.0);
    }
    // setGridSizes 内部不入撤销栈：等分是布局动作，不是对子图的编辑
    if (!fig->setGridSizes(equalRows, equalCols)) {
        daWarning << tr("Failed to equalize the grid tracks");  // cn:网格轨道等分失败
    }
}

/**
 * @brief 把已有子图按当前视觉顺序登记进网格
 * @param fig 目标 figure（网格已启用）
 * @param rows 网格行数
 * @param cols 网格列数
 *
 * 顺序为「先上后下、先左后右」，与用户在自由布局里看到的排列一致；
 * 超过 rows×cols 的子图保持自由布局并给出提示，不静默丢弃。
 */
void DAFigureWidgetSettingPanel::placeExistingChartsInGrid(DAFigureWidget* fig, int rows, int cols)
{
    QwtFigure* qwtfig = fig ? fig->figure() : nullptr;
    if (!qwtfig || rows <= 0 || cols <= 0) {
        return;
    }

    struct Member
    {
        QWidget* widget { nullptr };
        QRectF normRect;
    };
    QList< Member > members;
    const QList< DAChartWidget* > charts = fig->getCharts();
    for (DAChartWidget* chart : charts) {
        members.append(Member { chart, qwtfig->widgetNormRect(chart) });
    }
    const QList< DAChart3DWidget* > charts3d = fig->get3DCharts();
    for (DAChart3DWidget* chart3d : charts3d) {
        members.append(Member { chart3d, qwtfig->widgetNormRect(chart3d) });
    }
    std::sort(members.begin(), members.end(), [](const Member& lhs, const Member& rhs) {
        const qreal deltaTop = lhs.normRect.top() - rhs.normRect.top();
        if (qAbs(deltaTop) > 1e-6) {
            return deltaTop < 0.0;
        }
        return lhs.normRect.left() < rhs.normRect.left();
    });

    const int capacity = rows * cols;
    int placed   = 0;
    for (const Member& member : std::as_const(members)) {
        if (placed >= capacity) {
            break;
        }
        if (!member.widget) {
            continue;
        }
        if (!fig->setGridCell(member.widget, placed / cols, placed % cols)) {
            qWarning() << "DAFigureWidgetSettingPanel: failed to place a sub chart into the grid";
            continue;
        }
        ++placed;
    }
    const int overflow = members.size() - placed;
    if (overflow > 0) {
        daWarning << tr("%1 sub charts do not fit the %2 x %3 grid and stay in the free layout")
                         // cn:%1 个子图放不进 %2 x %3 网格，保持自由布局
                         .arg(overflow)
                         .arg(rows)
                         .arg(cols);
    }
}

}  // end namespace DA
