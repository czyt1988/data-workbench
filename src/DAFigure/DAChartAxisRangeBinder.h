#ifndef DACHARTAXISRANGEBINDER_H
#define DACHARTAXISRANGEBINDER_H
#include "DAFigureAPI.h"
#include <QObject>
#include <QPointer>
#include <QMetaObject>
class QwtPlot;
namespace DA
{
/**
 * @brief 绘图坐标轴绑定器
 */
class DAFIGURE_API DAChartAxisRangeBinder : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DAChartAxisRangeBinder)
public:
    DAChartAxisRangeBinder(QObject* par = nullptr);
    DAChartAxisRangeBinder(QwtPlot* source, int sourceAxisid, QwtPlot* follower, int followerAxisid, QObject* par = nullptr);
    // 设置源绘图和对应坐标轴
    void setSourceChart(QwtPlot* source, int axisid);
    QwtPlot* getSourcePlot() const;
    int getSourceAxis() const;
    // 设置跟随绘图和对应坐标轴
    void setFollowerChart(QwtPlot* follower, int axisid);
    QwtPlot* getFollowerPlot() const;
    int getFollowerAxis() const;
    // 绑定
    bool bind();
    // 解绑
    bool unbind();
    // 是否绑定
    bool isBinded() const;
    // 是否立即刷新
    bool isRplotImmediately() const;
    void setReplotImmediately(bool v);
    // 是否有效
    bool isValid() const;
    // 是否相等
    bool isSame(const DAChartAxisRangeBinder& other) const;
    bool isSame(QwtPlot* source, int sourceAxisid, QwtPlot* follower, int followerAxisid) const;
    // 重载等号
    bool operator==(const DAChartAxisRangeBinder& other) const;

protected Q_SLOTS:
    void onSourcePlotScaleDivChanged();

private:
    bool m_isBinded { false };
    QPointer< QwtPlot > m_sourcePlot;
    QPointer< QwtPlot > m_followerPlot;
    int m_sourceAxisid { 4 };
    int m_followerAxisid { 4 };
    QMetaObject::Connection m_con;
    bool m_replotImmediately { true };  ///< 立即刷新
};
}

#endif  // DACHARTAXISRANGEBINDER_H
