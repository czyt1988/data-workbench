# 项目概览

!!! success "项目定位"
    **DAWorkBench** 是一个基于工作流的数据分析平台，致力于为科研人员和数据分析工程师提供一个高效、直观、可扩展的数据处理解决方案。

---

## 项目愿景

在现代数据处理领域，我们面临着三大核心挑战：

1. **重复性数据处理** - 科研实验数据往往需要处理多组数据，每组数据的清洗方式基本一致，手动处理效率低下
2. **复杂的函数调用** - Python 的 pandas、numpy、scipy 数据处理三大利器，函数繁多，需要频繁查阅文档
3. **可视化调参繁琐** - matplotlib/matlab 绘图参数调节复杂，细微调整（如文本位置）需要大量时间

**DAWorkBench 的解决方案：**

| 挑战 | 解决方案 | 核心模块 |
|------|----------|----------|
| 重复性数据处理 | 工作流驱动自动化 | **Workflow** |
| 函数调用复杂 | GUI 封装 pandas 核心功能 | **Data** |
| 可视化调参繁琐 | 交互式数据可视化 | **Chart** |

---

## 核心特性

### 1. 工作流驱动数据处理

- :material-graph: **有向图描述** - 使用有向图作为工作流的数据描述，可视化呈现数据处理流程
- :material-cog-sync: **节点化操作** - 每个处理步骤封装为节点，参数化配置，一次设计多次执行
- :material-play-circle: **自动执行** - 工作流一键执行，自动完成数据清洗、转换、输出等全部流程

``` mermaid
graph LR
    A[数据导入] --> B[数据清洗]
    B --> C[数据转换]
    C --> D[数据分析]
    D --> E[图表生成]
    E --> F[结果导出]
    
    style A fill:#e1f5fe
    style B fill:#fff3e0
    style C fill:#f3e5f5
    style D fill:#e8f5e9
    style E fill:#fce4ec
    style F fill:#fff8e1
```

### 2. Python 数据处理集成

- :material-language-python: **pandas 封装** - GUI 封装 pandas 核心函数，无需编写代码即可操作 DataFrame
- :material-function: **numpy/scipy 支持** - 内置数值计算和科学计算功能
- :material-code-block: **Python 脚本节点** - 支持自定义 Python 脚本节点，灵活扩展

### 3. 论文级数据可视化

- :material-chart-line: **交互式图表** - 拖拽调整元素位置，实时预览效果
- :material-file-image: **高质量输出** - 支持导出论文级别的矢量图（SVG/PDF）
- :material-brush: **精细调参** - 图表元素可视化配置，告别坐标调试

### 4. 插件化架构

- :material-puzzle: **松耦合设计** - 核心功能与业务逻辑分离，易于扩展
- :material-toolbox: **插件仓库** - 内置数据分析插件，支持自定义开发
- :material-api: **开放接口** - 完整的插件开发接口和生命周期管理

---

## 技术栈

| 类别 | 技术 | 版本要求 |
|------|------|----------|
| 语言 | C++ | C++17 |
| GUI 框架 | Qt | 5.14+ / 6.x |
| 数据处理 | Python + pandas | Python 3.8+ |
| 构建系统 | CMake | 3.12+ |
| 绑定框架 | pybind11 | - |

### 核心第三方库

- **SARibbon** - Ribbon 界面框架
- **Qt-Advanced-Docking-System** - 高级 Dock 窗口管理
- **qwt** - 科学图表库
- **spdlog** - 高性能日志库
- **pybind11** - Python/C++ 绑定

---

## 应用场景

### 科研数据分析

```
实验室数据 → 自动清洗 → 统计分析 → 图表生成 → 论文插图
```

- 处理大量实验数据组
- 标准化数据处理流程
- 快速生成可复用的分析模板

### 一维仿真建模

```
参数设置 → 仿真计算 → 结果分析 → 曲线绘制 → 报告生成
```

- 类似 Amesim 的仿真框架
- 模块化组件设计
- 参数化仿真配置

### 工业数据处理

```
传感器数据 → 异常检测 → 数据清洗 → 报表生成 → 自动归档
```

- 处理十万级以上数据点
- 比 Excel 更快的处理速度
- 可定制化的数据处理流程

---

## 快速上手

### 环境准备

!!! tip "前置条件"
    - Qt 5.14+ 或 Qt 6.x 开发环境
    - Python 3.8+ 环境
    - CMake 3.12+
    - Git（用于拉取第三方库）

### 克隆项目

```bash
git clone https://github.com/czyt1988/data-workbench.git
cd data-workbench
git submodule update --init --recursive
```

### 构建步骤

```bash
# 1. 构建 zlib（quazip 依赖）
cmake -S src/3rdparty/zlib -B build/zlib
cmake --build build/zlib --config Release
cmake --install build/zlib --prefix ./bin_Release

# 2. 构建所有第三方库
cmake -S src/3rdparty -B build/3rdparty
cmake --build build/3rdparty --config Release
cmake --install build/3rdparty --prefix ./bin_Release

# 3. 构建主程序
cmake -S . -B build/main
cmake --build build/main --config Release
cmake --install build/main --prefix ./bin_Release
```

### 运行程序

```bash
# Windows
./bin_Release/bin/DAWorkbench.exe

# Linux
./bin_Release/bin/DAWorkbench
```

---

## 项目截图

![动态演示](../assets/screenshot/screenshot1.gif)

![主界面](../assets/screenshot/01.png)

![数据分析界面](../assets/screenshot/02.png)

---

## 下一步

- [:material-book: 项目结构详解](./project-structure.md) - 了解项目目录和模块组织
- [:material-tools: 开发环境搭建](./build/build-instructions.md) - 详细构建指南
- [:material-puzzle: 插件开发指南](./plugin-development.md) - 开始开发自己的插件