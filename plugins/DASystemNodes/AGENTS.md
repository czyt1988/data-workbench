# DASystemNodes 插件开发指南

DAWorkbench 系统级工作流节点插件，提供流程控制（Start/End/If/Else/Delay）、数据展示（TextViewer）、数据发布（DataToManager）等通用节点。本插件是 **Python 工作流节点开发的参考实现**——所有节点使用 `@NodeDef` 装饰器定义，无 C++ 代码，是新增 Python 节点插件的最佳模板。

> 📖 阅读 `src/DAPyWorkFlow/AGENTS.md` 了解 C++ 渲染代理层架构；本文件聚焦 Python 插件节点开发。

---

## 一、包结构

```
DASystemNodes/
├── CMakeLists.txt              # CMake 安装规则（复制 PyScripts 到 pyplugins/）
├── DASystemNodesPlugin.cpp/h   # C++ 插件入口（仅注册 Python 路径，不定义节点）
└── PyScripts/
    └── DASystemNodes/          # Python 包（pip-installable）
        ├── __init__.py         # 导出所有节点类
        ├── setup.py            # entry_points 声明（data_workbench.plugin）
        ├── utils.py            # 通用工具函数（如 data_to_text）
        └── nodes/
            ├── start.py        # 工作流起点
            ├── end.py          # 工作流终点
            ├── condition_if.py # 条件分支（菱形）
            ├── delay.py        # 延迟
            ├── constant.py     # 常量输出
            ├── data_to_manager.py  # 发布到 DataManager
            └── text_viewer.py  # 文本显示（含 paint 自定义绘制）
```

### 安装路径

| 项 | 路径 | 说明 |
|----|------|------|
| 源码 | `plugins/DASystemNodes/PyScripts/DASystemNodes/` | 开发时编辑此处 |
| 运行时 | `bin/<Config>/pyplugins/DASystemNodes/` | CMake 复制到此，程序从此加载 |

**修改 Python 文件后必须同步到 build 目录**（或重新构建），否则改动不生效：

```powershell
# 同步单个文件
copy /Y "plugins\DASystemNodes\PyScripts\DASystemNodes\nodes\text_viewer.py" `
      "build\Desktop_Qt_6_7_3_MSVC2019_64bit-Debug\bin\pyplugins\DASystemNodes\nodes\text_viewer.py"
```

> ⚠️ Python 模块在程序启动时导入，修改后需**重启程序**才能生效（不支持热重载）。

---

## 二、节点定义规范

### 国际化（i18n）要求

> ⚠️ 节点包必须实现 i18n，详见 [docs/zh/dev-guide/python-i18n.md](../../docs/zh/dev-guide/python-i18n.md#节点包-nodedef-i18n) 的"节点包 i18n"章节

| 字段 | 是否翻译 | 写法 |
|------|---------|------|
| `@NodeDef(name=...)` | ❌ **不翻译** | 保持英文，`name` 参与 `qualified_name` 序列化 |
| `@NodeDef(category=...)` | ✅ 翻译 | `category=_("English Category"))  # cn:中文分类` |
| `Parameter(description=...)` | ✅ 翻译 | `description=_("English desc"))  # cn:中文描述` |
| `Input/Output(description=...)` | ✅ 翻译 | 同上 |
| 类 docstring | 改为英文 | docstring 作为 tooltip 显示 |
| `paint()` 中硬编码文本 | ✅ 翻译 | `painter.drawText(..., _("English"))  # cn:中文` |
| `execute()` 中日志 | ❌ 不翻译 | 保持英文 |

**setup_i18n() 调用时机**：必须在 `__init__.py` 顶部、节点模块导入之前调用，否则 `_()` 未定义会导致节点注册失败。

### 最小节点模板

```python
# -*- coding: utf-8 -*-
"""My Node brief description."""  # docstring 改英文（作为 tooltip）

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(
    name="My Node",            # 显示名称（必填，保持英文不翻译）
    category=_("System / Xxx"),  # cn:系统 / Xxx  分类路径，用 " / " 分层，翻译
    icon="",                   # 图标路径（可空）
)
class MyNode:
    """My Node brief description."""

    # 参数声明（类属性，@NodeDef 会收集为 parameters dict）
    threshold = Parameter(
        float,
        default=0.5,
        min=0.0,
        max=1.0,
        description=_("Threshold"),  # cn:阈值
    )

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        result = Output("DataFrame", description=_("Processed data"))  # cn:处理后的数据

    def __init__(self):
        super().__init__()  # ⚠️ 必须调用，详见 § 四.1
        self._cache = None  # 用户自定义实例属性

    def execute(self, inputs=None, params=None):
        # ⚠️ 签名必须是 (self, inputs=None, params=None)，详见 § 四.3
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("data")
        threshold = params.get("threshold", 0.5)

        result = df[df["value"] > threshold]
        self._output_data["result"] = result  # ⚠️ 通过 _output_data 写输出
        self._cache = result  # 缓存供 paint() 使用

        return True
```

