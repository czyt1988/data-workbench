#include "DAPyScriptsStatistics.h"
#include "DAPybind11QtCaster.hpp"
#include "DALogCategory.h"

namespace DA
{

//===================================================
// DAPyScriptsStatistics
//===================================================

/** @brief 构造DAPyScriptsStatistics @param autoImport 是否自动导入模块 */
DAPyScriptsStatistics::DAPyScriptsStatistics(bool autoImport) : DAPyModule()
{
    if (autoImport) {
        if (!import()) {
            daCritical << QObject::tr("cannot import DAWorkbench.DAStatistics module");  // cn:无法导入 DAWorkbench.DAStatistics 模块
        }
    }
}

/** @brief 构造DAPyScriptsStatistics @param obj Python模块对象 */
DAPyScriptsStatistics::DAPyScriptsStatistics(const pybind11::object& obj) : DAPyModule(obj)
{
    if (!isModule()) {
        daCritical << QObject::tr("cannot import DAWorkbench.DAStatistics");  // cn:无法导入 DAWorkbench.DAStatistics 模块
    }
}

/** @brief 析构DAPyScriptsStatistics */
DAPyScriptsStatistics::~DAPyScriptsStatistics()
{
}

/** @brief 导入DAWorkbench.DAStatistics模块 @return 导入成功返回true */
bool DAPyScriptsStatistics::import()
{
    try {
        pybind11::module m = pybind11::module::import("DAWorkbench.DAStatistics");
        object()           = m;
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}

//===================================================
// distribution.py
//===================================================

/** @brief 计算直方图 @param data 输入数据 @param args 计算参数 @param err 错误信息输出 @return 包含bins和counts的字典 */
pybind11::dict DAPyScriptsStatistics::computeHistogram(const DAPySeries& data,
                                                       const QVariantMap& args,
                                                       QString* err)
{
    try {
        pybind11::object fn = attr("compute_histogram");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_histogram";
            return pybind11::dict();
        }
        pybind11::object v = fn(data.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 按类别计算直方图 @param df 数据表 @param dataCol 数据列名 @param hueCol 分类列名 @param args 计算参数 @param err 错误信息输出 @return 按类别分组的直方图结果字典 */
pybind11::dict DAPyScriptsStatistics::computeHistogramByHue(const DAPyDataFrame& df,
                                                            const QString& dataCol,
                                                            const QString& hueCol,
                                                            const QVariantMap& args,
                                                            QString* err)
{
    try {
        pybind11::object fn = attr("compute_histogram_by_hue");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_histogram_by_hue";
            return pybind11::dict();
        }
        pybind11::object v = fn(df.object(), pybind11::cast(dataCol), pybind11::cast(hueCol), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 计算经验累积分布函数 @param data 输入数据 @param args 计算参数 @param err 错误信息输出 @return 包含x和y的ECDF结果字典 */
pybind11::dict DAPyScriptsStatistics::computeECDF(const DAPySeries& data,
                                                  const QVariantMap& args,
                                                  QString* err)
{
    try {
        pybind11::object fn = attr("compute_ecdf");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_ecdf";
            return pybind11::dict();
        }
        pybind11::object v = fn(data.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

//===================================================
// kde.py
//===================================================

/** @brief 计算一维核密度估计 @param data 输入数据 @param args 计算参数 @param err 错误信息输出 @return 包含x和pdf的核密度估计结果字典 */
pybind11::dict DAPyScriptsStatistics::computeKde1d(const DAPySeries& data,
                                                   const QVariantMap& args,
                                                   QString* err)
{
    try {
        pybind11::object fn = attr("compute_kde_1d");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_kde_1d";
            return pybind11::dict();
        }
        pybind11::object v = fn(data.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 按类别计算一维核密度估计 @param df 数据表 @param dataCol 数据列名 @param hueCol 分类列名 @param args 计算参数 @param err 错误信息输出 @return 按类别分组的一维核密度估计结果字典 */
pybind11::dict DAPyScriptsStatistics::computeKde1dByHue(const DAPyDataFrame& df,
                                                        const QString& dataCol,
                                                        const QString& hueCol,
                                                        const QVariantMap& args,
                                                        QString* err)
{
    try {
        pybind11::object fn = attr("compute_kde_1d_by_hue");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_kde_1d_by_hue";
            return pybind11::dict();
        }
        pybind11::object v = fn(df.object(), pybind11::cast(dataCol), pybind11::cast(hueCol), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 计算二维核密度估计 @param x x轴数据 @param y y轴数据 @param args 计算参数 @param err 错误信息输出 @return 包含Z、x_grid、y_grid的二维核密度估计结果字典 */
pybind11::dict DAPyScriptsStatistics::computeKde2d(const DAPySeries& x,
                                                   const DAPySeries& y,
                                                   const QVariantMap& args,
                                                   QString* err)
{
    try {
        pybind11::object fn = attr("compute_kde_2d");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_kde_2d";
            return pybind11::dict();
        }
        pybind11::object v = fn(x.object(), y.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 从KDE结果计算等高线 @param kdeResult KDE结果字典 @param args 计算参数 @param err 错误信息输出 @return 等高线数据，包含levels和polygons */
DA::DAContourData DAPyScriptsStatistics::computeContours(const pybind11::dict& kdeResult,
                                                        const QVariantMap& args,
                                                        QString* err)
{
    DA::DAContourData result;
    try {
        pybind11::object fn = attr("compute_contours");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_contours";
            return result;
        }
        // Extract Z, x_grid, y_grid from the kde_2d result dict
        pybind11::object Z       = kdeResult["Z"];
        pybind11::object x_grid  = kdeResult["x_grid"];
        pybind11::object y_grid  = kdeResult["y_grid"];
        pybind11::object v       = fn(Z, x_grid, y_grid, pybind11::cast(args));
        if (v.is_none()) {
            return result;
        }
        pybind11::dict resDict = v.cast< pybind11::dict >();

        // Extract levels
        pybind11::object levelsObj = resDict["levels"];
        if (!levelsObj.is_none() && pybind11::isinstance< pybind11::list >(levelsObj)) {
            auto levelsList = pybind11::cast< pybind11::list >(levelsObj);
            for (auto item : levelsList) {
                result.levels.append(pybind11::cast< double >(item));
            }
        }

        // Extract polygons
        pybind11::object polysObj = resDict["polygons"];
        if (!polysObj.is_none() && pybind11::isinstance< pybind11::list >(polysObj)) {
            auto polysList = pybind11::cast< pybind11::list >(polysObj);
            for (auto poly : polysList) {
                // Each polygon is a list of [x, y] pairs
                if (pybind11::isinstance< pybind11::list >(poly)) {
                    QPolygonF qpoly;
                    auto vertexList = pybind11::cast< pybind11::list >(poly);
                    for (auto vertex : vertexList) {
                        // vertex is [x, y] — use QPointF type caster
                        QPointF pt = pybind11::cast< QPointF >(vertex);
                        qpoly.append(pt);
                    }
                    if (qpoly.size() >= 3) {
                        result.polygons.append(qpoly);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return result;
}

//===================================================
// regression.py
//===================================================

/** @brief 多项式拟合 @param x x轴数据 @param y y轴数据 @param args 拟合参数 @param err 错误信息输出 @return 包含系数和R方的拟合结果字典 */
pybind11::dict DAPyScriptsStatistics::fitPolynomial(const DAPySeries& x,
                                                    const DAPySeries& y,
                                                    const QVariantMap& args,
                                                    QString* err)
{
    try {
        pybind11::object fn = attr("fit_polynomial");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr fit_polynomial";
            return pybind11::dict();
        }
        pybind11::object v = fn(x.object(), y.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 计算Bootstrap置信区间 @param x x轴数据 @param y y轴数据 @param args 计算参数 @param err 错误信息输出 @return 包含置信区间信息的Bootstrap结果字典 */
pybind11::dict DAPyScriptsStatistics::computeBootstrapCI(const DAPySeries& x,
                                                         const DAPySeries& y,
                                                         const QVariantMap& args,
                                                         QString* err)
{
    try {
        pybind11::object fn = attr("compute_bootstrap_ci");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_bootstrap_ci";
            return pybind11::dict();
        }
        pybind11::object v = fn(x.object(), y.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

//===================================================
// categorical.py
//===================================================

/** @brief 按类别聚合数据 @param df 数据表 @param xCol x轴列名 @param yCol y轴列名 @param args 聚合参数 @param err 错误信息输出 @return 按类别聚合的结果字典 */
pybind11::dict DAPyScriptsStatistics::aggregateByCategory(const DAPyDataFrame& df,
                                                           const QString& xCol,
                                                           const QString& yCol,
                                                           const QVariantMap& args,
                                                           QString* err)
{
    try {
        pybind11::object fn = attr("aggregate_by_category");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr aggregate_by_category";
            return pybind11::dict();
        }
        // yCol empty → pass None to trigger count plot in Python
        pybind11::object yColObj = yCol.isEmpty() ? pybind11::none() : pybind11::cast(yCol);
        pybind11::object v = fn(df.object(), pybind11::cast(xCol), yColObj, pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 计算置信区间 @param df 数据表 @param args 计算参数 @param err 错误信息输出 @return 置信区间结果字典 */
pybind11::dict DAPyScriptsStatistics::computeCI(const DAPyDataFrame& df,
                                                const QVariantMap& args,
                                                QString* err)
{
    try {
        pybind11::object fn = attr("compute_ci");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_ci";
            return pybind11::dict();
        }
        pybind11::object v = fn(df.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

//===================================================
// boxplot_stats.py
//===================================================

/** @brief 计算箱线图统计量 @param df 数据表 @param args 计算参数 @param err 错误信息输出 @return 箱线图统计信息字典 */
pybind11::dict DAPyScriptsStatistics::computeBoxplotStats(const DAPyDataFrame& df,
                                                          const QVariantMap& args,
                                                          QString* err)
{
    try {
        pybind11::object fn = attr("compute_boxplot_stats");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_boxplot_stats";
            return pybind11::dict();
        }
        pybind11::object v = fn(df.object(), pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

//===================================================
// matrix.py
//===================================================

/** @brief 计算透视矩阵 @param df 数据表 @param indexCol 行索引列名 @param columnsCol 列索引列名 @param valuesCol 值列名 @param args 计算参数 @param err 错误信息输出 @return 透视矩阵结果字典 */
pybind11::dict DAPyScriptsStatistics::computePivotMatrix(const DAPyDataFrame& df,
                                                         const QString& indexCol,
                                                         const QString& columnsCol,
                                                         const QString& valuesCol,
                                                         const QVariantMap& args,
                                                         QString* err)
{
    try {
        pybind11::object fn = attr("compute_pivot_matrix");
        if (fn.is_none()) {
            qDebug() << "DAStatistics have no attr compute_pivot_matrix";
            return pybind11::dict();
        }
        // valuesCol empty → pass None for count matrix in Python
        pybind11::object valuesColObj = valuesCol.isEmpty() ? pybind11::none() : pybind11::cast(valuesCol);
        pybind11::object v = fn(df.object(),
                                pybind11::cast(indexCol),
                                pybind11::cast(columnsCol),
                                valuesColObj,
                                pybind11::cast(args));
        if (v.is_none()) {
            return pybind11::dict();
        }
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

}  // namespace DA
