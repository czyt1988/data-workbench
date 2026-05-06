"""
测试节点定义模块

提供多种 @NodeDef 装饰的节点类，用于工作流和执行器测试。

子模块：
- sample_nodes: 基础功能测试节点（数据源、筛选、绘图、错误等）
- render_style_nodes: 渲染样式配置测试节点（覆盖 DANodeStyle 所有字段）
"""

# 注意: render_style_nodes 依赖 da_py_workflow（pybind11 嵌入模块），
# 仅在 C++ 嵌入 Python 环境中可导入。独立 pytest 运行时会跳过此模块。
try:
    from . import render_style_nodes
except ImportError:
    pass

from . import sample_nodes