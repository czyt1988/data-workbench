#include "DAChart3DCoordSysSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAChart3DWidget.h"
#include <QSignalBlocker>
#include <QVBoxLayout>
#include "qwt3d_coordsys.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"

namespace DA
{

/**
 * @brief 构造函数
 */
DAChart3DCoordSysSettingPanel::DAChart3DCoordSysSettingPanel(QWidget* parent) : QWidget(parent), mPanel(nullptr)
{
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged,
            this, &DAChart3DCoordSysSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChart3DCoordSysSettingPanel::propertyValueChanged,
            this, &DAChart3DCoordSysSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

DAChart3DCoordSysSettingPanel::~DAChart3DCoordSysSettingPanel()
{
}

DAPropertyPanelContainerWidget* DAChart3DCoordSysSettingPanel::propertyPanel() const
{
    return mPanel;
}

void DAChart3DCoordSysSettingPanel::setTarget(DAChart3DWidget* chart)
{
    if (mChart3D == chart) {
        return;
    }
    mChart3D = chart;
    updateUI();
}

DAChart3DWidget* DAChart3DCoordSysSettingPanel::target() const
{
    return mChart3D;
}

void DAChart3DCoordSysSettingPanel::replot()
{
    if (mChart3D) {
        mChart3D->update();
    }
}

/**
 * @brief 构建属性面板布局
 *
 * 分组：Style / Colors / Fonts / Ticks / Interior Grid Width / Line
 */
void DAChart3DCoordSysSettingPanel::buildPropertyPanel()
{
    // ── Style ──
    mPanel->addCollapsibleGroup(tr("Style")  // cn:样式
    );
    mPanel->addEnumProperty(PID_Style, tr("Style")  // cn:样式
                            ,
                            QStringList() << tr("None")  // cn:无
                                          << tr("Box")  // cn:盒形
                                          << tr("Frame")  // cn:框架
                            ,
                            QList< int >() << static_cast< int >(NOCOORD)
                                          << static_cast< int >(BOX)
                                          << static_cast< int >(FRAME)
    );
    mPanel->endGroup();

    // ── Colors ──
    mPanel->addCollapsibleGroup(tr("Colors")  // cn:颜色
    );
    mPanel->addColorProperty(PID_AxesColor, tr("Axes Color")  // cn:轴颜色
    );
    mPanel->addColorProperty(PID_NumberColor, tr("Number Color")  // cn:数字颜色
    );
    mPanel->addColorProperty(PID_LabelColor, tr("Label Color")  // cn:标签颜色
    );
    mPanel->addColorProperty(PID_GridLinesColor, tr("Grid Lines Color")  // cn:网格线颜色
    );
    mPanel->addColorProperty(PID_InteriorGridLinesColor, tr("Interior Grid Color")  // cn:内部网格颜色
    );
    mPanel->endGroup();

    // ── Fonts ──
    mPanel->addCollapsibleGroup(tr("Fonts")  // cn:字体
    );
    mPanel->addFontProperty(PID_NumberFont, tr("Number Font")  // cn:数字字体
    );
    mPanel->addFontProperty(PID_LabelFont, tr("Label Font")  // cn:标签字体
    );
    mPanel->endGroup();

    // ── Ticks ──
    mPanel->addCollapsibleGroup(tr("Ticks")  // cn:刻度
    );
    mPanel->addDoubleProperty(PID_TicLength, tr("Tic Length")  // cn:刻度长度
                               ,
                               0.0, 0.0, 100.0, 3);
    mPanel->addDoubleProperty(PID_TicLengthScale, tr("Tic Length Scale")  // cn:刻度缩放
                               ,
                               0.015, 0.0, 1.0, 4);
    mPanel->addBoolProperty(PID_AutoScale, tr("Auto Scale")  // cn:自动缩放
    );
    mPanel->addBoolProperty(PID_AutoDecoration, tr("Auto Decoration")  // cn:自动装饰
    );
    mPanel->addEnumProperty(PID_TickPosition, tr("Tick Position")  // cn:刻度位置
                             ,
                             QStringList() << tr("Bottom")  // cn:底部
                                           << tr("Top")  // cn:顶部
                             ,
                             QList< int >() << static_cast< int >(TICK_BOTTOM)
                                           << static_cast< int >(TICK_TOP)
    );
    mPanel->endGroup();

    // ── Interior Grid Width ──
    mPanel->addCollapsibleGroup(tr("Interior Grid Width")  // cn:内部网格线宽
    );
    mPanel->addDoubleProperty(PID_InteriorGridMajorWidth, tr("Interior Major Width")  // cn:内部主网格线宽
                               ,
                               1.0, 0.1, 10.0, 2);
    mPanel->addDoubleProperty(PID_InteriorGridMinorWidth, tr("Interior Minor Width")  // cn:内部次网格线宽
                               ,
                               1.0, 0.1, 10.0, 2);
    mPanel->endGroup();

    // ── Line ──
    mPanel->addCollapsibleGroup(tr("Line")  // cn:线
    );
    mPanel->addBoolProperty(PID_LineSmooth, tr("Line Smooth")  // cn:线平滑
    );
    mPanel->endGroup();
}

/**
 * @brief 从 Qwt3DCoordinateSystem 读取当前状态写入面板
 */
void DAChart3DCoordSysSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    if (!mChart3D) {
        return;
    }
    Qwt3DCoordinateSystem* cs = static_cast< Qwt3DPlot* >(mChart3D.data())->coordinates();
    if (!cs) {
        return;
    }

    mPanel->setEnumValue(PID_Style, static_cast<int>(cs->style()));

    RGBA ac = cs->axesColor();
    mPanel->setColorValue(PID_AxesColor, GL2Qt(ac.r, ac.g, ac.b));
    RGBA nc = cs->numberColor();
    mPanel->setColorValue(PID_NumberColor, GL2Qt(nc.r, nc.g, nc.b));
    RGBA lc = cs->labelColor();
    mPanel->setColorValue(PID_LabelColor, GL2Qt(lc.r, lc.g, lc.b));
    RGBA gc = cs->gridLinesColor();
    mPanel->setColorValue(PID_GridLinesColor, GL2Qt(gc.r, gc.g, gc.b));
    RGBA igc = cs->interiorGridLinesColor();
    mPanel->setColorValue(PID_InteriorGridLinesColor, GL2Qt(igc.r, igc.g, igc.b));

    QFont numFnt = cs->numberFont();
    mPanel->setFontValue(PID_NumberFont, numFnt);
    QFont lblFnt = cs->labelFont();
    mPanel->setFontValue(PID_LabelFont, lblFnt);

    double majorLen, minorLen;
    cs->ticLength(majorLen, minorLen);
    mPanel->setDoubleValue(PID_TicLength, majorLen);
    mPanel->setDoubleValue(PID_TicLengthScale, cs->ticLengthScale());
    mPanel->setBoolValue(PID_AutoScale, cs->autoScale());
    mPanel->setBoolValue(PID_AutoDecoration, cs->autoDecoration());
    mPanel->setEnumValue(PID_TickPosition, static_cast<int>(cs->tickPosition()));

    // Interior grid widths from theme
    Qwt3DTheme t = static_cast< Qwt3DPlot* >(mChart3D.data())->theme();
    mPanel->setDoubleValue(PID_InteriorGridMajorWidth, t.interiorGridMajorWidth());
    mPanel->setDoubleValue(PID_InteriorGridMinorWidth, t.interiorGridMinorWidth());

    mPanel->setBoolValue(PID_LineSmooth, cs->lineSmooth());
}

/**
 * @brief 信号转发
 */
void DAChart3DCoordSysSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 */
void DAChart3DCoordSysSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mChart3D) {
        return;
    }
    Qwt3DCoordinateSystem* cs = static_cast< Qwt3DPlot* >(mChart3D.data())->coordinates();
    if (!cs) {
        return;
    }

    switch (propertyId) {
    case PID_Style: {
        int val = mPanel->getEnumValue(PID_Style);
        cs->setStyle(static_cast< COORDSTYLE >(val));
        break;
    }
    case PID_AxesColor:
        cs->setAxesColor(Qt2GL(mPanel->getColorValue(PID_AxesColor)));
        break;
    case PID_NumberColor:
        cs->setNumberColor(Qt2GL(mPanel->getColorValue(PID_NumberColor)));
        break;
    case PID_LabelColor:
        cs->setLabelColor(Qt2GL(mPanel->getColorValue(PID_LabelColor)));
        break;
    case PID_NumberFont:
        cs->setNumberFont(mPanel->getFontValue(PID_NumberFont));
        break;
    case PID_LabelFont:
        cs->setLabelFont(mPanel->getFontValue(PID_LabelFont));
        break;
    case PID_GridLinesColor:
        cs->setGridLinesColor(Qt2GL(mPanel->getColorValue(PID_GridLinesColor)));
        break;
    case PID_InteriorGridLinesColor:
        cs->setInteriorGridLinesColor(Qt2GL(mPanel->getColorValue(PID_InteriorGridLinesColor)));
        break;
    case PID_InteriorGridMajorWidth: {
        double minor = mPanel->getDoubleValue(PID_InteriorGridMinorWidth);
        cs->setInteriorGridLinesWidth(mPanel->getDoubleValue(PID_InteriorGridMajorWidth), minor);
        break;
    }
    case PID_InteriorGridMinorWidth: {
        double major = mPanel->getDoubleValue(PID_InteriorGridMajorWidth);
        cs->setInteriorGridLinesWidth(major, mPanel->getDoubleValue(PID_InteriorGridMinorWidth));
        break;
    }
    case PID_TicLength: {
        double newMajor = mPanel->getDoubleValue(PID_TicLength);
        double curMajor, curMinor;
        cs->ticLength(curMajor, curMinor);
        cs->setTicLength(newMajor, curMinor);
        break;
    }
    case PID_TicLengthScale:
        cs->setTicLengthScale(mPanel->getDoubleValue(PID_TicLengthScale));
        break;
    case PID_AutoScale:
        cs->setAutoScale(mPanel->getBoolValue(PID_AutoScale));
        break;
    case PID_AutoDecoration:
        cs->setAutoDecoration(mPanel->getBoolValue(PID_AutoDecoration));
        break;
    case PID_TickPosition: {
        int val = mPanel->getEnumValue(PID_TickPosition);
        cs->setTickPosition(static_cast< TICKPOSITION >(val));
        break;
    }
    case PID_LineSmooth:
        cs->setLineSmooth(mPanel->getBoolValue(PID_LineSmooth));
        break;
    default:
        break;
    }

    replot();
}

}  // namespace DA
