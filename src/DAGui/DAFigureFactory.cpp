#include "DAFigureFactory.h"
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
    return new DAFigureWidget(par);
}

}
