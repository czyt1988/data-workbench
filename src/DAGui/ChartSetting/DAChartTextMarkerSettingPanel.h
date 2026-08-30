#ifndef DACHARTTEXTMARKERSETTINGPANEL_H
#define DACHARTTEXTMARKERSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "DAChartTextMarker.h"

class QToolButton;

namespace DA
{

/**
 * @brief DAChartTextMarker属性设置面板
 *
 * 为文本标注提供属性编辑界面，包含标题、Z值、坐标轴、锚点坐标、
 * 富文本编辑入口、默认字体、默认文字颜色、标签对齐、边框圆角、
 * 背景画刷等属性。
 *
 * @note QwtText 通过值副本返回，修改后需调用 setLabel() 写回。
 *
 * @see DAChartItemSettingPanel
 * @see DAChartTextMarker
 */
class DAGUI_API DAChartTextMarkerSettingPanel : public DAChartItemSettingPanel
{
    Q_OBJECT
public:
    // 属性ID枚举
    enum PropertyId {
        PropTitle = 1,
        PropZValue,
        PropXAxis,
        PropYAxis,
        PropX,
        PropY,
        PropEditText,
        PropFont,
        PropTextColor,
        PropLabelAlignment,
        PropBorderRadius,
        PropBackgroundBrush,
        PropSpacing
    };

    explicit DAChartTextMarkerSettingPanel(QWidget* parent = nullptr);
    ~DAChartTextMarkerSettingPanel() override;

protected:
    // 构建属性面板
    void buildPropertyPanel() override;

    // 从QwtPlotItem更新界面
    void updateUI(QwtPlotItem* item) override;

protected Q_SLOTS:
    // 属性值变化处理
    void onPropertyValueChanged(int propertyId);
    // 富文本编辑按钮点击
    void onEditTextButtonClicked();

private:
    // 获取当前文本标注，类型不匹配返回nullptr
    DAChartTextMarker* currentTextMarker() const;

private:
    QToolButton* mEditTextButton { nullptr };  ///< 富文本编辑入口按钮
};

}  // end namespace DA

#endif  // DACHARTTEXTMARKERSETTINGPANEL_H
