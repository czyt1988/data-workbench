#ifndef DAPYWORKFLOWPYTHONBINDING_H
#define DAPYWORKFLOWPYTHONBINDING_H
/**
@file DAPyWorkFlowPythonBinding
@brief Python 绑定模块 da_py_workflow 的完整 API 清单与调用示例
@author ChenZongyan
@date   2026-04-20

@section py_overview  一、模块功能
da_py_workflow 模块把 C++ 的 Python 工作流引擎桥接层暴露给 Python，使脚本能够：
- 创建和管理 DAPyWorkFlowScene 场景（基于 DAGraphicsView 的 Python 节点渲染）；
- 获取 DAPyNodeProxy 代理节点实例，执行 Python 定义的节点；
- 查询和设置节点状态（DAPyNodeState 枚举）；
- 通过 DAPythonSignalHandler 实现 Python→C++ 的状态更新通知（复用 da_interface 模块已有机制）。

@section py_class_list  二、导出类型一览

| Python 类 / 枚举        | 对应 C++ 类型                  | 说明 |
|-------------------------|--------------------------------|------|
| DAPyWorkFlowScene       | DA::DAPyWorkFlowScene          | Python 工作流场景管理，继承 DAGraphicsScene |
| DAPyNodeState           | DA::DAPyNodeState              | 节点状态枚举（Idle/Waiting/Running/Success/Error/Skipped） |
| DAPyNodeProxy           | DA::DAPyNodeProxy              | Python 节点的 C++ 代理（通过 DAPyModuleWorkflow 获取） |
| DAPyPainterProxy        | DA::DAPyPainterProxy           | QPainter 代理，暴露基本绘制操作给 Python 回调 |

@section py_ctor  三、构造与获取

@subsection py_ctor_scene  1. DAPyWorkFlowScene 构造
- DAPyWorkFlowScene(parent:QObject = None)
创建新的 Python 工作流场景。场景持有 DAGraphicsScene 的渲染能力，用于显示 Python 定义的节点。

@subsection py_get_proxy  2. DAPyNodeProxy 获取
@code
proxy = da_py_workflow.getNodeProxy(qualified_name:str) -> DAPyNodeProxy
@endcode
通过 Python 节点的 qualified_name（模块.类名）获取对应的 C++ 代理节点。代理节点用于执行 Python 节点逻辑并管理状态。

@section py_member_scene  四、DAPyWorkFlowScene 成员

@subsection py_scene_node  1. 节点管理
- addNode(qualified_name:str, pos:tuple) -> DAPyNodeProxy
在指定位置 (x, y) 添加 Python 节点，返回代理节点引用。
- removeNode(proxy:DAPyNodeProxy) -> bool
移除节点，成功返回 True。
- getNodeAt(pos:tuple) -> DAPyNodeProxy
获取指定位置的节点，无节点返回 None。
- getAllNodes() -> list[DAPyNodeProxy]
获取场景中所有 Python 节点代理。

@subsection py_scene_connection  2. 连接管理
- addConnection(src_proxy:DAPyNodeProxy, src_output:str, dst_proxy:DAPyNodeProxy, dst_input:str) -> bool
添加节点连接（源节点输出 → 目标节点输入）。
- removeConnection(src_proxy:DAPyNodeProxy, src_output:str, dst_proxy:DAPyNodeProxy, dst_input:str) -> bool
移除指定连接。
- getConnections() -> list[tuple]
获取所有连接的列表，每个元素为 (src_proxy, src_output, dst_proxy, dst_input)。

@subsection py_scene_exec  3. 执行控制
- executeAll() -> bool
执行场景中所有节点（按拓扑顺序）。
- clearScene() -> None
清空场景，移除所有节点和连接。

@section py_member_proxy  五、DAPyNodeProxy 成员

@subsection py_proxy_state  1. 状态管理
- getState() -> DAPyNodeState
获取当前节点状态。
- setState(state:DAPyNodeState) -> None
设置节点状态（通常由 Python 执行器通过 DAPythonSignalHandler 回调设置）。
- getErrorString() -> str
获取错误信息字符串（当状态为 Error 时）。

@subsection py_proxy_exec  2. 执行
- execute() -> bool
执行节点逻辑。内部获取 GIL，调用 Python 节点的 execute() 方法，处理异常，返回执行结果。

@subsection py_proxy_data  3. 数据传递
- setInputData(key:str, data:object) -> None
设置输入数据（Python 对象引用传递）。
- getOutputData(key:str) -> object
获取输出数据（Python 对象引用）。

@subsection py_proxy_info  4. 信息查询
- getQualifiedName() -> str
获取 Python 节点的 qualified_name（模块.类名）。
- getDescriptor() -> dict
获取节点描述符字典（包含 inputs/outputs/parameters 等元数据）。

@section py_enum  六、DAPyNodeState 枚举
| 成员名 | 含义 |
|--------|------|
| Idle      | 默认空闲状态 |
| Waiting   | 等待上游数据 |
| Running   | 正在执行 |
| Success   | 执行成功 |
| Error     | 执行失败 |
| Skipped   | 跳过（上游失败导致无输入数据） |

@section py_painter  七、DAPyPainterProxy 成员

DAPyPainterProxy 是 QPainter 的 Python 代理类，仅在 paint_callback 调用期间有效。
不应在回调外保存引用。

@subsection py_painter_draw  1. 绘制操作
- drawRect(x:float, y:float, w:float, h:float) -> None
  绘制矩形轮廓。
- drawText(x:float, y:float, text:str) -> None
  在指定位置绘制文本。
- drawLine(x1:float, y1:float, x2:float, y2:float) -> None
  绘制直线。
- drawEllipse(x:float, y:float, w:float, h:float) -> None
  绘制椭圆。
- fillRect(x:float, y:float, w:float, h:float, r:int, g:int, b:int, a:int=255) -> None
  用指定RGBA颜色填充矩形。

@subsection py_painter_style  2. 样式设置
- setPenColor(r:int, g:int, b:int, a:int=255) -> None
  设置画笔颜色（RGBA）。
- setPenWidth(width:float) -> None
  设置画笔宽度。
- setBrushColor(r:int, g:int, b:int, a:int=255) -> None
  设置画刷颜色（RGBA）。
- setFont(family:str, size:float) -> None
  设置字体族和字号。
- setNoPen() -> None
  设置无画笔（不绘制边框线）。
- setNoBrush() -> None
  设置无画刷（不填充内部区域）。

@subsection py_painter_paint_callback  3. paint_callback 用法

Python 节点可通过 DANodeDescriptor 的 paint_callback 字段提供自定义绘制：
@code{.py}
# Python 节点定义自定义绘制回调
def paint(self, painter_proxy, body_rect):
    # body_rect 是 (x, y, w, h) 元组
    painter_proxy.fillRect(body_rect[0], body_rect[1], body_rect[2], body_rect[3], 240, 240, 240)
    painter_proxy.setPenColor(0, 0, 0)
    painter_proxy.drawText(body_rect[0] + 5, body_rect[1] + 5, "Custom Node")
@endcode

回调应在50ms内完成绘制，否则可能阻塞GUI线程。
如果回调抛出异常，节点会回退到默认矩形模板渲染，并显示红色"Error"标记。

@section py_example  八、完整示例

@code{.py}
import da_py_workflow
import da_interface

# 1. 创建场景
scene = da_py_workflow.DAPyWorkFlowScene()

# 2. 添加 Python 节点（假设已通过 NodeDef 定义 DataFilter 类）
proxy = scene.addNode("my_package.nodes.DataFilter", (100, 100))

# 3. 设置节点状态（通常由执行器自动设置）
proxy.setState(da_py_workflow.DAPyNodeState.Running)

# 4. 执行节点
success = proxy.execute()
if success:
    print("Node executed successfully")
    proxy.setState(da_py_workflow.DAPyNodeState.Success)
else:
    print("Node failed:", proxy.getErrorString())
    proxy.setState(da_py_workflow.DAPyNodeState.Error)

# 5. Python→C++ 状态更新通知（复用 da_interface 的 DAPythonSignalHandler）
core = da_app.getCore()
handler = core.getPythonSignalHandler()

def update_node_state():
    proxy.setState(da_py_workflow.DAPyNodeState.Success)

handler.callInMainThread(update_node_state)  # 安全回到 Qt 主线程更新状态
@endcode

@section py_thread  九、线程与生命周期
- 所有函数默认持有 GIL，Qt GUI 线程直接调用安全。
- DAPyNodeProxy::execute() 内部使用 DAPyGILGuard 管理 GIL，支持后台线程调用。
- Python→C++ 状态更新必须通过 DAPythonSignalHandler::callInMainThread 回到 Qt 主线程（复用 da_interface 模块已有绑定）。
- DAPyNodeProxy 持有 Python 节点实例的引用，Python 对象被 Python 垃圾回收时 C++ 侧引用自动失效（通过 safe_pyobject 机制）。

@section py_limit  十、当前限制
- DAPyWorkFlowScene 目前仅支持矩形节点渲染模板（'rect'），SVG 和 Widget 模板待实现。
- 节点连接验证目前仅检查连接点名称存在性，不验证数据类型兼容性。
- 自定义 paint() 回调目前仅支持基本绘制操作（drawRect, drawText, drawLine, fillRect）。
*/

// pybind11
#include "DAPybind11InQt.h"

// DA
#include "DAPyWorkFlow/DAPyWorkFlowScene.h"
#include "DAPyWorkFlow/DAPyNodeProxy.h"
#include "DAPyWorkFlow/DAPyNodeState.h"
#include "DAPyWorkFlow/DAPyPainterProxy.h"

namespace DA {

/**
 * @brief 通过 qualified_name 获取 DAPyNodeProxy 实例
 * @param qualified_name Python 节点的完整限定名（模块.类名）
 * @return 代理节点引用，如果节点未注册返回空代理
 */
DAPyNodeProxy getNodeProxy(const std::string& qualified_name);

}  // namespace DA

#endif  // DAPYWORKFLOWPYTHONBINDING_H