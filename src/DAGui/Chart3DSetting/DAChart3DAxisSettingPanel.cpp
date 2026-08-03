#include "DAChart3DAxisSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAChart3DWidget.h"
#include <QSignalBlocker>
#include <QVBoxLayout>
#include "qwt3d_axis.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_types.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param dir 轴方向（X/Y/Z）
 * @param parent 父控件
 */
DAChart3DAxisSettingPanel::DAChart3DAxisSettingPanel(AxisDirection dir, QWidget* parent)
    : QWidget(parent), mPanel(nullptr), mDirection(dir)
{
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged,
            this, &DAChart3DAxisSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChart3DAxisSettingPanel::propertyValueChanged,
            this, &DAChart3DAxisSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

DAChart3DAxisSettingPanel::~DAChart3DAxisSettingPanel()
{
}

DAPropertyPanelContainerWidget* DAChart3DAxisSettingPanel::propertyPanel() const
{
    return mPanel;
}

void DAChart3DAxisSettingPanel::setTarget(DAChart3DWidget* chart)
{
    if (mChart3D == chart) {
        return;
    }
    mChart3D = chart;
    updateUI();
}

DAChart3DWidget* DAChart3DAxisSettingPanel::target() const
{
    return mChart3D;
}

DAChart3DAxisSettingPanel::AxisDirection DAChart3DAxisSettingPanel::axisDirection() const
{
    return mDirection;
}

void DAChart3DAxisSettingPanel::replot()
{
    if (mChart3D) {
        mChart3D->update();
    }
}

/**
 * @brief 返回该方向的 4 个 AXIS 枚举值
 */
QList< int > DAChart3DAxisSettingPanel::axisEnumList() const
{
    switch (mDirection) {
    case DirX:
        return QList< int >() << X1 << X2 << X3 << X4;
    case DirY:
        return QList< int >() << Y1 << Y2 << Y3 << Y4;
    case DirZ:
        return QList< int >() << Z1 << Z2 << Z3 << Z4;
    default:
        return QList< int >();
    }
}

/**
 * @brief 获取当前选中的 AXIS 枚举值
 */
int DAChart3DAxisSettingPanel::currentAxisEnum() const
{
    int idx = mPanel->getEnumValue(PID_AxisSelector);
    QList< int > axes = axisEnumList();
    if (idx >= 0 && idx < axes.size()) {
        return axes.at(idx);
    }
    return -1;
}

/**
 * @brief 获取当前选中的 Qwt3DAxis 引用
 */
Qwt3DAxis* DAChart3DAxisSettingPanel::currentAxis()
{
    if (!mChart3D) {
        return nullptr;
    }
    Qwt3DCoordinateSystem* cs = static_cast< Qwt3DPlot* >(mChart3D.data())->coordinates();
    if (!cs) {
        return nullptr;
    }
    int axisEnum = currentAxisEnum();
    if (axisEnum < 0 || axisEnum >= static_cast< int >(cs->axes.size())) {
        return nullptr;
    }
    return &cs->axes[ axisEnum ];
}

/**
 * @brief 构建属性面板布局
 *
 * 分组：Axis Selector / Label / Numbers / Range / Appearance / Grid
 */
void DAChart3DAxisSettingPanel::buildPropertyPanel()
{
    // ── Axis Selector ──
    mPanel->addCollapsibleGroup(tr("Axis Selector")  // cn:轴选择
    );
    QList< int > axes = axisEnumList();
    QString prefix;
    switch (mDirection) {
    case DirX: prefix = "X"; break;
    case DirY: prefix = "Y"; break;
    case DirZ: prefix = "Z"; break;
    }
    QStringList axisNames;
    QList< int > dataValues;
    for (int i = 0; i < axes.size(); ++i) {
        axisNames << QString("%1%2").arg(prefix).arg(i + 1);
        dataValues << i;
    }
    mPanel->addEnumProperty(PID_AxisSelector, tr("Axis")  // cn:轴
                            ,
                            axisNames, dataValues);
    mPanel->endGroup();

    // ── Label ──
    mPanel->addCollapsibleGroup(tr("Label")  // cn:标签
    );
    mPanel->addStringProperty(PID_LabelText, tr("Label Text")  // cn:标签文本
    );
    mPanel->addFontProperty(PID_LabelFont, tr("Label Font")  // cn:标签字体
    );
    mPanel->addColorProperty(PID_LabelColor, tr("Label Color")  // cn:标签颜色
    );
    mPanel->endGroup();

    // ── Numbers ──
    mPanel->addCollapsibleGroup(tr("Numbers")  // cn:数字
    );
    mPanel->addFontProperty(PID_NumberFont, tr("Number Font")  // cn:数字字体
    );
    mPanel->addColorProperty(PID_NumberColor, tr("Number Color")  // cn:数字颜色
    );
    mPanel->endGroup();

    // ── Range ──
    mPanel->addCollapsibleGroup(tr("Range")  // cn:范围
    );
    mPanel->addDoubleProperty(PID_MinRange, tr("Min Range")  // cn:最小范围
                               ,
                               0.0, -1e15, 1e15, 5);
    mPanel->addDoubleProperty(PID_MaxRange, tr("Max Range")  // cn:最大范围
                               ,
                               0.0, -1e15, 1e15, 5);
    mPanel->addIntProperty(PID_MajorCount, tr("Major Count")  // cn:主刻度数
                            ,
                            0, 0, 100);
    mPanel->addIntProperty(PID_MinorCount, tr("Minor Count")  // cn:次刻度数
                            ,
                            0, 0, 100);
    mPanel->endGroup();

    // ── Appearance ──
    mPanel->addCollapsibleGroup(tr("Appearance")  // cn:外观
    );
    mPanel->addDoubleProperty(PID_LineWidth, tr("Line Width")  // cn:线宽
                               ,
                               1.0, 0.1, 10.0, 2);
    mPanel->addEnumProperty(PID_TickPosition, tr("Tick Position")  // cn:刻度位置
                             ,
                             QStringList() << tr("Bottom")  // cn:底部
                                           << tr("Top")  // cn:顶部
                             ,
                             QList< int >() << static_cast< int >(TICK_BOTTOM)
                                           << static_cast< int >(TICK_TOP)
    );
    mPanel->addBoolProperty(PID_SmoothLine, tr("Smooth Line")  // cn:平滑线
    );
    mPanel->endGroup();

    // ── Grid ──
    mPanel->addCollapsibleGroup(tr("Grid")  // cn:网格
    );
    mPanel->addBoolProperty(PID_GridMajor, tr("Major Grid")  // cn:主网格线
    );
    mPanel->addBoolProperty(PID_GridMinor, tr("Minor Grid")  // cn:次网格线
    );
    mPanel->addColorProperty(PID_GridColor, tr("Grid Color")  // cn:网格颜色
    );
    mPanel->endGroup();
}

/**
 * @brief 从 Qwt3DAxis / Qwt3DCoordinateSystem 读取当前状态写入面板
 */
void DAChart3DAxisSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    if (!mChart3D) {
        return;
    }
    Qwt3DCoordinateSystem* cs = static_cast< Qwt3DPlot* >(mChart3D.data())->coordinates();
    if (!cs) {
        return;
    }
    Qwt3DAxis* ax = currentAxis();
    if (!ax) {
        return;
    }

    // Label
    mPanel->setStringValue(PID_LabelText, ax->labelString());
    QFont labelFnt = ax->labelFont();
    mPanel->setFontValue(PID_LabelFont, labelFnt);
    RGBA lc = ax->labelColor();
    mPanel->setColorValue(PID_LabelColor, GL2Qt(lc.r, lc.g, lc.b));

    // Numbers
    QFont numFnt = ax->numberFont();
    mPanel->setFontValue(PID_NumberFont, numFnt);
    RGBA nc = ax->numberColor();
    mPanel->setColorValue(PID_NumberColor, GL2Qt(nc.r, nc.g, nc.b));

    // Range
    double start, stop;
    ax->limits(start, stop);
    mPanel->setDoubleValue(PID_MinRange, start);
    mPanel->setDoubleValue(PID_MaxRange, stop);
    mPanel->setIntValue(PID_MajorCount, ax->majors());
    mPanel->setIntValue(PID_MinorCount, ax->minors());

    // Appearance
    mPanel->setDoubleValue(PID_LineWidth, ax->lineWidth());
    mPanel->setEnumValue(PID_TickPosition, static_cast<int>(cs->tickPosition()));
    mPanel->setBoolValue(PID_SmoothLine, cs->lineSmooth());

    // Grid — parse grids() return value for major/minor
    int sides = cs->grids();
    mPanel->setBoolValue(PID_GridMajor, sides != 0);
    RGBA gc = cs->gridLinesColor();
    mPanel->setColorValue(PID_GridColor, GL2Qt(gc.r, gc.g, gc.b));
}

