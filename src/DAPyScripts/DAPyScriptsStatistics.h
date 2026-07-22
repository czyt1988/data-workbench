#ifndef DAPYSCRIPTSSTATISTICS_H
#define DAPYSCRIPTSSTATISTICS_H
#include "DAPyScriptsGlobal.h"
#include "DAPyModule.h"
#include <QVariant>
#include <QString>
#include "DAPyObjectWrapper.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#include "DAUtils/DAPlotDataTypes.h"
namespace DA
{

/**
 * @brief Encapsulation of DAWorkbench.DAStatistics python package
 *
 * This wrapper provides C++ methods that call the atomic statistics
 * functions implemented in the DAStatistics Python subpackage.
 *
 * All Python functions return plain dicts (not pandas DataFrames).
 * Methods that return pybind11::dict pass the raw result through;
 * computeContours converts the dict to DA::DAContourData.
 */
class DAPYSCRIPTS_API DAPyScriptsStatistics : public DAPyModule
{
public:
    DAPyScriptsStatistics(bool autoImport = true);
    DAPyScriptsStatistics(const pybind11::object& obj);
    ~DAPyScriptsStatistics();
    // Import the DAWorkbench.DAStatistics module
    bool import();

public:
    // distribution.py — compute_histogram
    pybind11::dict computeHistogram(const DAPySeries& data,
                                   const QVariantMap& args,
                                   QString* err = nullptr);
    // distribution.py — compute_ecdf
    pybind11::dict computeECDF(const DAPySeries& data,
                               const QVariantMap& args,
                               QString* err = nullptr);

    // kde.py — compute_kde_1d
    pybind11::dict computeKde1d(const DAPySeries& data,
                                const QVariantMap& args,
                                QString* err = nullptr);
    // kde.py — compute_kde_2d
    pybind11::dict computeKde2d(const DAPySeries& x,
                                const DAPySeries& y,
                                const QVariantMap& args,
                                QString* err = nullptr);
    // kde.py — compute_contours
    // @param kdeResult  dict returned by computeKde2d (must contain Z, x_grid, y_grid)
    DA::DAContourData computeContours(const pybind11::dict& kdeResult,
                                      const QVariantMap& args,
                                      QString* err = nullptr);

    // regression.py — fit_polynomial
    pybind11::dict fitPolynomial(const DAPySeries& x,
                                 const DAPySeries& y,
                                 const QVariantMap& args,
                                 QString* err = nullptr);
    // regression.py — compute_bootstrap_ci
    pybind11::dict computeBootstrapCI(const DAPySeries& x,
                                      const DAPySeries& y,
                                      const QVariantMap& args,
                                      QString* err = nullptr);

    // categorical.py — aggregate_by_category
    // @param xCol  categorical (X-axis) column name
    // @param yCol  value column name; empty string for count plot (y_col=None in Python)
    pybind11::dict aggregateByCategory(const DAPyDataFrame& df,
                                       const QString& xCol,
                                       const QString& yCol,
                                       const QVariantMap& args,
                                       QString* err = nullptr);
    // categorical.py — compute_ci
    pybind11::dict computeCI(const DAPyDataFrame& df,
                             const QVariantMap& args,
                             QString* err = nullptr);

    // boxplot_stats.py — compute_boxplot_stats
    pybind11::dict computeBoxplotStats(const DAPyDataFrame& df,
                                       const QVariantMap& args,
                                       QString* err = nullptr);

    // matrix.py — compute_pivot_matrix
    // @param valuesCol  value column name; empty string for count matrix (values_col=None in Python)
    pybind11::dict computePivotMatrix(const DAPyDataFrame& df,
                                      const QString& indexCol,
                                      const QString& columnsCol,
                                      const QString& valuesCol,
                                      const QVariantMap& args,
                                      QString* err = nullptr);
};

}  // namespace DA
#endif  // DAPYSCRIPTSSTATISTICS_H
