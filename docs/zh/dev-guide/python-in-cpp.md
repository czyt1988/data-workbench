# Python/C++ 集成

本文档是 DAWorkBench 中 C++ 与 Python 集成相关文档的导航入口。完整内容已拆分为以下 5 个独立章节，便于阅读和维护：

| 文档 | 说明 |
|------|------|
| [总览与环境搭建](./python-binding/index.md) | 架构总览、CMake 配置、目录结构、相关模块与参考资料 |
| [C++ 调用 Python](./python-binding/cpp-calling-python.md) | Python 解释器初始化、GIL 管理、脚本调用示例 |
| [Python 绑定开发](./python-binding/python-binding-development.md) | 接口绑定架构与实现、所有权策略、跨线程通信、Qt 类型转换器、绑定开发实操流程、模块绑定路线图 |
| [故障排除与最佳实践](./python-binding/troubleshooting-and-best-practices.md) | 问题诊断流程、常见错误与调试技巧、设计原则与检查清单 |
| [Python 脚本开发实战](./python-binding/python-script-development.md) | 四种交互模式、标准脚本编写流程、getConfigValues 对话框、撤销/重做、跨线程操作、Thread Status Manager |