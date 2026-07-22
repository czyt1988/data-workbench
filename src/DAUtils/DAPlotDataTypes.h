#pragma once
#include <QVector>
#include <QStringList>
#include <QPolygonF>
#include <QPointF>
#include <QList>

namespace DA {

// 箱线图数据
struct DABoxPlotData {
    QVector<double> positions;
    QVector<double> q1;
    QVector<double> median;
    QVector<double> q3;
    QVector<double> whiskerLower;
    QVector<double> whiskerUpper;
    QVector<QVector<QPointF>> outliers;
};

// 柱状图数据（含可选误差棒）
struct DABarChartData {
    QStringList categories;
    QVector<double> values;
    QVector<double> ciLower;
    QVector<double> ciUpper;
};

// 等高线数据
struct DAContourData {
    QList<QPolygonF> polygons;
    QVector<double> levels;
};

// 区间曲线数据（CI边界等）
struct DAIntervalCurveData {
    QVector<double> x;
    QVector<double> yLower;
    QVector<double> yUpper;
};

// 2D光谱图数据（heatmap/kdeplot_2d用）
struct DASpectrogramData {
    QVector<double> values;
    int nrows;
    int ncols;
    double xmin, xmax, ymin, ymax;
    QStringList rowLabels;
    QStringList colLabels;
};

} // namespace DA
