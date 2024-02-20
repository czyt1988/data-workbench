#ifndef DAAPPFIGUREWIDGET_H
#define DAAPPFIGUREWIDGET_H
#include "DAFigureWidget.h"

namespace DA
{
class DADataManager;

/**
 * @brief DAFigureWidget的特例化，加入了拖曳等功能
 */
class DAAppFigureWidget : public DAFigureWidget
{
    Q_OBJECT
public:
    DAAppFigureWidget(QWidget* parent = 0);
    ~DAAppFigureWidget();
};
}
#endif  // DAAPPFIGUREWIDGET_H
