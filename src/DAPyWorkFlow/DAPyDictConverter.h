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
 * @brief Python dict 到 C++ struct 的临时转换工具
 *
 * DAPyNode 作为纯代理，每次通过 attr() 从 Python 对象读取属性，
 * 样式等复合类型在 Python 端为 dict，
 * 本模块提供 dict→C++ struct 的临时转换函数（不做缓存）。
 *
 * Python dict 键使用 snake_case（data_type, body_shape, fill_color 等），
 * 枚举值使用字符串（"Ellipse", "Circle", "North" 等）。
 */

namespace DictConverter
{

/// 从 Python dict 转换为 DAPyLinkPointStyle
DAPYWORKFLOW_API DAPyLinkPointStyle linkPointStyleFromDict(const pybind11::dict& d);

/// 从 Python dict 转换为 DANodeStyle
DAPYWORKFLOW_API DANodeStyle nodeStyleFromDict(const pybind11::dict& d);

/// 从字符串转换为 RenderTemplate 枚举
DAPYWORKFLOW_API RenderTemplate renderTemplateFromString(const QString& s);

/// 从 Python 对象读取颜色（支持 hex string 和 RGB tuple）
DAPYWORKFLOW_API QColor colorFromPyObj(const pybind11::object& obj);

}  // namespace DictConverter

}  // namespace DA
#endif  // DAPYDICTCONVERTER_H
