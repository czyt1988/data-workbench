#ifndef DAPLOTUTILS_H
#define DAPLOTUTILS_H
#include "DAPlotAPI.h"
#include <QIcon>
namespace QIM
{
class QImPlotNode;
class QImPlotItemNode;
}

namespace DA
{
class DAPLOT_API DAPlotUtils
{
public:
    DAPlotUtils();
    ~DAPlotUtils();
    //
    static QIcon toIcon(QIM::QImPlotItemNode* n);
};
}
#endif  // DAPLOTUTILS_H
