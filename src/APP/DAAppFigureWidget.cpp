#include "DAAppFigureWidget.h"
#include <QDebug>
namespace DA
{

//==============================================================
// DAAppFigureWidget
//==============================================================

DAAppFigureWidget::DAAppFigureWidget(QWidget* parent) : DAFigureWidget(parent)
{
    setAcceptDrops(true);
}

DAAppFigureWidget::~DAAppFigureWidget()
{
}

}
