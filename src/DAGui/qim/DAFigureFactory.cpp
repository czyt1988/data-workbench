#include "DAFigureFactory.h"
namespace DA
{

DAFigureFactory::DAFigureFactory()
{
}

DAFigureFactory::~DAFigureFactory()
{
}

DAFigureScrollArea* DAFigureFactory::createFigure(QWidget* par)
{
    DAFigureScrollArea* fig = new DAFigureScrollArea(par);
    return fig;
}


}
