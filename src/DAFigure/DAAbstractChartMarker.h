#ifndef DAABSTRACTCHARTMARKER_H
#define DAABSTRACTCHARTMARKER_H
#include "DAFigureAPI.h"
#include "qwt_plot_marker.h"
namespace DA
{

/**
 * @brief marker接口
 */
class DAFIGURE_API DAAbstractChartMarker : public QwtPlotMarker
{
public:
    enum
    {
        PointMarker = 0  ///< 标记为点标记
        ,
        YValueMarker  ///< y值标记，水平线
        ,
        UserDefineMarker = 0x100  ///<用户自定义
    };
    DAAbstractChartMarker(const QString& title = QString());
    DAAbstractChartMarker(const QwtText& title);
    virtual ~DAAbstractChartMarker();
    virtual int markerType() const = 0;
};
}  // End Of Namespace DA
#endif  // DAABSTRACTCHARTMARKER_H
