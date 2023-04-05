#ifndef DACOLORPICKERBUTTON_H
#define DACOLORPICKERBUTTON_H
#include "ctkColorPickerButton.h"
#include "DACommonWidgetsAPI.h"
namespace DA
{
/**
 * @brief 颜色拾取按钮封装
 */
class DACOMMONWIDGETS_API DAColorPickerButton : public ctkColorPickerButton
{
    Q_OBJECT
public:
    DAColorPickerButton(QWidget* parent = nullptr);
};
}  // namespace DA
#endif  // DACOLORPICKERBUTTON_H
