#ifndef DAPYWORKFLOWPYTHONBINDING_H
#define DAPYWORKFLOWPYTHONBINDING_H
#include <memory>
// pybind11
#include "DAPybind11InQt.h"
// DAPyWorkflow
#include "DAPyNodeMetaData.h"
#include "DAPyNodeState.h"
#include "DAPyNodeStyle.h"

namespace DA
{
namespace PY
{
// 从pybind11::object转换为DAPyNodeMetaData,要求obj必须拥有qualified_name、name、category 属性
DAPyNodeMetaData toNodeMetaData(const pybind11::object& obj);

// 获取节点的状态
DAPyNodeState getNodeState(const pybind11::object& obj);

// 从 Python NodeDisplay 对象属性读取样式，转换为 DAPyNodeStyle
// obj 为 _node_display 对象，直接从其属性（body_shape、background_color 等）读取
DAPyNodeStyle toNodeStyle(const pybind11::object& obj);
}
}  // namespace DA

#endif  // DAPYWORKFLOWPYTHONBINDING_H
