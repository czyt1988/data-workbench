# 命令行参数

DAWorkBench 支持命令行参数，通过命令行可以控制程序启动行为，实现自动化数据导入和工程打开等功能。

## 主要功能特性

**特性**

- ✅ **工程文件打开**：直接通过命令行参数打开指定的 `.dapro` 工程文件
- ✅ **数据文件导入**：启动时自动导入 CSV、Excel 等格式数据文件
- ✅ **版本信息显示**：快速查看程序版本号
- ✅ **帮助信息显示**：查看所有可用命令行参数

## 参数一览

程序支持的命令行参数如下：

| 参数 | 简写 | 说明 |
|------|------|------|
| `--help` | `-h` | 显示帮助信息并退出 |
| `--version` | `-v` | 显示程序版本号并退出 |
| `[project]` | 无 | 打开指定的工程文件（位置参数） |
| `--import-data <path>` | 无 | 导入数据文件，支持多次使用 |

## 使用方法

### 打开工程文件

直接在命令行指定 `.dapro` 工程文件路径，程序启动后自动加载该工程：

```shell
# 打开指定工程文件
DAWorkBench.exe "D:\projects\my-project.dapro"
```

!!! tip "路径引号"
    如果路径包含空格，需要使用双引号包裹路径。

### 导入数据文件

使用 `--import-data` 参数在启动时导入数据文件，支持的数据格式包括：

| 格式 | 扩展名 | 说明 |
|------|--------|------|
| CSV | `.csv` | 逗号分隔值文件 |
| Excel | `.xlsx` / `.xls` | Excel 工作簿 |
| 文本文件 | `.txt` | 文本格式数据 |
| Pickle | `.pkl` | Python 序列化数据 |

**导入单个数据文件：**

```shell
# 导入单个 CSV 文件
DAWorkBench.exe --import-data "D:\data.csv"

# 导入 Excel 文件
DAWorkBench.exe --import-data "D:\data.xlsx"
```

**导入多个数据文件：**

多次使用 `--import-data` 参数可导入多个数据文件：

```shell
# 同时导入多个数据文件
DAWorkBench.exe --import-data "D:\data1.csv" --import-data "D:\data2.xlsx"
```

**同时打开工程并导入数据：**

```shell
# 打开工程文件并导入额外数据
DAWorkBench.exe "D:\my-project.dapro" --import-data "D:\additional-data.csv"
```

### 显示版本号

使用 `--version` 参数快速查看程序版本信息：

```shell
# 显示版本号并退出
DAWorkBench.exe --version
```

程序输出示例：

```
DAWorkBench version 1.0.0
```

### 显示帮助信息

使用 `--help` 参数查看所有可用命令行参数：

```shell
# 显示帮助信息并退出
DAWorkBench.exe --help
```

## 常见使用场景

### 场景一：批量数据处理

通过命令行参数实现批量数据处理的自动化启动：

```mermaid
flowchart LR
    A[命令行启动] --> B[导入数据文件]
    B --> C[执行工作流]
    C --> D[导出结果]
```

命令示例：

```shell
DAWorkBench.exe "D:\batch-process.dapro" --import-data "D:\input-data.csv"
```

### 场景二：快捷启动指定工程

为常用工程创建快捷方式，直接打开目标工程：

```shell
# Windows 快捷方式目标设置
"D:\Qt\bin\DAWorkBench.exe" "D:\projects\daily-analysis.dapro"
```

## 注意事项

!!! warning "参数顺序"
    工程文件参数（位置参数）应放在其他参数之前，确保正确解析。

!!! info "数据格式兼容性"
    数据文件格式需与程序支持的解析器兼容。对于复杂格式的 Excel 文件，建议先在程序中手动导入测试。

!!! tip "自动化脚本"
    可将命令行参数与 Windows 批处理脚本或 PowerShell 结合，实现自动化数据处理流程。

## 参考资料

- [使用指南概述](../index.md)
- [工程文件说明](../index.md#工程文件)
- [数据导入方式](../index.md#数据导入)