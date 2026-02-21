#include "DAFigureFactory.h"
#include "DAFigureWidget.h"
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
    DAFigureWidget* fig      = new DAFigureWidget(par);
    DAChartFactory* chartFac = createChartFactory();
    if (chartFac) {
        fig->setupChartFactory(chartFac);
    }
    return fig;
}

DAChartFactory* DAFigureFactory::createChartFactory()
{
    return nullptr;
}

}
