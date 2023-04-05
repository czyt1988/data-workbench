#ifndef DAABSTRACTMARKSYMBOL_H
#define DAABSTRACTMARKSYMBOL_H
#include "DAFigureAPI.h"
#include <qwt_symbol.h>
#include <QColor>
namespace DA
{
/**
 * @brief 绘图mark
 */
class DAFIGURE_API DAAbstractMarkSymbol : public QwtSymbol
{
public:
    DAAbstractMarkSymbol();
    DAAbstractMarkSymbol(const QPainterPath& path, const QBrush& brush, const QPen& pen);
    virtual ~DAAbstractMarkSymbol();
    enum
    {
        SymbolType_None = 0,           ///< 无标记
        SymbolType_TriangleDataMarker  ///< 三角数据标记
    };
    virtual int markType() const = 0;
};
}  // End Of Namespace DA
#endif  // DAABSTRACTMARKSYMBOL_H
