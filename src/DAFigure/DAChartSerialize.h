#ifndef DACHARTSERIALIZE_H
#define DACHARTSERIALIZE_H
#include "DAFigureAPI.h"
#include <string>
#include <QDataStream>
#include "qwt_text.h"
#include "qwt_samples.h"

class QwtPlotItem;
class QwtSymbol;
class QwtPlotCurve;
class QwtPlotCanvas;
class QFrame;
class QwtScaleWidget;
class QwtPlot;
class QwtColorMap;
class QwtScaleDraw;
class QwtPlotBarChart;
class QwtColumnSymbol;
class QwtPlotIntervalCurve;
class QwtIntervalSymbol;
///
/// \brief 序列化类都是带异常的，使用中需要处理异常
///
namespace DA
{
class DAFIGURE_API DABadSerializeExpection : public std::exception
{
public:
    DABadSerializeExpection();
    DABadSerializeExpection(const char* why);
    DABadSerializeExpection(const QString& why);
    ~DABadSerializeExpection();
    const char* what() const noexcept;

private:
    QString mWhy;
};

// QFrame的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QFrame* f);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QFrame* f);

// QwtText的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtText& t);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtText& t);
// QwtSymbol的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtSymbol* t);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtSymbol* t);
// QwtIntervalSymbol的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtIntervalSymbol* t);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtIntervalSymbol* t);
// QwtColumnSymbol的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtColumnSymbol* t);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtColumnSymbol* t);
// QwtPlotItem指针的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtPlotItem* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlotItem* item);
// QwtScaleWidget指针的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtScaleWidget* w);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtScaleWidget* w);
// QwtScaleDraw指针的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtScaleDraw* w);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtScaleDraw* w);

// item 的序列化
// QwtPlotCurve指针的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtPlotCurve* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlotCurve* item);
// QwtPlotBarChart指针的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtPlotBarChart* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlotBarChart* item);
// QwtPlotIntervalCurve指针的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtPlotIntervalCurve* item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlotIntervalCurve* item);

// QwtPlotCanvas的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtPlotCanvas* c);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlotCanvas* c);
// QwtColorMap的序列化
//    DAFIGURE_API QDataStream& operator <<(QDataStream & out,const QwtColorMap* c);
//    DAFIGURE_API QDataStream& operator >>(QDataStream & in,QwtColorMap* c);
// QwtPlot的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtPlot* chart);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlot* chart);

}
//下面这两个要放到DA命名空间外，因为使用了QVector<T>的<<

// QwtIntervalSample的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtIntervalSample& item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtIntervalSample& item);
// QwtInterval的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtInterval& item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtInterval& item);

#endif  // SAQWTSERIALIZE_H