### `@NodeDef` 装饰器机制

`@NodeDef` 会通过 `type(name, (DAWorkflowNode, cls), {})` 创建**新类**，MRO 为：

```
新类 → DAWorkflowNode（框架基类）→ 用户类 → object
```

这意味着：

- 用户类**自动继承** `DAWorkflowNode` 的 `run()`、`qualified_name`、`_input_data`、`_output_data` 等
- 用户类的 `__init__` 通过 `super().__init__()` 链式调用 `DAWorkflowNode.__init__`（详见 § 四.1）
- 用户类**不需要**显式继承 `DAWorkflowNode`，装饰器会处理

### Parameter 声明

```python
# 基本类型
column = Parameter(str, default="value", description=_("Column name"))  # cn:列名
count = Parameter(int, default=10, min=0, max=100)
ratio = Parameter(float, default=0.5, min=0.0, max=1.0, step=0.1, decimals=2)
enabled = Parameter(bool, default=True)

# "code" 类型（Python 字面量表达式，用 ast.literal_eval 求值）
value = Parameter("code", default="1", description=_("Constant value, supports Python literal: 1, 'hello', [1,2,3]"))  # cn:常量值，支持 Python 字面量表达式

# 枚举类型（choices 生成下拉框）
mode = Parameter(str, default="fast", choices=["fast", "slow"], description=_("Mode"))  # cn:模式

# 布局控制：layout="below" 让编辑器占据属性名下方整行宽度
# - str 类型在 below 模式下自动切换为多行 QPlainTextEdit
# - height 可选，控制编辑器高度（像素）；str 默认 80，code 默认 100
expression = Parameter(str, default="", description=_("Pandas eval expression"), layout="below")  # cn:pandas eval 表达式
long_code = Parameter("code", default="", layout="below", height=150)
```

**layout 取值**：

- `"inline"`（默认）：属性名在左，编辑器在右同一行
- `"below"`：属性名在顶部，编辑器占满下方整行；适合长文本/表达式输入

**height 属性**：仅在 below 模式生效，未设置时使用类型默认值（str 80px / code 100px）。

**Parameter 实例是类级别的描述符**，不要在 `execute()` 中通过 `self.X.default` 访问——反序列化后 `self.X` 会被替换为实际值（普通 str/int 等），`.default` 会抛 `AttributeError`。用 `params.get("X", default_value)` 替代（详见 § 四.2）。

### Input / Output 声明

```python
class Inputs:
    # type_name 是字符串约定，非强制类型检查："DataFrame" / "any" / "int" / "bool" / "str"
    data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据
    optional_flag = Input("bool", required=False, description=_("Optional flag"))  # cn:可选标志

class Outputs:
    result = Output("DataFrame", description=_("Output data"))  # cn:输出数据
    # 无输出的节点可以不定义 Outputs（如 TextViewerNode）
```

---

## 三、`execute()` 方法

### 签名规范

```python
def execute(self, inputs=None, params=None):
    # inputs: dict[str, Any]，键为 Input 名称
    # params: dict[str, Any]，键为 Parameter 名称，值为反序列化后的实际值
    ...
    return True  # 返回 True 表示成功，False 或异常表示失败
```

- `inputs` 可能是 `None`（无输入端口的节点），需防御 `if inputs is None: inputs = {}`
- `params` 可能是 `None`，同上
- 返回值：`True` 成功，`False` 失败；抛异常会被框架捕获并标记节点为 error 状态

### 输入读取

```python
# ✅ 正确：通过 inputs.get() 读取
df = inputs.get("data")
if df is None:
    return False  # 必填输入缺失

# ❌ 错误：直接索引（KeyError 会让节点失败但无有用错误信息）
df = inputs["data"]
```

### 参数读取

