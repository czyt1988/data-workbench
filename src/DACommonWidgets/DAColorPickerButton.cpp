#include "DAColorPickerButton.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAColorPickerButton
//===================================================
DAColorPickerButton::DAColorPickerButton(QWidget* parent) : ctkColorPickerButton(parent)
{
    setDisplayColorName(false);
}
