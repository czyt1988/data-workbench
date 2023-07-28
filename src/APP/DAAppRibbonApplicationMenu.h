#ifndef DAAPPRIBBONAPPLICATIONMENU_H
#define DAAPPRIBBONAPPLICATIONMENU_H
#include "SARibbonMenu.h"
namespace DA
{

/**
 * @brief ribbon application button 弹出的菜单
 */
class DAAppRibbonApplicationMenu : public SARibbonMenu
{
public:
    DAAppRibbonApplicationMenu(QWidget* parent = Q_NULLPTR);
};
}

#endif  // DAAPPRIBBONAPPLICATIONMENU_H
