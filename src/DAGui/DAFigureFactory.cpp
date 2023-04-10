#include "DAFigureFactory.h"
#include "DAAppFigureWidget.h"
namespace DA
{

DAFigureFactory::DAFigureFactory()
{
}

DAFigureFactory::~DAFigureFactory()
{
}

DAFigureWidget* DAFigureFactory::createFigure(QWidget* par)
{
    return new DAAppFigureWidget(par);
}

}