```python
# ✅ 正确：通过 params.get() 读取，default 与 Parameter 声明一致
threshold = params.get("threshold", 0.5)
mode = (params.get("mode", "fast") or "fast").strip().lower()

# ❌ 错误：访问 Parameter 的 .default 属性（反序列化后会崩溃）
threshold = params.get("threshold", self.threshold.default)
```

### 输出写入

```python
# ✅ 正确：通过 self._output_data dict 写入
self._output_data["result"] = processed_df

# ❌ 错误：直接 return 结果（框架不会自动收集返回值）
return processed_df
```

`_output_data` 由 `DAWorkflowNode.__init__()` 初始化为 `{}`，因此**必须**调用 `super().__init__()`（详见 § 四.1）。

### 调用 C++ API（仅特定节点）

部分节点需要调用 C++ 导出的 API（如 `DataToManagerNode` 发布数据到 DataManager）：

```python
try:
    import da_app
    import da_data
except ImportError:
    return False  # C++ 模块不可用（如纯 Python 测试环境）

core = da_app.getCore()
data_mgr = core.getDataManagerInterface()
# ...
```

- `import da_app` / `import da_data` 等是 C++ 通过 pybind11 导出的模块，仅在 DAWorkbench 运行时可用
- **必须** try/except ImportError，保证节点在纯 Python 环境下也能被导入（否则会破坏节点注册）
- 在非主线程调用 C++ Qt API 时，必须通过 `core.getPythonSignalHandler().callInMainThread(fn)` 切回主线程

---

## 四、⚠️ 核心陷阱（必读）

### 陷阱 1：`__init__` 必须调用 `super().__init__()`

**症状**：`AttributeError: 'MyNode' object has no attribute '_xxx'`，特别是在 `paint()` 中访问实例属性时。

**根本原因**：`@NodeDef` 装饰器创建的类 MRO 为 `新类 → DAWorkflowNode → 用户类 → object`。Python 查找 `__init__` 时找到 `DAWorkflowNode.__init__`（MRO 中第一个有 `__init__` 的类）就停止了——如果 `DAWorkflowNode.__init__` 不调用 `super().__init__()`，用户类的 `__init__` **永远不会执行**，其中设置的实例属性（如 `self._cache = ...`）不存在。

**正确写法**：

```python
def __init__(self):
    super().__init__()  # ✅ 必须！激活 MRO 链，调用 DAWorkflowNode.__init__
    self._cache = None  # 然后设置用户自定义属性
```

**错误写法**：

```python
def __init__(self):
    # ❌ 不调 super().__init__()，DAWorkflowNode.__init__ 不会执行，
    #    _input_data / _output_data / 参数默认值都不会被初始化
    self._cache = None
```

> 历史背景：曾存在"DAWorkflowNode 基类可能没有显式 **init**，不强制 super().**init**()"的误导性注释，导致节点报实例属性不存在的错误。已修复，所有节点**必须**调用 `super().__init__()`。

### 陷阱 2：不要通过 `self.X.default` 访问 Parameter 默认值

**症状**：`AttributeError: 'str' object has no attribute 'default'`（仅在加载已保存的工程后执行时出现）。

**根本原因**：序列化时 `DAWorkflowSerializer` 调用 `setattr(node_instance, "file_path", "/path")`，把 Parameter 描述符**覆盖**为普通字符串。下次访问 `self.file_path.default` 就会崩溃。

**正确写法**：

```python
# ✅ 直接使用字面量默认值（与 Parameter 声明中的 default 一致）
file_path = params.get("file_path", "")
file_type = (params.get("file_type", "csv") or "csv").strip().lower()
```

**错误写法**：

```python
# ❌ 通过 Parameter 实例的 .default 属性获取默认值
file_path = params.get("file_path", self.file_path.default)
```

### 陷阱 3：`execute()` 签名必须一致

**正确签名**：

```python
def execute(self, inputs=None, params=None):
    ...
```

- `inputs` 和 `params` **都要有默认值** `None`，因为框架内部可能在某些路径下传 `None`
- 参数顺序固定：`inputs` 在前，`params` 在后

框架的 `run()` 方法通过 `inspect.signature()` 检测 arity，兼容老式 `execute(self)` 签名，但**新节点必须用双参数签名**。

### 陷阱 4：`paint()` 方法必须防御性访问实例属性

**症状**：节点拖入场景后疯狂打印 `DAPyNodeGraphicsItem paint_callback error: AttributeError: ...`，节点显示空白或异常。

