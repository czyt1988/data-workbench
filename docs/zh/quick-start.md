# 快速上手指南

本指南帮助您快速开始使用 DAWorkBench。

---

## 前置要求

| 要求 | 版本 | 说明 |
|------|------|------|
| **Qt** | 5.14+ 或 6.x | GUI 框架 |
| **Python** | 3.7+ (推荐 3.11) | 数据处理（可选） |
| **CMake** | 3.16+ | 构建系统 |
| **Ninja** | 推荐 | 快速构建工具 |
| **编译器** | C++17 支持 | MSVC/GCC/Clang |

---

## 获取源码

```bash
# 克隆仓库
git clone https://github.com/czyt1988/data-workbench.git

# 进入项目目录
cd data-workbench

# 拉取第三方库（重要！）
git submodule update --init --recursive
```

!!! warning "注意"
    必须使用 git clone，不要直接下载 zip 包，因为项目使用 submodule 管理第三方库。

---

## 构建步骤

!!! warning "重要：必须使用 Qt 工具链文件"
    构建项目**必须**指定 Qt 工具链文件（`qt.toolchain.cmake`），否则会出现 Windows SDK 头文件找不到的问题。

### 一键构建（推荐）

```powershell
# Windows PowerShell - 使用 Qt 6 工具链文件
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake"

cmake --build build --config Release --parallel
cmake --build build --config Release --target install
```

!!! info "Qt 路径"
    请根据实际 Qt 安装路径修改 `CMAKE_TOOLCHAIN_FILE` 参数。

### 分步构建

如需分步构建第三方库，请参阅 [构建说明](./build/build-instructions.md#分步构建)。

!!! tip "提示"
    构建完成后，所有文件将安装到 `bin_Release_qt{版本}_{编译器}_x64` 目录。

---

## 运行程序

构建输出目录格式：`bin_{BuildType}_qt{QtVersion}_{Compiler}_{Arch}`

### Windows

```powershell
# 进入输出目录（示例：Qt 6.7.3, Release, MSVC）
.\bin_Release_qt6.7.3_MSVC_x64\DAWorkbench.exe
```

### Linux

```bash
# 进入输出目录（示例：Qt 6.7.3, Release, GCC）
./bin_Release_qt6.7.3_GCC_x64/DAWorkbench
```

---

## 验证安装

程序启动后，您应该看到：

1. **Ribbon 工具栏** - 顶部功能区
2. **工作流面板** - 左侧节点列表
3. **数据面板** - 数据管理区域
4. **图表区域** - 可视化显示

![主界面](../assets/screenshot/01.png)

---

## 下一步

- [:material-book: 项目概览](./overview.md) - 了解项目详情
- [:material-folder: 项目结构](./project-structure.md) - 理解目录组织
- [:material-puzzle: 插件开发](./plugin-development.md) - 开发自定义插件

---

## 常见构建问题

### 问题 1：找不到第三方库

**原因**：未执行 `git submodule update`

**解决**：
```bash
git submodule update --init --recursive
```

### 问题 2：Python 环境问题

**原因**：未配置 Python 路径

**解决**：设置环境变量或使用 CMake 选项：
```bash
cmake -DDA_ENABLE_AUTO_INSTALL_PYTHON_ENV=ON ...
```

### 问题 3：Qt 工具链文件找不到

**原因**：未指定 Qt 工具链文件路径

**解决**：使用正确的工具链文件路径：
```powershell
# Qt 6 工具链文件通常位于：
-D CMAKE_TOOLCHAIN_FILE="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake"

# Qt 5 工具链文件通常位于：
-D CMAKE_TOOLCHAIN_FILE="C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5\qt.toolchain.cmake"
```

---

## 详细构建文档

如需更多构建选项和详细说明，请参阅：

- [构建说明](./build/build-instructions.md) - 完整构建指南
- [Python 环境配置](./build/python-environment.md) - Python 环境设置