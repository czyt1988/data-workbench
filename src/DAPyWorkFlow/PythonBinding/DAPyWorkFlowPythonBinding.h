#ifndef DAPYWORKFLOWPYTHONBINDING_H
#define DAPYWORKFLOWPYTHONBINDING_H
#include <memory>
// pybind11
#include "DAPybind11InQt.h"
// DAPyWorkflow
#include "DAPyNodeMetaData.h"
#include "DAPyNodeState.h"

namespace DA
{
namespace PY
{
// 从pybind11::object转换为DAPyNodeMetaData,要求obj必须拥有qualified_name、name、category 属性
DAPyNodeMetaData toNodeMetaData(const pybind11::object& obj);

// 获取节点的状态
DAPyNodeState getNodeState(const pybind11::object& obj);
}
}  // namespace DA

#endif  // DAPYWORKFLOWPYTHONBINDING_H
