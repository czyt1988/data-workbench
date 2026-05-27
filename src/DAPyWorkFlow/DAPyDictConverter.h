#ifndef DAPYDICTCONVERTER_H
#define DAPYDICTCONVERTER_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyNodeStyle.h"
#include "DAPyNodeStyleDefine.h"
#include "DAPybind11InQt.h"

namespace DA
{
/**
 * @file DAPyDictConverter.h
 * @brief Python 对象到 C++ struct 的辅助转换工具
 *
 * 提供 colorFromPyObj 和 renderTemplateFromString 辅助函数，
 * 供 DAPyWorkFlowPythonBinding::toNodeStyle() 和 DAPyNode::getRenderTemplate() 使用。
 *
 * 已移除 nodeStyleFromDict 和 linkPointStyleFromDict，
 * 样式转换统一使用 PY::toNodeStyle() 直接从 Python NodeDisplay 属性读取。
 */

namespace DictConverter
{

/// 从字符串转换为 RenderTemplate 枚举
DAPYWORKFLOW_API DAPyNodeRenderTemplate renderTemplateFromString(const QString& s);

}  // namespace DictConverter

}  // namespace DA
#endif  // DAPYDICTCONVERTER_H
