#include "DAAbstractMarkSymbol.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
namespace DA
{
DAAbstractMarkSymbol::DAAbstractMarkSymbol()
{
}

DAAbstractMarkSymbol::DAAbstractMarkSymbol(const QPainterPath& path, const QBrush& brush, const QPen& pen)
    : QwtSymbol(path, brush, pen)
{
}

DAAbstractMarkSymbol::~DAAbstractMarkSymbol()
{
}
}  // End Of Namespace DA
