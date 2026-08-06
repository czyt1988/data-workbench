#pragma once
#include "DAAgentToolBase.h"   // fat base class (plan-04 will slim it and move chart methods here)
#include <QObject>
namespace DA
{
/**
 * @brief Chart tool base class
 *
 * Inherits DAAgentToolBase (which currently holds the data/response methods AND the
 * chart methods, as a fat base). plan-04 will move the chart methods out of
 * DAAgentToolBase into this class; at that point DAAgentToolBase becomes the slim
 * data-only base and DAAgentChartToolBase becomes the owner of chart methods.
 *
 * The 8 built-in chart tools inherit this class so that, after plan-04, they pick up
 * the chart methods without depending on DAAgent's link to DAGui/DAFigure.
 */
class DAAgentChartToolBase : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;  // inherit constructor
};
}  // namespace DA
