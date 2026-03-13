#include "DAChartItemCreatInteractor.h"
#include <QMouseEvent>
#include "DAChartWidget.h"
#include "qwt_plot_item.h"
#include "da_qt5qt6_compat.hpp"
#include "DAFigureWidget.h"
namespace DA
{

class DAChartItemCreatInteractor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartItemCreatInteractor)
public:
    PrivateData(DAChartItemCreatInteractor* p);
    FpCreatePlotItem m_createPlotItem { nullptr };
};

DAChartItemCreatInteractor::PrivateData::PrivateData(DAChartItemCreatInteractor* p)
{
}

//==============================================
// DAChartItemCreatInteractor
//==============================================
DAChartItemCreatInteractor::DAChartItemCreatInteractor(QwtPlot* parent)
    : DAAbstractChartEditor(parent), DA_PIMPL_CONSTRUCT
{
}
DAChartItemCreatInteractor::~DAChartItemCreatInteractor()
{
}
void DAChartItemCreatInteractor::setPlotItemInteractorFactory(FpCreatePlotItem fun)
{
    d_ptr->m_createPlotItem = fun;
}
DAChartItemCreatInteractor::FpCreatePlotItem DAChartItemCreatInteractor::getPlotItemInteractorFactory() const
{
    return d_ptr->m_createPlotItem;
}
bool DAChartItemCreatInteractor::mousePressEvent(const QMouseEvent* e)
{
    if (d_ptr->m_createPlotItem) {
        DAChartWidget* gca  = chart();
        DAFigureWidget* fig = gca->figureWidget();
        if (!gca || !fig) {
            return false;
        }
        Q_EMIT beginEdit();
        QwtPlotItem* item = d_ptr->m_createPlotItem(gca, compat::eventPos(e));
        if (item) {
            // 由于plotitem已经添加到plot中，因此addItem_的第三个参数设置为false
            fig->addItem_(gca, item, true);
        }
        Q_EMIT finishedEdit(false);
    }
    return false;
}

}  // namespace DA