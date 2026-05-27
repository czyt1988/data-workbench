#include "DAPyDictConverter.h"
#include "DAPyWorkFlowEnumStringUtils.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyBindQt/DAPyJsonCast.h"
#include <QDebug>

namespace DA
{
namespace DictConverter
{

DAPyNodeRenderTemplate renderTemplateFromString(const QString& s)
{
    // 支持新旧格式：nodestyle/node_style/rect/svg → NodeStyleTemplate
    if (s == "nodestyle" || s == "node_style" || s == "rect" || s == "svg") {
        return DAPyNodeRenderTemplate::NodeStyleTemplate;
    }
    if (s == "widget") {
        return DAPyNodeRenderTemplate::WidgetTemplate;
    }
    // 尝试使用 stringToEnum
    return stringToEnum(s, DAPyNodeRenderTemplate::NodeStyleTemplate);
}

}  // namespace DictConverter
}  // namespace DA
