#include "DAAppFigureFactory.h"
#include "DAAppFigureWidget.h"
namespace DA
{
DAAppFigureFactory::DAAppFigureFactory() : DAFigureFactory()
{
}

DAAppFigureFactory::~DAAppFigureFactory()
{
}

DAFigureWidget* DAAppFigureFactory::createFigure(QWidget* par)
{
    DAAppFigureWidget* fig = new DAAppFigureWidget(par);
    return fig;
}

}
