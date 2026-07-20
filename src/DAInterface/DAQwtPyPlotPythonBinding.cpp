// DAQwtPyPlotPythonBinding.cpp - pybind11 binding for the da_pyplot embedded module
//
// This module exposes QwtPyPlot (a matplotlib pyplot-style API for Qwt) to Python.
// Python scripts can use it like matplotlib.pyplot for quick, high-level plotting.
//
// Architecture:
//   - QwtPyPlot is bound as the "PyPlot" class with all its plotting/decoration methods
//   - QwtPlot is registered minimally for type recognition (returned by subplot/twinx)
//   - Plotting methods return QwtPlotItem* (registered in da_figure module)
//     → Python users must `import da_figure` before using return values with da_figure.setPen() etc.
//   - Module-level gca(chart) creates a QwtPyPlot from a DAChartWidget*
//   - Module-level gca() (no args) reuses da_figure's chart-getter callback

// DAPybind11QtCaster.hpp includes DAPybind11InQt.h as its first header,
// which resolves the Qt "slots" vs pybind11 conflict.
#include "DAPybind11QtCaster.hpp"

// DA headers
#include "DAChartWidget.h"
#include "DAFigurePythonBinding.h"

// QwtPyPlot header
#include "qwt_pyplot.h"
#include "qwt_figure.h"

// Qwt plot item headers (for return type casts)
#include "qwt_plot.h"
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_boxchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_arrowmarker.h"
#include "qwt_plot_legenditem.h"

// Qwt core headers
#include "qwt_samples.h"

// Qt headers
#include <QPointF>
#include <QStringList>

namespace {

// ==================== Utility helpers ====================

static double dictGetDouble(const pybind11::dict& d, const char* key, double defaultValue)
{
    if (!d.contains(key)) return defaultValue;
    pybind11::object val = d[pybind11::str(key)];
    if (val.is_none()) return defaultValue;
    try {
        return val.cast< double >();
    } catch (...) {
        return defaultValue;
    }
}

// ==================== Conversion helpers ====================

// Convert pybind11::list of dicts to QVector<QwtBoxSample>
// Each dict has keys: position, whiskerLower, q1, median, q3, whiskerUpper
static QVector< QwtBoxSample > convertToBoxSamples(const pybind11::list& samples)
{
    QVector< QwtBoxSample > result;
    result.reserve(static_cast< int >(samples.size()));
    for (auto item : samples) {
        if (!pybind11::isinstance< pybind11::dict >(item)) continue;
        pybind11::dict d = item.cast< pybind11::dict >();
        double position     = dictGetDouble(d, "position", 0.0);
        double whiskerLower = dictGetDouble(d, "whiskerLower", 0.0);
        double q1           = dictGetDouble(d, "q1", 0.0);
        double median       = dictGetDouble(d, "median", 0.0);
        double q3           = dictGetDouble(d, "q3", 0.0);
        double whiskerUpper = dictGetDouble(d, "whiskerUpper", 0.0);
        result.append(QwtBoxSample(position, whiskerLower, q1, median, q3, whiskerUpper));
    }
    return result;
}

// Convert pybind11::list of dicts to QVector<QwtOHLCSample>
// Each dict has keys: time, open, high, low, close
static QVector< QwtOHLCSample > convertToOHLCSamples(const pybind11::list& samples)
{
    QVector< QwtOHLCSample > result;
    result.reserve(static_cast< int >(samples.size()));
    for (auto item : samples) {
        if (!pybind11::isinstance< pybind11::dict >(item)) continue;
        pybind11::dict d = item.cast< pybind11::dict >();
        double time  = dictGetDouble(d, "time", 0.0);
        double open  = dictGetDouble(d, "open", 0.0);
        double high  = dictGetDouble(d, "high", 0.0);
        double low   = dictGetDouble(d, "low", 0.0);
        double close = dictGetDouble(d, "close", 0.0);
        result.append(QwtOHLCSample(time, open, high, low, close));
    }
    return result;
}

// Convert pybind11::list (2D) to QVector<QVector<double>>
static QVector< QVector< double > > convertTo2DMatrix(const pybind11::list& data)
{
    QVector< QVector< double > > result;
    int numRows = static_cast< int >(data.size());
    if (numRows == 0) return result;
    result.resize(numRows);
    for (int i = 0; i < numRows; ++i) {
        if (pybind11::isinstance< pybind11::list >(data[i])) {
            pybind11::list row = data[i].cast< pybind11::list >();
            int numCols = static_cast< int >(row.size());
            result[i].resize(numCols);
            for (int j = 0; j < numCols; ++j) {
                result[i][j] = row[j].cast< double >();
            }
        } else if (pybind11::isinstance< pybind11::tuple >(data[i])) {
            pybind11::tuple row = data[i].cast< pybind11::tuple >();
            int numCols = static_cast< int >(row.size());
            result[i].resize(numCols);
            for (int j = 0; j < numCols; ++j) {
                result[i][j] = row[j].cast< double >();
            }
        }
    }
    return result;
}

// Convert pybind11::list to QList<double>
static QList< double > convertToDoubleList(const pybind11::list& data)
{
    QList< double > result;
    result.reserve(static_cast< int >(data.size()));
    for (auto item : data) {
        result.append(item.cast< double >());
    }
    return result;
}

// Convert pybind11::object (list/tuple of 2 doubles) to QPointF
static QPointF convertToPointF(const pybind11::object& obj)
{
    if (pybind11::isinstance< pybind11::list >(obj)) {
        pybind11::list l = obj.cast< pybind11::list >();
        if (l.size() >= 2) {
            return QPointF(l[0].cast< double >(), l[1].cast< double >());
        }
    } else if (pybind11::isinstance< pybind11::tuple >(obj)) {
        pybind11::tuple t = obj.cast< pybind11::tuple >();
        if (t.size() >= 2) {
            return QPointF(t[0].cast< double >(), t[1].cast< double >());
        }
    }
    return QPointF();
}

}  // anonymous namespace