**根本原因**：`paint()` 由 Qt 渲染线程在每次重绘时调用，可能在 `execute()` 之前（节点刚拖入还未运行）或 `execute()` 异常中断后调用。此时 `self._cache` 等实例属性可能尚未设置（虽然 `__init__` 应该已设置，但如果 `__init__` 异常或未调用 `super().__init__()` 就会缺失）。

**正确写法**（参考 `text_viewer.py`）：

```python
def paint(self, painter, body_rect):
    x, y, w, h = body_rect

    # ✅ 用 getattr 防御性访问，避免属性未初始化时崩溃
    lines = getattr(self, "_display_lines", [])
    if not lines:
        lines = ["<no data>"]

    font_size = getattr(self, "font_size", 9)  # Parameter 也可能未设置

    # 绘制逻辑...
```

**错误写法**（参考修复前的节点代码）：

```python
def paint(self, painter, body_rect):
    # ❌ 直接访问实例属性，__init__ 未执行时会崩溃
    display = self._last_text
    if self._last_prefix:
        display = f"{self._last_prefix}{display}"
```

### 陷阱 5：`paint()` 是 C++ 自动注册的回调

`paint()` 方法**不需要**手动注册——`DAPyNodeGraphicsItem::setProxy()` 会检测 Python 节点对象是否有 `paint` 属性，有则自动注册为绘制回调。

`paint()` 签名固定为：

```python
def paint(self, painter, body_rect):
    """
    Args:
        painter: da_py_workflow.DAPyPainterProxy 实例，封装了 QPainter 的基本绘制方法
        body_rect: tuple (x, y, w, h) 节点主体矩形（item 局部坐标）
    """
```

`DAPyPainterProxy` 暴露的方法（参考 `text_viewer.py`）：

| 方法 | 说明 |
|------|------|
| `setNoPen()` / `setNoBrush()` | 设置无画笔/无画刷 |
| `setPenColor(r, g, b, a=255)` | 设置画笔颜色 |
| `setPenWidth(w)` | 设置画笔宽度 |
| `setFont(family, size)` | 设置字体 |
| `drawText(x, y, text)` | 绘制文本 |
| `drawRect(x, y, w, h)` | 绘制矩形 |
| `drawLine(x1, y1, x2, y2)` | 绘制直线 |
| `fillRect(x, y, w, h, r, g, b, a)` | 填充矩形 |
| `textBoundingRect(text, family, size, bold, italic)` | 返回 (width, height) 文本尺寸 |
| `setClipRect(x, y, w, h)` / `clearClip()` | 裁剪区域 |

`paint()` 执行后，节点状态变更（`execute()` 完成）会自动触发 `item->update()` 重绘，无需手动通知。

### 陷阱 6：`_output_data` 访问方式

```python
# ✅ 正确：直接通过 self._output_data dict 写入
self._output_data["result"] = value

# ❌ 错误：尝试通过方法返回值输出（框架不会收集 return 的值）
return {"result": value}
```

`_output_data` 由 `DAWorkflowNode.__init__()` 初始化为 `{}`，因此依赖 `super().__init__()`（陷阱 1）。如果未调用，`_output_data` 不存在，写入会抛 `AttributeError`。

---

## 五、样式定制（NodeDisplay）

通过 `@NodeDef` 的 `style` 参数配置节点外观：

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, NodeDisplay, LinkPointStyle

@NodeDef(
    name="If / Else",
    category="System / Flow Control",
    style=NodeDisplay(
        body_shape="Diamond",          # 形状：Rect（默认）/ Diamond / Ellipse / RoundedRect
        background_color="#E3F2FD",    # 背景色（hex 字符串或 RGB 元组）
        border_color="#2196F3",        # 边框色
        corner_radius=8.0,             # 圆角半径（RoundedRect 时生效）
        name_position="Below",         # 名称位置：Above / Below / Inside / Hidden
        input_port_style=LinkPointStyle(shape="Diamond"),   # 输入端口形状
        output_port_style=LinkPointStyle(shape="Diamond"),  # 输出端口形状
    ),
)
class IfElseNode:
    ...
