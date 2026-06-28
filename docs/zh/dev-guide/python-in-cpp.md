# Python/C++ 集成

本文档是 DAWorkBench 中 C++ 与 Python 集成相关文档的导航入口。完整内容已拆分为以下 5 个独立章节，便于阅读和维护：

| 文档 | 说明 |
|------|------|
| [总览与环境搭建](./python-binding/index.md) | 架构总览、CMake 配置、目录结构、相关模块与参考资料 |
| [C++ 调用 Python](./python-binding/cpp-calling-python.md) | Python 解释器初始化、GIL 管理、脚本调用示例 |
| [Python 绑定开发](./python-binding/python-binding-development.md) | 接口绑定架构与实现、所有权策略、跨线程通信、Qt 类型转换器、绑定开发实操流程、模块绑定路线图 |
| [pybind11 ↔ Qt 类型转换器](./dapybind11-qt-caster.md) | `DAPybind11QtCaster.hpp` 完整使用指南 — 类型映射表、自动转换机制、`DA::PY` 辅助函数、`safe_pyobject`、numpy/pandas 集成 |
| [故障排除与最佳实践](./python-binding/troubleshooting-and-best-practices.md) | 问题诊断流程、常见错误与调试技巧、设计原则与检查清单 |
| [Python 脚本开发实战](./python-binding/python-script-development.md) | 四种交互模式、标准脚本编写流程、getConfigValues 对话框、撤销/重做、跨线程操作、Thread Status Manager |
| [脚本的国际化](./python-i18n.md) | Python 脚本基于 GNU gettext 的多语言支持：_() 标记、xgettext、msginit、msgfmt 完整流程 |
| [嵌入式 Python 调试](./embedded-python-debugging.md) | stubs 代码提示、Mock 逻辑验证、debugpy 远程调试、日志调试 |

---

## DAWorkbench Python 模块目录结构

`src/PyScripts/DAWorkbench/` 是主程序内置的 Python 模块，已重构为子包结构：

```text
src/PyScripts/DAWorkbench/
├── __init__.py              # 模块入口，sys.modules 别名保持旧路径可用
├── DAPyBase/                # 基础工具子包
│   ├── __init__.py
│   ├── form_spec.py                # FormSpec 数据结构定义
│   ├── form_builder.py             # FormBuilder 链式构建器
│   ├── thread_status_manager.py    # 线程状态管理
│   ├── utils.py                    # 通用工具函数
│   ├── da_logger.py                # 日志工具
│   ├── io.py                       # I/O 操作
│   ├── dataframe.py                # DataFrame 操作
│   ├── data_processing.py          # 数据处理/信号处理
│   └── app_wrapper.py              # 应用包装器
└── DAWorkFlowPy/              # 工作流 Python 核心
    ├── __init__.py
    ├── executor.py            # 执行引擎
    ├── node_def.py            # 节点定义装饰器（@NodeDef）
    ├── signal_manager.py      # 信号管理器
    ├── _debug.py              # 调试工具
    └── nodes/                 # 内置节点子包
        └── __init__.py
```

!!! tip "旧路径兼容"
    `__init__.py` 通过 `sys.modules` 别名机制保持 `DAWorkbench.form_builder` 等旧导入路径可用，但推荐使用新路径 `DAWorkbench.DAPyBase.form_builder`。

---

## DataAnalysis 插件 Python 包目录结构

`plugins/DataAnalysis/PyScripts/` 已从单层 `DADataAnalysis` 包重构为三层架构：

```text
plugins/DataAnalysis/PyScripts/
├── DADataAnalysisCore/      # 纯 pandas 核心算法（无 Qt 依赖）
│   ├── __init__.py
│   ├── cleaning.py          # 数据清洗算法
│   ├── io.py                # 文件读写算法
│   └── operations.py        # DataFrame 操作算法
├── DADataAnalysisGui/       # GUI 交互逻辑（依赖 da_app/da_interface）
│   ├── __init__.py
│   ├── dataframe_cleaner.py # 数据清洗 UI 交互
│   ├── dataframe_io.py      # 数据导入导出 UI 交互
│   ├── utils.py             # 工具函数
│   └── i18n/                # 国际化翻译
├── DADataAnalysisNodes/     # 工作流节点插件（自动发现）
│   ├── __init__.py
│   ├── *_node.py            # 20+ 节点定义文件
│   └── setup.py             # entry_points 注册
└── DADataAnalysis/          # 兼容旧包（可选，逐步废弃）
```
