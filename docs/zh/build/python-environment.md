# Python 环境配置

本文档介绍 Python 环境的配置方法，Python 是数据处理模块的后端引擎，正确配置对软件功能至关重要。

本文档说明 DAWorkBench 的 Python 环境配置方法，包括构建时和运行时的 Python 环境设置。

## 主要功能特性

**特性**

- ✅ **可选 Python 支持**：可禁用 Python 模块，仅使用绘图功能
- ✅ **嵌入式 Python**：支持 embeddable package，便于打包发布
- ✅ **灵活配置**：支持配置文件和系统环境两种查找方式

---

## Python 环置概述

### 构建时 Python 环境

DAWorkBench 可选择是否依赖 Python：

- **启用 Python**：自动查找系统 Python 环境，编译 Python 相关模块
- **禁用 Python**：通过 CMake 选项 `DA_ENABLE_PYTHON=OFF` 禁用

!!! tip "仅绘图场景"
    如果仅需使用绘图模块，可禁用 Python 环境以简化构建。

### 运行时 Python 环境

程序运行时按以下逻辑查找 Python：

1. **配置文件优先**：检查程序目录下的 `python-config.json`
2. **系统环境备选**：使用 `where python` 查找系统 Python

---

## Python 版本要求

| 要求 | 说明 |
|------|------|
| 最低版本 | Python 3.7+ |
| 推荐版本 | Python 3.11+（AI 功能需要） |

### Python 依赖库

| 库名 | 用途 |
|------|------|
| pandas | 数据处理核心 |
| numpy | 数值计算 |
| scipy | 科学计算 |
| loguru | Python 脚本日志 |
| openpyxl | Excel 文件导入依赖 |
| chardet | 字符编码检测 |
| PyWavelets | 小波分析 |
| pyarrow | Parquet/Feather 格式支持 |

安装依赖：

以下命令安装项目所需的 Python 依赖库。建议使用 requirements.txt 批量安装：

```shell
# 使用 pip 批量安装依赖
# requirements.txt 位于项目根目录
pip install -r requirements.txt
```

如需单独安装核心库，可使用以下命令：

```shell
# 安装数据处理核心库
pip install pandas numpy scipy

# 安装辅助库（Excel、编码检测等）
pip install openpyxl chardet loguru
```

---

## 嵌入式 Python 配置（推荐发布）

!!! tip "embeddable package"
    使用 Python embeddable package 可获得干净的 Python 环境，便于打包发布，不影响系统 Python。

### 步骤一：下载 embeddable package

从 Python 官网下载 Windows embeddable package：

- 下载地址：[https://www.python.org/downloads/windows](https://www.python.org/downloads/windows)
- 选择与系统 Python **完全相同版本**（版本号、架构需匹配）

### 步骤二：启用 site 模块

1. 解压 Python embeddable package 到程序目录
2. 找到 `python3xx._pth` 文件（如 `python313._pth`）
3. 删除 `#import site` 行首的 `#`，变为 `import site`
4. 保存文件

### 步骤三：安装 pip

以下命令下载并安装 pip 到嵌入式 Python 环境中。pip 是 Python 的包管理工具，必需安装：

```powershell
# 下载 get-pip.py
# 从 pypa.io 获取 pip 安装脚本，保存到 Python 目录
# 访问 https://bootstrap.pypa.io/get-pip.py 下载

# 安装 pip
# 使用嵌入式 Python 执行安装脚本
.\python.exe get-pip.py

# 删除安装脚本（可选清理）
del get-pip.py
```

### 步骤四：安装依赖

使用 `--target` 参数安装依赖到嵌入式环境的 `Lib/site-packages` 目录。这种方式便于打包发布：

```powershell
# 安装基础工具
# setuptools 和 wheel 是 pip 安装包的基础依赖
.\python.exe -m pip install --target="./Lib/site-packages" setuptools wheel

# 安装项目依赖
# 批量安装 requirements.txt 中列出的所有库
.\python.exe -m pip install --target="./Lib/site-packages" -r requirements.txt

# 使用清华镜像加速（推荐国内用户）
# -i 参数指定镜像源，大幅提升下载速度
.\python.exe -m pip install --target="./Lib/site-packages" -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple
```

!!! warning "Windows 包安装位置"
    Windows 下 pip 默认安装到用户目录 `%APPDATA%\Python\Pythonxx\site-packages`，不利于打包。使用 `--target` 参数指定安装位置。

### 步骤五：完善库路径

解压 `python3xx.zip` 内容到 `Lib` 目录，形成完整 Python 环境。

---

## 运行时 Python 配置

### 配置文件方式

创建 `python-config.json` 文件指定 Python 解释器路径。此文件放在程序目录下，程序启动时优先读取：

```json
{
  "config": {
    "interpreter": "${current-app-dir}/python311/python.exe"
  }
}
```

!!! info "路径变量"
    `${current-app-dir}` 代表程序安装目录。例如 Python 安装在程序目录下：
    ```
    ${current-app-dir}/python311/python.exe
    ```
    此路径变量使配置文件可移植，无需修改即可在不同安装位置使用。

### 系统环境方式

如无配置文件，程序使用系统 Python 环境（`where python` 查找）。

---

## CMake 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `DA_ENABLE_PYTHON` | ON | 启用/禁用 Python 模块 |
| `DA_ENABLE_AUTO_INSTALL_PYTHON_ENV` | OFF | 自动安装 Python 依赖 |
| `Python_ROOT_DIR` | - | 指定 Python 安装路径 |

### 禁用 Python 构建

如果仅需使用绘图功能，可通过 CMake 参数禁用 Python 模块。以下命令展示如何构建不包含 Python 支持的版本：

```powershell
# 配置项目并禁用 Python 模块
# DA_ENABLE_PYTHON=OFF 跳过 Python 相关代码编译
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake" `
    -DDA_ENABLE_PYTHON=OFF
```

禁用 Python 后，软件仅保留绘图和数据可视化功能，无法执行 pandas 数据处理操作。

---

## 开发环境建议

### 双 Python 环境策略

建议维护两个 Python 环境：

| 环境 | 用途 | 说明 |
|------|------|------|
| 系统 Python | 开发构建 | 提供头文件和库文件 |
| 嵌入式 Python | 打包发布 | 干净的运行环境 |

!!! warning "版本必须匹配"
    两个 Python 环境版本必须完全相同（版本号、架构）。

---

## 常见问题

### Python.h 找不到

**原因**：未安装 Python 开发包（Windows 下通常需要安装完整 Python，而非仅解释器）

**解决**：安装 Python 开发库，或指定 Python 路径使 CMake 正确定位头文件：

```powershell
# 方式一：设置 Python_ROOT_DIR 环境变量
# 指定 Python 安装根目录
$env:Python_ROOT_DIR = "C:\Python311"

# 方式二：CMake 参数指定路径
cmake -DPython_ROOT_DIR="C:\Python311" ...
```

### 运行时 Python 找不到

**原因**：未配置 `python-config.json`

**解决**：创建配置文件或确保系统 Python 在 PATH 中。

### 包导入失败

**原因**：`--target` 安装的包路径未配置

**解决**：解压 `python3xx.zip` 到 Lib 目录，或将 `Lib/site-packages` 添加到 zip 中。

---

## 参考资料

- [构建说明](./build-instructions.md) - 完整构建指南
- [Python embeddable package 文档](https://docs.python.org/3/using/windows.html#embedded-distribution)
- [pip 安装指南](https://pip.pypa.io/en/stable/installation/)