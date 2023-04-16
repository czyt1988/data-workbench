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
    fig->setDataManager(mDataManager);
    return fig;
}

void DAAppFigureFactory::setDataManager(DADataManager* mgr)
{
    mDataManager = mgr;
}
}
