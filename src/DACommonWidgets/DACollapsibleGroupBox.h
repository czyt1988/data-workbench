#ifndef DACOLLAPSIBLEGROUPBOX_H
#define DACOLLAPSIBLEGROUPBOX_H
#include "ctkCollapsibleGroupBox.h"
#include "DACommonWidgetsAPI.h"
namespace DA
{
/**
 * @brief 基于ctkCollapsibleGroupBox的可缩放GroupBox
 */
class DACOMMONWIDGETS_API DACollapsibleGroupBox : public ctkCollapsibleGroupBox
{
    Q_OBJECT
public:
    DACollapsibleGroupBox(QWidget* parent = nullptr);
    DACollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);
};
}

#endif  // DACOLLAPSIBLEGROUPBOX_H
