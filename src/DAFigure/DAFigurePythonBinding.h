#ifndef DAFIGUREPYTHONBINDING_H
#define DAFIGUREPYTHONBINDING_H

#include "DAFigureAPI.h"

namespace DA
{
class DAChartWidget;
class DAFigureWidget;
}

namespace da_figure
{
/**
 * Function pointer type for getting the current active DAChartWidget.
 * Set during app initialization via setCurrentChartGetter().
 * Returns nullptr if no active chart exists.
 */
using GetCurrentChartFn = DA::DAChartWidget* (*)();

/**
 * @brief Set the callback function used by da_figure.getCurrentChart() to retrieve
 *        the current active DAChartWidget.
 *
 * This function is called from the APP module during Python module initialization
 * (see DAAppPythonBinding.cpp), because DAFigure cannot directly depend on the APP
 * module to access the application core interface chain.
 *
 * @param fn Function pointer that returns the current DAChartWidget*, or nullptr.
 */
void DAFIGURE_API setCurrentChartGetter(GetCurrentChartFn fn);

}  // namespace da_figure

#endif  // DAFIGUREPYTHONBINDING_H
