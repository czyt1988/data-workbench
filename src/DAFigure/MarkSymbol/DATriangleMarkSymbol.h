#ifndef DATRIANGLEMARKSYMBOL_H
#define DATRIANGLEMARKSYMBOL_H
#include "DAFigureAPI.h"
#include "DAAbstractMarkSymbol.h"
namespace DA
{
/**
 * @brief 三角点数据标记
 */
class DAFIGURE_API DATriangleMarkSymbol : public DAAbstractMarkSymbol
{
public:
    DATriangleMarkSymbol(QColor clr = Qt::blue, int H = 8, int W = 8, bool isReversal = false);
    virtual ~DATriangleMarkSymbol();
    virtual int markType() const
    {
        return DAAbstractMarkSymbol::SymbolType_TriangleDataMarker;
    }
};
}  // End Of Namespace DA
#endif  // DATRIANGLEMARKSYMBOL_H