// ==================== pybind11 module ====================

PYBIND11_EMBEDDED_MODULE(da_pyplot, m)
{
    m.doc() = "QwtPyPlot - matplotlib pyplot style API for Qwt";

    // QwtPlot — minimal binding for type recognition.
    // Returned by subplot(), twinx(), twiny(), gca() methods.
    // Registered here (not in da_figure) so Python can pass it back to sca()/twinx().
    pybind11::class_< QwtPlot >(m, "QwtPlot")
        .def("replot", &QwtPlot::replot, "Redraw the plot.");

    // QwtPyPlot — the main pyplot-style class
    // Note: plotting methods return QwtPlotItem* (registered in da_figure module).
    //       Python users should `import da_figure` before using return values.
    pybind11::class_< QwtPyPlot >(m, "PyPlot")
        // ---- Constructors ----
        // Construct from a DAChartWidget (upcast to QwtPlot since DAChartWidget inherits QwtPlot)
        .def(pybind11::init< QwtPlot* >(), pybind11::arg("plot"),
             "Create a PyPlot for a single QwtPlot (single-plot mode).")
        // Construct from a QwtFigure (multi-subplot mode)
        .def(pybind11::init< QwtFigure* >(), pybind11::arg("figure"),
             "Create a PyPlot for a QwtFigure (multi-subplot mode).")

        // ---- State management ----
        .def("gcf", &QwtPyPlot::gcf, pybind11::return_value_policy::reference,
             "Get the current figure (like matplotlib's gcf).")
        .def("gca", &QwtPyPlot::gca, pybind11::return_value_policy::reference,
             "Get the current axes (like matplotlib's gca). Returns a QwtPlot.")
        .def("sca", &QwtPyPlot::sca, pybind11::arg("plot"),
             "Set the current axes (like matplotlib's sca).")

        // ---- Figure operations ----
        .def("subplot", &QwtPyPlot::subplot, pybind11::arg("rows"), pybind11::arg("cols"), pybind11::arg("index"),
             pybind11::return_value_policy::reference,
             "Create a subplot in a grid layout (1-based index). Returns a QwtPlot.")
        .def("addAxes", &QwtPyPlot::addAxes, pybind11::arg("rect") = QRectF(0.1, 0.1, 0.8, 0.8),
             pybind11::return_value_policy::reference,
             "Add axes at a normalized rectangle position. Returns a QwtPlot.")
        .def("twinx", &QwtPyPlot::twinx, pybind11::arg("host") = nullptr,
             pybind11::return_value_policy::reference,
             "Create a twin Y-axis (like matplotlib's twinx). Returns a QwtPlot.")
        .def("twiny", &QwtPyPlot::twiny, pybind11::arg("host") = nullptr,
             pybind11::return_value_policy::reference,
             "Create a twin X-axis (like matplotlib's twiny). Returns a QwtPlot.")
        .def("tightLayout", &QwtPyPlot::tightLayout,
             "Apply tight layout to align all subplot axes.")

        // ---- Plotting methods ----
        // plot() — 3 overloads
        .def("plot",
             [](QwtPyPlot& self, const QVector< double >& y, const QString& fmt, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.plot(y, fmt, label));
             },
             pybind11::arg("y"), pybind11::arg("fmt") = QString(), pybind11::arg("label") = QString(),
             "Plot y-only data (x auto-generated as index). Returns a PlotItem.")
        .def("plot",
             [](QwtPyPlot& self, const QVector< double >& x, const QVector< double >& y, const QString& fmt, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.plot(x, y, fmt, label));
             },
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("fmt") = QString(), pybind11::arg("label") = QString(),
             "Plot x-y data. Returns a PlotItem.")
        .def("plot",
             [](QwtPyPlot& self, const QVector< QPointF >& data, const QString& fmt, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.plot(data, fmt, label));
             },
             pybind11::arg("data"), pybind11::arg("fmt") = QString(), pybind11::arg("label") = QString(),
             "Plot from QPointF data. Returns a PlotItem.")

        // scatter()
        .def("scatter",
             [](QwtPyPlot& self, const QVector< double >& x, const QVector< double >& y, double size, const QString& color, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.scatter(x, y, size, color, label));
             },
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("size") = 20,
             pybind11::arg("color") = QString(), pybind11::arg("label") = QString(),
             "Scatter plot (markers only, no lines). Returns a PlotItem.")

        // bar() — 2 overloads
        .def("bar",
             [](QwtPyPlot& self, const QVector< double >& values, const QString& color, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.bar(values, color, label));
             },
             pybind11::arg("values"), pybind11::arg("color") = QString(), pybind11::arg("label") = QString(),
             "Bar chart from y-only values (x = index). Returns a PlotItem.")
        .def("bar",
             [](QwtPyPlot& self, const QVector< double >& x, const QVector< double >& values, double width, const QString& color, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.bar(x, values, width, color, label));
             },
             pybind11::arg("x"), pybind11::arg("values"), pybind11::arg("width") = 0.8,
             pybind11::arg("color") = QString(), pybind11::arg("label") = QString(),
             "Bar chart from x-y data with configurable width. Returns a PlotItem.")

        // hist()
        .def("hist",
             [](QwtPyPlot& self, const QVector< double >& data, int bins, const QString& color, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.hist(data, bins, color, label));
             },
             pybind11::arg("data"), pybind11::arg("bins") = 10,
             pybind11::arg("color") = QString(), pybind11::arg("label") = QString(),
             "Histogram from raw data with automatic binning. Returns a PlotItem.")

        // boxplot() — accepts list of dicts
        .def("boxplot",
             [](QwtPyPlot& self, const pybind11::list& data, const QString& label) -> QwtPlotItem* {
                 QVector< QwtBoxSample > samples = convertToBoxSamples(data);
                 return static_cast< QwtPlotItem* >(self.boxplot(samples, label));
             },
             pybind11::arg("data"), pybind11::arg("label") = QString(),
             "Box plot from pre-computed box samples. Each sample is a dict with keys: "
             "position, whiskerLower, q1, median, q3, whiskerUpper. Returns a PlotItem.")

        // fillBetween()
        .def("fillBetween",
             [](QwtPyPlot& self, const QVector< double >& x, const QVector< double >& y1, const QVector< double >& y2, const QString& color, double alpha) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.fillBetween(x, y1, y2, color, alpha));
             },
             pybind11::arg("x"), pybind11::arg("y1"), pybind11::arg("y2"),
             pybind11::arg("color") = QString(), pybind11::arg("alpha") = 0.3,
             "Fill the area between two curves. Returns a PlotItem.")

        // errorbar()
        .def("errorbar",
             [](QwtPyPlot& self, const QVector< double >& x, const QVector< double >& y, const QVector< double >& yerr, const QString& fmt, const QString& label) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.errorbar(x, y, yerr, fmt, label));
             },
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("yerr"),
             pybind11::arg("fmt") = QString(), pybind11::arg("label") = QString(),
             "Error bars (symmetric y-error). Returns a PlotItem.")

        // imshow() — accepts 2D list
        .def("imshow",
             [](QwtPyPlot& self, const pybind11::list& data, const QString& cmap, double vmin, double vmax) -> QwtPlotItem* {
                 QVector< QVector< double > > matrix = convertTo2DMatrix(data);
                 return static_cast< QwtPlotItem* >(self.imshow(matrix, cmap, vmin, vmax));
             },
             pybind11::arg("data"), pybind11::arg("cmap") = "viridis",
             pybind11::arg("vmin") = 0.0, pybind11::arg("vmax") = 0.0,
             "Display a 2D matrix as a color-mapped image. Returns a PlotItem.")

        // contour() — accepts 2D list and optional levels list
        .def("contour",
             [](QwtPyPlot& self, const pybind11::list& data, const pybind11::list& levels, const QString& cmap) -> QwtPlotItem* {
                 QVector< QVector< double > > matrix = convertTo2DMatrix(data);
                 QList< double > levelsList = convertToDoubleList(levels);
                 return static_cast< QwtPlotItem* >(self.contour(matrix, levelsList, cmap));
             },
             pybind11::arg("data"), pybind11::arg("levels") = pybind11::list(),
             pybind11::arg("cmap") = "viridis",
             "Draw contour lines from a 2D matrix. Returns a PlotItem.")

        // quiver()
        .def("quiver",
             [](QwtPyPlot& self, const QVector< double >& x, const QVector< double >& y, const QVector< double >& u, const QVector< double >& v, const QString& color) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.quiver(x, y, u, v, color));
             },
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("u"), pybind11::arg("v"),
             pybind11::arg("color") = QString(),
             "Quiver plot (vector field). Returns a PlotItem.")

        // candlestick() — accepts list of dicts
        .def("candlestick",
             [](QwtPyPlot& self, const pybind11::list& data, const QString& label) -> QwtPlotItem* {
                 QVector< QwtOHLCSample > samples = convertToOHLCSamples(data);
                 return static_cast< QwtPlotItem* >(self.candlestick(samples, label));
             },
             pybind11::arg("data"), pybind11::arg("label") = QString(),
             "Candlestick (OHLC) chart. Each sample is a dict with keys: "
             "time, open, high, low, close. Returns a PlotItem.")

        // ---- Auxiliary elements ----
        .def("grid",
             [](QwtPyPlot& self, bool show, bool minor) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.grid(show, minor));
             },
             pybind11::arg("show") = true, pybind11::arg("minor") = false,
             "Add or remove a grid. Returns a PlotItem.")
        .def("axhline",
             [](QwtPyPlot& self, double y, const QString& fmt) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.axhline(y, fmt));
             },
             pybind11::arg("y"), pybind11::arg("fmt") = QString(),
             "Add a horizontal line at y. Returns a PlotItem.")
        .def("axvline",
             [](QwtPyPlot& self, double x, const QString& fmt) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.axvline(x, fmt));
             },
             pybind11::arg("x"), pybind11::arg("fmt") = QString(),
             "Add a vertical line at x. Returns a PlotItem.")
        .def("axhspan",
             [](QwtPyPlot& self, double y1, double y2, const QString& color, double alpha) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.axhspan(y1, y2, color, alpha));
             },
             pybind11::arg("y1"), pybind11::arg("y2"),
             pybind11::arg("color") = QString(), pybind11::arg("alpha") = 0.3,
             "Add a horizontal colored span between y1 and y2. Returns a PlotItem.")
        .def("axvspan",
             [](QwtPyPlot& self, double x1, double x2, const QString& color, double alpha) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.axvspan(x1, x2, color, alpha));
             },
             pybind11::arg("x1"), pybind11::arg("x2"),
             pybind11::arg("color") = QString(), pybind11::arg("alpha") = 0.3,
             "Add a vertical colored span between x1 and x2. Returns a PlotItem.")
        .def("annotate",
             [](QwtPyPlot& self, const QString& text, const pybind11::object& xy, const pybind11::object& xytext) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.annotate(text, convertToPointF(xy), convertToPointF(xytext)));
             },
             pybind11::arg("text"), pybind11::arg("xy"), pybind11::arg("xytext"),
             "Add an arrow annotation from xytext to xy. Returns a PlotItem.")
        .def("legend",
             [](QwtPyPlot& self, const QString& loc) -> QwtPlotItem* {
                 return static_cast< QwtPlotItem* >(self.legend(loc));
             },
             pybind11::arg("loc") = "best",
             "Add a legend (in-canvas legend item). Returns a PlotItem.")

        // ---- Axis configuration ----
        .def("setTitle", &QwtPyPlot::setTitle, pybind11::arg("title"), "Set the plot title.")
        .def("setXLabel", &QwtPyPlot::setXLabel, pybind11::arg("label"), "Set the X-axis label.")
        .def("setYLabel", &QwtPyPlot::setYLabel, pybind11::arg("label"), "Set the Y-axis label.")
        .def("setXLim", &QwtPyPlot::setXLim, pybind11::arg("min"), pybind11::arg("max"), "Set X-axis limits.")
        .def("setYLim", &QwtPyPlot::setYLim, pybind11::arg("min"), pybind11::arg("max"), "Set Y-axis limits.")
        .def("setXScale", &QwtPyPlot::setXScale, pybind11::arg("scale"), "Set X-axis scale type ('linear' or 'log').")
        .def("setYScale", &QwtPyPlot::setYScale, pybind11::arg("scale"), "Set Y-axis scale type ('linear' or 'log').")
        .def("setXTicks", &QwtPyPlot::setXTicks, pybind11::arg("ticks"), pybind11::arg("labels") = QStringList(),
             "Set custom X-axis tick positions and optional labels.")
        .def("setYTicks", &QwtPyPlot::setYTicks, pybind11::arg("ticks"), pybind11::arg("labels") = QStringList(),
             "Set custom Y-axis tick positions and optional labels.")
        .def("invertXAxis", &QwtPyPlot::invertXAxis, "Invert the X-axis.")
        .def("invertYAxis", &QwtPyPlot::invertYAxis, "Invert the Y-axis.")

        // ---- Appearance ----
        .def("setFaceColor", &QwtPyPlot::setFaceColor, pybind11::arg("color"), "Set figure background color.")
        .def("setAxesColor", &QwtPyPlot::setAxesColor, pybind11::arg("color"), "Set axes canvas background color.")
        .def("colorbar", &QwtPyPlot::colorbar, pybind11::arg("spectro") = nullptr,
             "Add a colorbar for a spectrogram.")

        // ---- Output ----
        .def("savefig", &QwtPyPlot::savefig, pybind11::arg("filename"), pybind11::arg("dpi") = -1,
             "Save figure to file. Returns True on success.")
        .def("show", &QwtPyPlot::show, "Show the figure or plot widget.")

        // ---- Interaction ----
        .def("enablePan", &QwtPyPlot::enablePan, pybind11::arg("enable") = true,
             "Enable/disable canvas panning.")
        .def("enableZoom", &QwtPyPlot::enableZoom, pybind11::arg("enable") = true,
             "Enable/disable canvas zooming.")
        ;

    // ==================== Module-level convenience functions ====================

    // gca(chart) — create a PyPlot from a DAChartWidget
    // DAChartWidget inherits QwtPlot, so we upcast.
    m.def("gca",
          [](DA::DAChartWidget* chart) -> QwtPyPlot* {
              if (!chart) {
                  throw std::runtime_error("da_pyplot.gca: chart is None");
              }
              return new QwtPyPlot(static_cast< QwtPlot* >(chart));
          },
          pybind11::arg("chart"),
          pybind11::return_value_policy::take_ownership,
          "Create a PyPlot from a DAChartWidget. The PyPlot operates in single-plot mode.");

    // gca() — no args, reuse da_figure's chart-getter callback
    m.def("gca",
          []() -> QwtPyPlot* {
              DA::DAChartWidget* chart = da_figure::getCurrentChartWidget();
              if (!chart) {
                  throw std::runtime_error("da_pyplot.gca: no active chart available");
              }
              return new QwtPyPlot(static_cast< QwtPlot* >(chart));
          },
          pybind11::return_value_policy::take_ownership,
          "Create a PyPlot from the current active chart (via da_figure callback).");
}