```

**如果定义了 `paint()` 方法，`paint()` 会完全接管节点主体绘制**，`NodeDisplay` 的 `background_color` / `border_color` 等仅在 `paint()` 中手动使用（作为参考值），不会自动应用。`paint()` 中通常手动绘制背景和边框，参考 `text_viewer.py`。

⚠️ **`_` 变量名陷阱**：`paint()` 中解构赋值时**禁止用 `_` 作为变量名**（如 `_, h = painter.textBoundingRect(...)`），因为 Python 会将 `_` 视为局部变量，导致后续 `_("...")` 调用 `gettext._()` 时报 `UnboundLocalError`。必须用 `_w`、`_h` 等替代名称。

---

## 六、构建与测试

### 构建

DASystemNodes 是纯 Python 插件，CMake 只负责复制文件到 build 目录：

```powershell
.\scripts\build.ps1 -Target DASystemNodes
```

或手动复制（开发时快速迭代）：

```powershell
# 复制整个包
xcopy /E /Y /I "plugins\DASystemNodes\PyScripts\DASystemNodes" `
              "build\Desktop_Qt_6_7_3_MSVC2019_64bit-Debug\bin\pyplugins\DASystemNodes\"
```

### 测试

目前本插件无独立单元测试。验证节点行为的方式：

1. **启动程序** → 拖入节点 → 检查是否报错（看日志面板）
2. **构建工作流** → 执行 → 检查输出（TextViewer 看节点画面（开启 log_to_console 时同时看日志），DataToManager 看 DataManager 面板）
3. **保存/加载工程** → 验证参数和连接正确恢复

### 调试技巧

- Python 异常会被 C++ 代理层吞掉（返回安全默认值），**看日志面板**才能看到完整错误
- `paint()` 异常会疯狂刷屏（每帧重绘都报错），出现时立即检查 `__init__` 是否调用了 `super().__init__()`
- 怀疑节点未注册？在 Python 控制台执行：

  ```python
  from DAWorkbench.DAWorkFlowPy import DANodeRegistry
  print(DANodeRegistry.list_nodes())
  ```

---

## 七、反模式汇总

| 反模式 | 正确做法 | 参考 |
|--------|---------|------|
| `__init__` 不调 `super().__init__()` | 必须调用，激活 MRO 链 | § 四.1 |
| `params.get("x", self.x.default)` | `params.get("x", literal_default)` | § 四.2 |
| `execute(self)` 单参数签名 | `execute(self, inputs=None, params=None)` | § 四.3 |
| `paint()` 直接访问 `self._cache` | `getattr(self, "_cache", default)` | § 四.4 |
| 通过 `return value` 输出数据 | `self._output_data["key"] = value` | § 四.6 |
| 显式继承 `DAWorkflowNode` | 不需要，`@NodeDef` 会处理 | § 二 |
| 在 `execute()` 中读取 `_node_display` | `_node_display` 仅 C++ 渲染层使用 | `src/DAPyWorkFlow/AGENTS.md` P4 |
| 节点类中 `import da_app`（顶层） | 在 `execute()` 内 try/except ImportError | § 三 |
| `description="中文"` 直接写中文 | `description=_("English"))  # cn:中文` | § 二 i18n |
| `@NodeDef(name=_("..."))` 翻译 name | name 保持英文不翻译（参与序列化） | § 二 i18n |
| `__init__.py` 中节点导入前未调 `setup_i18n()` | 顶部先调 `setup_i18n()` 再导入节点 | § 二 i18n |
| `paint()` 中硬编码显示文本 | 用 `_()` 包裹显示文本 | § 二 i18n |

---

## 八、参考节点

| 节点 | 文件 | 学习点 |
|------|------|--------|
| **StartNode** | `nodes/start.py` | 最小节点定义（无参数、无 paint） |
| **ConstantNode** | `nodes/constant.py` | Parameter "code" 类型 + `ast.literal_eval` 安全求值 |
| **IfElseNode** | `nodes/condition_if.py` | NodeDisplay 菱形样式 + LinkPointStyle 端口形状 + 多输出分支 |
| **DelayNode** | `nodes/delay.py` | Parameter float 类型（min/max/step/decimals）+ time.sleep 阻塞执行 |
| **DataToManagerNode** | `nodes/data_to_manager.py` | 调用 C++ API（da_app/da_data）+ 主线程切换 |
| **TextViewerNode** | `nodes/text_viewer.py` | paint() 自定义绘制 + 缓存实例属性 + 防御性 getattr + clipRect 裁剪 + 工具函数复用 + **运行时状态持久化** + 可选日志输出（log_to_console） |

新增节点时，**先找到最相似的参考节点**复制结构，再修改业务逻辑。

---

## 九、运行时状态持久化

### 问题背景

