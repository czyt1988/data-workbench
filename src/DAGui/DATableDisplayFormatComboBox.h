#ifndef DATABLEDISPLAYFORMATCOMBOBOX_H
#define DATABLEDISPLAYFORMATCOMBOBOX_H
#include "DAGuiAPI.h"
#include "DATableDisplayFormat.h"
#include <QComboBox>
namespace DA
{
class DAPyDType;

/**
 * @brief 表格列显示格式下拉框
 *
 * 始终装载全部类别项，按当前列 dtype 启/禁不适用项（General/Text 永远可用）。
 * 用户切换项时发射 currentFormatCategoryChanged，供控制器应用格式。
 * setCurrentFormat 用于反向同步（block signals）。
 */
class DAGUI_API DATableDisplayFormatComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit DATableDisplayFormatComboBox(QWidget* parent = nullptr);

    // 按列 dtype 启/禁类别项
    void updateDType(const DAPyDType& dt);

    // 反向同步当前格式（block signals），invalid→General
    void setCurrentFormat(const DATableDisplayFormat& fmt);

    // 当前选中类别
    DATableDisplayFormat::Category currentCategory() const;

Q_SIGNALS:
    void currentFormatCategoryChanged(DA::DATableDisplayFormat::Category c);

private:
    void populate();
    int indexOfCategory(DATableDisplayFormat::Category c) const;
};
}  // end of namespace DA
#endif  // DATABLEDISPLAYFORMATCOMBOBOX_H
