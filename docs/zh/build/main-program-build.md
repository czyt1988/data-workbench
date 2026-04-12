# 构建主程序

本文档介绍如何构建 DAWorkBench 主程序。构建前请确认已完成第三方库的构建，详见 [构建第三方库](./third-party-build.md)。

## 主要功能特性

**特性**

- ✅ **跨平台构建**：支持 Windows、Linux 平台
- ✅ **多 Qt 版本**：同时支持 Qt 5.14+ 和 Qt 6.x
- ✅ **命令行构建**：支持 CMake 命令行构建
- ✅ **IDE 构建**：支持 Qt Creator 图形化构建

---

## 命令行构建（推荐）

!!! warning "必须使用 Qt 工具链文件"
    构建项目**必须**指定 Qt 工具链文件（`qt.toolchain.cmake`），否则会出现 Windows SDK 头文件找不到的问题。

### Windows PowerShell

```powershell
# 进入项目根目录
cd C:\path\to\data-workbench

# 配置项目（必须指定 Qt 工具链文件）
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE:STRING=Release `
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE `
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake"

# 构建项目（使用所有 CPU 核心）
cmake --build build --config Release --parallel

# 安装到 bin 目录
cmake --build build --config Release --target install
```

### Linux Bash

```bash
# 进入项目根目录
cd /path/to/data-workbench

# 配置项目
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/opt/Qt/6.7.3/gcc_64/lib/cmake/Qt6/qt.toolchain.cmake

# 构建项目
cmake --build build --config Release --parallel

# 安装到 bin 目录
cmake --build build --config Release --target install
```

### CMake 参数说明

| 参数 | 必需 | 说明 |
|------|:----:|------|
| `-DCMAKE_TOOLCHAIN_FILE` | ✅ | Qt 工具链文件路径，**必须指定** |
| `-DCMAKE_BUILD_TYPE` | ✅ | 构建类型：`Debug` 或 `Release` |
| `-G Ninja` | 推荐 | 使用 Ninja 生成器，构建更快 |
| `-DCMAKE_EXPORT_COMPILE_COMMANDS` | 可选 | 生成 LSP 配置文件 |

---

## Qt Creator 构建

### 1. 打开项目

打开 Qt Creator，选择 **文件** → **打开文件或项目**（`Ctrl+O`），选择项目根目录下的 `CMakeLists.txt` 文件。

![build-daworkbench-cmake-qtc](../../assets/PIC/build-daworkbench-cmake-qtc-01.png)

### 2. 选择构建模式

切换到项目模式（`Ctrl+5`），Build 步骤选择 `all`，如需安装可勾选 `install`。

![build-daworkbench-cmake-qtc](../../assets/PIC/build-daworkbench-cmake-qtc-02.png)

### 3. 配置工具链文件

!!! warning "手动指定工具链文件"
    如果 Qt Creator 无法正确检测 Windows SDK，需要手动配置 CMake 参数：
    
    1. 打开 **项目** → **构建设置**
    2. 在 **CMake** 配置中添加参数：
       ```
       -DCMAKE_TOOLCHAIN_FILE:FILEPATH=D:/Qt/6.7.3/msvc2019_64/lib/cmake/Qt6/qt.toolchain.cmake
       ```

### 4. 编译和运行

点击运行（`Ctrl+R`）进行编译和安装。

!!! tip "首次运行提示"
    编译完的首次运行可能报错，因为第三方库的 DLL 未复制到构建目录。需要手动把第三方库的 DLL 复制到构建目录下的 bin 文件夹中，包括 `zlib.dll`（quazip 的依赖）。

---

## 第三方库路径设置

!!! info "默认配置"
    如果未修改第三方库安装路径，此步骤可省略。

主项目的 `CMakeLists.txt` 已预配置第三方库路径：

```cmake
# 定义第三方库路径
set(SARibbonBar_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/SARibbonBar)
set(DALiteCtk_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/DALiteCtk)
set(qwt_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/qwt)
set(QtPropertyBrowser_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/QtPropertyBrowser)
set(spdlog_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/spdlog)
set(tsl-ordered-map_DIR ${DA_INSTALL_LIB_SHARE_PATH}/tsl-ordered-map)
set(qt${QT_VERSION_MAJOR}advanceddocking_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/qt${QT_VERSION_MAJOR}advanceddocking)
```

如修改了安装路径，需在构建时指定正确位置。

---

## 构建输出目录

构建输出目录命名规则：

```
bin_{BuildType}_qt{QtVersion}_{Compiler}_{Arch}
```

| 构建配置 | 输出目录示例 |
|----------|-------------|
| Qt 6.7.3, Release, MSVC, x64 | `bin_Release_qt6.7.3_MSVC_x64` |
| Qt 5.15.2, Debug, MSVC, x64 | `bin_Debug_qt5.15.2_MSVC_x64` |
| Qt 6.7.3, Release, GCC, x64 | `bin_Release_qt6.7.3_GCC_x64` |

---

## 验证构建

```powershell
# Windows - 检查输出文件
dir bin_Release_qt6.7.3_MSVC_x64\*.exe
dir bin_Release_qt6.7.3_MSVC_x64\*.dll

# 运行程序
.\bin_Release_qt6.7.3_MSVC_x64\DAWorkbench.exe
```

程序启动后显示主窗口界面，表示构建成功。

---

## 常见问题

### DLL 缺失

运行时提示缺少 DLL，可使用 `windeployqt` 自动复制依赖：

```powershell
cd bin_Release_qt6.7.3_MSVC_x64
windeployqt DAWorkbench.exe
```

### 第三方库找不到

确保第三方库已正确编译并安装。参考 [构建第三方库](./third-party-build.md)。

---

## 参考资料

- [构建说明](./build-instructions.md) - 完整构建指南
- [Python 环境配置](./python-environment.md) - Python 环境设置
- [Qt CMake 手册](https://doc.qt.io/qt-6/cmake-manual.html)