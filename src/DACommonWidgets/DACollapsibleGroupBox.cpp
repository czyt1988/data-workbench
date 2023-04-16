#include "DACollapsibleGroupBox.h"
namespace DA
{

DACollapsibleGroupBox::DACollapsibleGroupBox(QWidget* parent) : ctkCollapsibleGroupBox(parent)
{
}

DACollapsibleGroupBox::DACollapsibleGroupBox(const QString& title, QWidget* parent)
    : ctkCollapsibleGroupBox(title, parent)
{
}

}
