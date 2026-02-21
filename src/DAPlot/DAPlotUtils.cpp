#include "DAPlotUtils.h"
#include "plot/QImPlotLineItemNode.h"

namespace DA
{
DAPlotUtils::DAPlotUtils()
{
}

DAPlotUtils::~DAPlotUtils()
{
}

QIcon DAPlotUtils::toIcon(QIM::QImPlotItemNode* n)
{
    static QIcon s_default_chart_icon(":/DAFigure/icon/chart-item.svg");
    switch (n->type()) {
    //! Unspecific value, that can be used, when it doesn't matter
    case QIM::QImPlotLineItemNode::Type: {
        static QIcon s_icon(":/DAFigure/icon/chart-curve.svg");
        return s_icon;
    }
    default:
        break;
    }
    return s_default_chart_icon;
}
}  // end namespace DA