/**
 * @brief 信号转发
 */
void DAChart3DAxisSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 */
void DAChart3DAxisSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mChart3D) {
        return;
    }
    Qwt3DCoordinateSystem* cs = static_cast< Qwt3DPlot* >(mChart3D.data())->coordinates();
    if (!cs) {
        return;
    }
    Qwt3DAxis* ax = currentAxis();
    if (!ax) {
        return;
    }

    switch (propertyId) {
    case PID_AxisSelector:
        // 轴选择变更，触发 updateUI 刷新显示新轴的属性
        updateUI();
        break;
    case PID_LabelText:
        ax->setLabelString(mPanel->getStringValue(PID_LabelText));
        break;
    case PID_LabelFont:
        ax->setLabelFont(mPanel->getFontValue(PID_LabelFont));
        break;
    case PID_LabelColor:
        ax->setLabelColor(Qt2GL(mPanel->getColorValue(PID_LabelColor)));
        break;
    case PID_NumberFont:
        ax->setNumberFont(mPanel->getFontValue(PID_NumberFont));
        break;
    case PID_NumberColor:
        ax->setNumberColor(Qt2GL(mPanel->getColorValue(PID_NumberColor)));
        break;
    case PID_MinRange: {
        double dummy, stop;
        ax->limits(dummy, stop);
        ax->setLimits(mPanel->getDoubleValue(PID_MinRange), stop);
        break;
    }
    case PID_MaxRange: {
        double start, dummy;
        ax->limits(start, dummy);
        ax->setLimits(start, mPanel->getDoubleValue(PID_MaxRange));
        break;
    }
    case PID_MajorCount:
        ax->setMajors(mPanel->getIntValue(PID_MajorCount));
        break;
    case PID_MinorCount:
        ax->setMinors(mPanel->getIntValue(PID_MinorCount));
        break;
    case PID_LineWidth:
        ax->setLineWidth(mPanel->getDoubleValue(PID_LineWidth));
        break;
    case PID_TickPosition: {
        int val = mPanel->getEnumValue(PID_TickPosition);
        cs->setTickPosition(static_cast< TICKPOSITION >(val));
        break;
    }
    case PID_SmoothLine:
        cs->setLineSmooth(mPanel->getBoolValue(PID_SmoothLine));
        break;
    case PID_GridMajor: {
        bool major = mPanel->getBoolValue(PID_GridMajor);
        bool minor = mPanel->getBoolValue(PID_GridMinor);
        int sides = cs->grids();
        cs->setGridLines(major, minor, sides);
        break;
    }
    case PID_GridMinor: {
        bool major = mPanel->getBoolValue(PID_GridMajor);
        bool minor = mPanel->getBoolValue(PID_GridMinor);
        int sides = cs->grids();
        cs->setGridLines(major, minor, sides);
        break;
    }
    case PID_GridColor:
        cs->setGridLinesColor(Qt2GL(mPanel->getColorValue(PID_GridColor)));
        break;
    default:
        break;
    }

    replot();
}

}  // namespace DA