`execute()` 阶段缓存的实例属性（如 `_display_text`、`_last_text`）默认在工程保存/加载时丢失。重新打开工程后，节点画面会变回空白，必须再次执行才能看到内容——这与用户直觉不符。

### 解决方案：serialize_runtime_state / deserialize_runtime_state

`DAWorkflowNode` 基类提供一对钩子，让节点把任意 JSON 可序列化的运行时状态写入工程文件并在加载时恢复：

```python
def __init__(self):
    super().__init__()
    self._display_text = ""

def execute(self, inputs=None, params=None):
    value = (inputs or {}).get("value")
    self._display_text = str(value) if value is not None else ""
    return True

def serialize_runtime_state(self) -> dict:
    """保存时调用，返回需要持久化的运行时状态。"""
    return {"display_text": getattr(self, "_display_text", "")}

def deserialize_runtime_state(self, state: dict) -> None:
    """加载时调用，从 state 恢复运行时状态。"""
    self._display_text = state.get("display_text", "")
```

`DAWorkflowSerializer` 会在 `to_dict` / `to_xml_element` 中调用 `serialize_runtime_state()`，把返回的 dict 写入工程文件；在 `from_dict` / `from_xml_element` 中调用 `deserialize_runtime_state()` 恢复状态。

### DASystemNodes 中的实现

| 节点 | 持久化的状态 | 说明 |
|------|-------------|------|
| `TextViewerNode` | `display_text` | `execute()` 缓存的显示文本，`paint()` 绘制 |

### XML 存储格式

运行时状态存储在 `<node>` 元素下的 `<state>` 子元素中：

```xml
<node node_id="DASystemNodes.TextViewer_1" qualified_name="DASystemNodes.TextViewer">
  <param name="font_color" type="str">#282828</param>
  <param name="font_size" type="int">9</param>
  <state>
    <item name="display_text" type="str">Hello World</item>
  </state>
</node>
```

### 使用约束

1. **仅存 JSON 可序列化类型**：`str/int/float/bool/list/dict/None`。DataFrame、numpy 数组、Python 对象引用等**不可**直接放入返回值。
2. **不替代 Parameter**：用户可配置的参数必须通过 `Parameter` 声明，运行时状态仅用于 `execute()` 的衍生缓存。
3. **防御性读取**：`deserialize_runtime_state()` 收到的 dict 可能为空（旧工程文件无此字段），必须用 `state.get(key, default)` 安全访问。
4. **异常隔离**：序列化器在调用钩子时用 `try/except` 包裹，单个节点的钩子失败不会中断整体保存/加载流程，但会丢失该节点的运行时状态。

### 何时需要实现

| 场景 | 是否实现 |
|------|----------|
| 节点 `execute()` 缓存了供 `paint()` 使用的衍生数据 | ✅ 必须 |
| 节点仅输出数据到下游（`_output_data`），无自定义绘制 | ❌ 不需要 |
| 节点缓存的派生状态可从参数 + 输入完全重建 | ❌ 不需要（重新执行即可） |
| 节点缓存的状态无法轻易重建（如历史日志、交互历史） | ✅ 推荐 |

详见 [项目序列化架构 - 运行时状态持久化](../../docs/zh/dev-guide/project-serialization-architecture.md#8-运行时状态持久化)。

### ⚠️ @NodeDef 的 MRO 遮盖陷阱

`@NodeDef` 装饰器通过 `type(cls.__name__, (DAWorkflowNode, cls), {})` 创建新类，MRO 为：

```
new_cls → DAWorkflowNode → 用户类 cls → object
```

**`DAWorkflowNode` 基类的方法会遮盖用户类的同名方法**。这意味着如果基类定义了 `def foo()`，用户类也定义了 `def foo()`，调用时基类的方法会被优先调用，用户类的覆写被忽略。

`serialize_runtime_state` / `deserialize_runtime_state` 钩子已在基类中通过 `super()` 转发到用户类实现，**用户类正常覆写即可，无需调用 `super()`**：

```python
# TextViewerNode 中的覆写 — 直接返回，不需要 super()
def serialize_runtime_state(self) -> dict:
    return {"display_text": self._display_text}
```

!!! warning "新增 DAWorkflowNode 基类方法"
    如果在 `DAWorkflowNode` 基类中新增需要被子类覆写的方法，**必须**使用 `super()` 转发模式。详见 [Python 节点开发指南 - MRO 遮盖陷阱](../../docs/zh/dev-guide/workflow-python-node-dev.md#nodef-的-mro-遮盖陷阱)。
