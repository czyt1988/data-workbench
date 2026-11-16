#ifndef DAPEAKMARKERSAMPLES_H
#define DAPEAKMARKERSAMPLES_H
#include <QPointF>
#include <QVector>

namespace DA
{
// 峰值标记吸附：把峰值频率落到「当前已绘制频谱曲线」的最近局部极大点上（返回 x=频率、y=曲线显示值）
QVector< QPointF > snapPeakMarkersToCurve(const QVector< double >& curveFreqs,
                                          const QVector< double >& curveValues,
                                          const QVector< double >& peakFreqs);
}  // namespace DA

#endif  // DAPEAKMARKERSAMPLES_H
