# Python 环境配置

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

```shell
pip install -r requirements.txt
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

```powershell
# 下载 get-pip.py
# https://bootstrap.pypa.io/get-pip.py 保存到 Python 目录

# 安装 pip
.\python.exe get-pip.py

# 删除 get-pip.py
del get-pip.py
```

### 步骤四：安装依赖

使用 `--target` 参数安装到嵌入式环境：

```powershell
# 安装基础工具
.\python.exe -m pip install --target="./Lib/site-packages" setuptools wheel

# 安装项目依赖
.\python.exe -m pip install --target="./Lib/site-packages" -r requirements.txt

# 使用清华镜像加速
.\python.exe -m pip install --target="./Lib/site-packages" -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple
```

!!! warning "Windows 包安装位置"
    Windows 下 pip 默认安装到用户目录 `%APPDATA%\Python\Pythonxx\site-packages`，不利于打包。使用 `--target` 参数指定安装位置。

### 步骤五：完善库路径

解压 `python3xx.zip` 内容到 `Lib` 目录，形成完整 Python 环境。

---

## 运行时 Python 配置

### 配置文件方式

创建 `python-config.json` 文件：

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

```powershell
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake" `
    -DDA_ENABLE_PYTHON=OFF
```

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

**原因**：未安装 Python 开发包

**解决**：安装 Python 开发库，或指定 Python 路径：

```powershell
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