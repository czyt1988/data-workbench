#ifndef DACHARTSERIALIZE_H
#define DACHARTSERIALIZE_H
#include "DAFigureAPI.h"
#include <string>
#include <QDataStream>
#include <functional>
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

/**
 * @brief 针对QwtPlotItem的二进制序列化
 */
class DAFIGURE_API DAChartItemSerialize
{
public:
    class Header
    {
    public:
        Header();
        ~Header();
        std::uint32_t magic;
        int version;  ///< 版本
        int rtti;
        unsigned char byte[ 5 ];  ///< 预留5字节
        // 是否正确的header
        bool isValid() const;
    };

public:
    /**
     * @brief 序列化为QByteArray函数
     */
    using FpSerializeOut = std::function< QByteArray(const QwtPlotItem*) >;  // QByteArray serializeOut(QwtPlotItem* item)
    using FpSerializeIn = std::function< QwtPlotItem*(int rtti, const QByteArray&) >;  // QwtPlotItem* serializeIn(int rtti,const QByteArray& d)
public:
    DAChartItemSerialize();
    ~DAChartItemSerialize();
    /**
     * @brief 注册序列化函数
     * @param rtti item的rtti
     * @param fp 函数指针
     */
    void registSerialize(int rtti, FpSerializeOut fp);

    /**
     * @brief 序列化输出
     * @param item
     * @return
     */
    QByteArray serializeOut(const QwtPlotItem* item);

private:
    QHash< int, FpSerializeOut > mSerializeOut;
};

DAFIGURE_API QDataStream& operator<<(QDataStream& out, const DAChartItemSerialize::Header& f);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, DAChartItemSerialize::Header& f);

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

// QwtIntervalSample的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtIntervalSample& item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtIntervalSample& item);
// QwtInterval的序列化
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtInterval& item);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtInterval& item);
}
// 下面这两个要放到DA命名空间外，因为使用了QVector<T>的<<

#endif  // SAQWTSERIALIZE_H
