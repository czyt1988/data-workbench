#include "DAChart3DColorLegendSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAChart3DWidget.h"
#include <QSignalBlocker>
#include <QVBoxLayout>
#include "qwt3d_colorlegend.h"
#include "qwt3d_plot.h"

namespace DA
{

/**
 * @brief 构造函数
 */
DAChart3DColorLegendSettingPanel::DAChart3DColorLegendSettingPanel(QWidget* parent) : QWidget(parent), mPanel(nullptr)
{
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged,
            this, &DAChart3DColorLegendSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChart3DColorLegendSettingPanel::propertyValueChanged,
            this, &DAChart3DColorLegendSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

DAChart3DColorLegendSettingPanel::~DAChart3DColorLegendSettingPanel()
{
}

DAPropertyPanelContainerWidget* DAChart3DColorLegendSettingPanel::propertyPanel() const
{
    return mPanel;
}

void DAChart3DColorLegendSettingPanel::setTarget(DAChart3DWidget* chart)
{
    if (mChart3D == chart) {
        return;
    }
    mChart3D = chart;
    updateUI();
}

DAChart3DWidget* DAChart3DColorLegendSettingPanel::target() const
{
    return mChart3D;
}

void DAChart3DColorLegendSettingPanel::replot()
{
    if (mChart3D) {
        mChart3D->update();
    }
}

/**
 * @brief 构建属性面板布局
 *
 * 分组：Display / Position
 */
void DAChart3DColorLegendSettingPanel::buildPropertyPanel()
{
    // ── Display ──
    mPanel->addCollapsibleGroup(tr("Display")  // cn:显示
    );
    mPanel->addBoolProperty(PID_Visible, tr("Visible")  // cn:显示
    );
    mPanel->endGroup();

    // ── Position ──
    mPanel->addCollapsibleGroup(tr("Position")  // cn:位置
    );
    mPanel->addEnumProperty(PID_Position, tr("Position")  // cn:位置
                            ,
                            QStringList() << tr("Top Left")  // cn:左上
                                          << tr("Top Center")  // cn:上中
                                          << tr("Top Right")  // cn:右上
                                          << tr("Left Center")  // cn:左中
                                          << tr("Center")  // cn:居中
                                          << tr("Right Center")  // cn:右中
                                          << tr("Bottom Left")  // cn:左下
                                          << tr("Bottom Center")  // cn:下中
                                          << tr("Bottom Right")  // cn:右下
                                          << tr("Custom")  // cn:自定义
                            ,
                            QList< int >() << static_cast< int >(Qwt3DColorLegend::PosTopLeft)
                                          << static_cast< int >(Qwt3DColorLegend::PosTopCenter)
                                          << static_cast< int >(Qwt3DColorLegend::PosTopRight)
                                          << static_cast< int >(Qwt3DColorLegend::PosLeftCenter)
                                          << static_cast< int >(Qwt3DColorLegend::PosCenter)
                                          << static_cast< int >(Qwt3DColorLegend::PosRightCenter)
                                          << static_cast< int >(Qwt3DColorLegend::PosBottomLeft)
                                          << static_cast< int >(Qwt3DColorLegend::PosBottomCenter)
                                          << static_cast< int >(Qwt3DColorLegend::PosBottomRight)
                                          << static_cast< int >(Qwt3DColorLegend::PosCustom)
    );
    mPanel->addDoubleProperty(PID_AbsPosX, tr("Absolute X")  // cn:绝对位置X
                               ,
                               0.0, 0.0, 9999.0, 0);
    mPanel->addDoubleProperty(PID_AbsPosY, tr("Absolute Y")  // cn:绝对位置Y
                               ,
                               0.0, 0.0, 9999.0, 0);
    mPanel->addDoubleProperty(PID_AbsWidth, tr("Absolute Width")  // cn:绝对宽度
                               ,
                               40.0, 1.0, 9999.0, 0);
    mPanel->addDoubleProperty(PID_AbsHeight, tr("Absolute Height")  // cn:绝对高度
                               ,
                               200.0, 1.0, 9999.0, 0);
    mPanel->endGroup();
}

/**
 * @brief 从 Qwt3DPlot / Qwt3DColorLegend 读取当前状态写入面板
 */
void DAChart3DColorLegendSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    if (!mChart3D) {
        return;
    }

    Qwt3DPlot* plot = static_cast< Qwt3DPlot* >(mChart3D.data());

    mPanel->setBoolValue(PID_Visible, plot->isColorLegendShown());

    Qwt3DColorLegend* legend = plot->legend();
    if (!legend) {
        return;
    }

    Qwt3DColorLegend::Position pos = legend->position();
    mPanel->setEnumValue(PID_Position, static_cast<int>(pos));

    if (pos == Qwt3DColorLegend::PosCustom) {
        QRectF abs = legend->absolutePosition();
        mPanel->setDoubleValue(PID_AbsPosX, abs.x());
        mPanel->setDoubleValue(PID_AbsPosY, abs.y());
        mPanel->setDoubleValue(PID_AbsWidth, abs.width());
        mPanel->setDoubleValue(PID_AbsHeight, abs.height());
        mPanel->setPropertyEnabled(PID_AbsPosX, true);
        mPanel->setPropertyEnabled(PID_AbsPosY, true);
        mPanel->setPropertyEnabled(PID_AbsWidth, true);
        mPanel->setPropertyEnabled(PID_AbsHeight, true);
    } else {
        mPanel->setPropertyEnabled(PID_AbsPosX, false);
        mPanel->setPropertyEnabled(PID_AbsPosY, false);
        mPanel->setPropertyEnabled(PID_AbsWidth, false);
        mPanel->setPropertyEnabled(PID_AbsHeight, false);
    }
}

/**
 * @brief 信号转发
 */
void DAChart3DColorLegendSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 */
void DAChart3DColorLegendSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mChart3D) {
        return;
    }

    Qwt3DPlot* plot = static_cast< Qwt3DPlot* >(mChart3D.data());

    switch (propertyId) {
    case PID_Visible:
        plot->showColorLegend(mPanel->getBoolValue(PID_Visible));
        break;
    case PID_Position: {
        int val = mPanel->getEnumValue(PID_Position);
        Qwt3DColorLegend::Position pos = static_cast< Qwt3DColorLegend::Position >(val);
        if (pos == Qwt3DColorLegend::PosCustom) {
            mPanel->setPropertyEnabled(PID_AbsPosX, true);
            mPanel->setPropertyEnabled(PID_AbsPosY, true);
            mPanel->setPropertyEnabled(PID_AbsWidth, true);
            mPanel->setPropertyEnabled(PID_AbsHeight, true);
            double x     = mPanel->getDoubleValue(PID_AbsPosX);
            double y     = mPanel->getDoubleValue(PID_AbsPosY);
            double w     = mPanel->getDoubleValue(PID_AbsWidth);
            double h     = mPanel->getDoubleValue(PID_AbsHeight);
            plot->setLegendAbsolutePosition(QRectF(x, y, w, h));
        } else {
            mPanel->setPropertyEnabled(PID_AbsPosX, false);
            mPanel->setPropertyEnabled(PID_AbsPosY, false);
            mPanel->setPropertyEnabled(PID_AbsWidth, false);
            mPanel->setPropertyEnabled(PID_AbsHeight, false);
            plot->setLegendPosition(pos);
        }
        break;
    }
    case PID_AbsPosX:
    case PID_AbsPosY:
    case PID_AbsWidth:
    case PID_AbsHeight: {
        Qwt3DColorLegend* legend = plot->legend();
        if (legend && legend->position() == Qwt3DColorLegend::PosCustom) {
            double x = mPanel->getDoubleValue(PID_AbsPosX);
            double y = mPanel->getDoubleValue(PID_AbsPosY);
            double w = mPanel->getDoubleValue(PID_AbsWidth);
            double h = mPanel->getDoubleValue(PID_AbsHeight);
            plot->setLegendAbsolutePosition(QRectF(x, y, w, h));
        }
        break;
    }
    default:
        break;
    }

    replot();
}

}  // namespace DA
