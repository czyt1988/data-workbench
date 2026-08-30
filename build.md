# 构建指南

## 快速参考（Agent 专用）

> **前置条件**：第三方库已编译安装完毕（`bin_*` 安装目录已存在）。如未编译，见 [首次构建：编译第三方库](#首次构建编译第三方库)。

### Windows

```powershell
# 一键构建（推荐，自动探测 Qt/VS 路径）
.\scripts\build.ps1 -Target DAPyWorkFlow       # 编译指定模块
.\scripts\build.ps1 -Target DAPyWorkFlow -Test # 编译并运行测试
.\scripts\build.ps1 -Full                      # 完整构建
.\scripts\build.ps1 -Clean                     # 清理后重新配置+编译
```

> **务必使用 Visual Studio 生成器**，不要用 Ninja。PowerShell 中 `vcvars64.bat` 无法注入 MSVC 环境，`scripts/build.ps1` 已自动处理此问题。

### Linux / WSL

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel

# 运行测试
./build-linux/src/tst/DAPyWorkFlow/DAPyWorkFlowTests -o test_result.txt
cat test_result.txt
```

---

## 前置依赖

| 依赖 | 最低版本 | 说明 |
|------|---------|------|
| CMake | 3.15+ | 项目使用 CMake 构建系统 |
| C++17 编译器 | - | MSVC 2019+ / GCC 9+ |
| Qt | 5.14+ 或 6.x | 需要 Core、Gui、Widgets、Xml、Svg 等模块 |
| Python | 3.x（含开发头文件） | pybind11 绑定需要，为强制依赖 |

### Linux / WSL 依赖安装（Ubuntu 24.04）

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-svg-dev \
    qt6-5compat-dev qt6-tools-dev qt6-base-private-dev \
    libgl-dev libglu1-mesa-dev pkg-config libxkbcommon-dev \
    zlib1g-dev ninja-build python3-dev libpython3-dev
```

> **⚠️ `qt6-base-private-dev` 不可省略**：ADS (Qt-Advanced-Docking-System) 在 Linux 上使用了 Qt private headers，缺少此包会导致编译失败。

---

## 一键构建（Windows 推荐）

`scripts/build.ps1` 脚本自动探测 Qt 安装路径、VS 版本，无需手动指定任何路径：

```powershell
# 编译指定模块（最常用）
.\scripts\build.ps1 -Target DAPyWorkFlow

# 编译指定模块并运行测试
.\scripts\build.ps1 -Target DAPyWorkFlow -Test

# 完整构建（所有模块）
.\scripts\build.ps1 -Full

# 清理后重新配置+编译
.\scripts\build.ps1 -Clean
```

脚本功能：
- **Qt 自动检测**：搜索 `C:\Qt`、`D:\Qt`、`~\Qt`、`Program Files\Qt` 等常见目录，查找 msvc*_64 安装
- **VS 自动检测**：根据 Qt 的 MSVC 版本自动选择 VS 生成器（2019/2022）
- **CMake 自动检测**：从 PATH 或 VS 内嵌路径查找 cmake.exe

脚本支持的完整参数：

| 参数 | 取值 | 说明 |
|------|------|------|
| `-Target` | 模块名（可选） | 仅构建指定目标，省略时构建全部 |
| `-Full` | 开关 | 完整构建（所有模块） |
| `-Clean` | 开关 | 清理后重新配置+编译 |
| `-Test` | 开关 | 构建后通过 `ctest` 运行测试（测试目标需 `DA_ENABLE_TESTING=ON` 才会编译） |
| `-QtPath` | 路径 | 手动指定 Qt 安装路径，覆盖自动检测 |
| `-VSVersion` | `2019` / `2022` | 手动指定 Visual Studio 版本，覆盖根据 Qt 自动推断的结果 |
| `-Config` | `Release` / `Debug` / `RelWithDebInfo` / `MinSizeRel` | 构建配置，默认 `Release` |
| `-Plugins` | `ON` / `OFF` | 是否构建 plugins，对应 `DA_BUILD_PLUGINS`，默认 `ON` |

也可手动指定 Qt 路径（覆盖自动检测）：

```powershell
.\scripts\build.ps1 -Full -QtPath "D:/Qt/6.7.3/msvc2019_64"
```

---

## 首次构建：编译第三方库

> **仅首次构建或第三方库有更新时需要执行**。编译产物会安装到项目根目录的 `bin_<BuildType>_qt<QtVersion>_<Compiler>_<Arch>/` 目录，后续主项目构建会自动找到。

第三方库位于 `src/3rdparty/`，包含 SARibbon、qwt、ADS 等依赖。

!!! warning "zlib 需预先安装"
    `src/3rdparty/CMakeLists.txt` 通过 `find_package(ZLIB QUIET)` 查找 zlib，并未 `add_subdirectory(zlib)`（虽然 zlib 是 submodule，但不在此处统一构建）。因此执行下面的单命令构建前，**必须先独立编译并安装 zlib**，否则 `quazip` 会因找不到 zlib 而配置失败。zlib 的独立构建见 [构建说明 - 分步构建](docs/zh/build/build-instructions.md#分步构建)。

### Windows (Visual Studio 生成器)

```powershell
cmake -S src/3rdparty -B build-3rdparty -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64"
cmake --build build-3rdparty --config Release --parallel
cmake --install build-3rdparty --config Release
```

### Linux / WSL (Ninja 生成器)

```bash
cmake -S src/3rdparty -B build-linux-3rdparty -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux-3rdparty --parallel
cmake --install build-linux-3rdparty
```

> Linux apt 安装的 Qt6 无需指定 `CMAKE_PREFIX_PATH`。自定义 Qt 路径需添加 `-DCMAKE_PREFIX_PATH=<Qt路径>`。

### 安装目录命名

- Windows: `bin_Release_qt6.7.3_MSVC_x64`
- Linux: `bin_Release_qt6.7.3_GNU_x64`

格式为 `bin_<BuildType>_qt<QtVersion>_<Compiler>_<Arch>`。

---

## 手动构建（不使用脚本）

如果第三方库已编译安装，手动构建主项目只需：

### Windows (Visual Studio 生成器)

```powershell
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64"
cmake --build build --config Release --parallel
```

### Linux / WSL (Ninja 生成器)

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel
```

---

## 运行测试

### Windows

```powershell
# 使用脚本（推荐，自动定位 exe 并捕获输出）
.\scripts\build.ps1 -Target DAPyWorkFlow -Test

# 手动运行（Qt Test 在 Windows 上 stdout 不可见，必须用 -o 输出到文件）
.\build\src\tst\<测试模块>\Release\<测试模块>.exe -o test_result.txt
Get-Content test_result.txt
```

### Linux / WSL

```bash
./build-linux/src/tst/<测试模块>/<测试模块> -o test_result.txt
cat test_result.txt
```

| 注意事项 | Windows | Linux |
|----------|---------|-------|
| 测试 exe 路径 | VS: `build\src\tst\<模块>\<Config>\` | Ninja: `build-linux/src/tst/<模块>/` |
| 输出捕获 | **必须**用 `-o file.txt` | 可直接 stdout 或 `-o file.txt` |
| 退出码 | `$LASTEXITCODE` | `$?` 或 `echo $?` |

---

## 常见构建问题

### Windows

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| `find_package` 失败，找不到 SARibbon/qwt/ADS | 第三方库未编译 install | 执行 [首次构建：编译第三方库](#首次构建编译第三方库) |
| Ninja 生成器报 `fatal error C1083` | MSVC 环境未注入 | **不要用 Ninja**，使用 VS 生成器或 `scripts/build.ps1` |
| Qt 路径找不到 | `CMAKE_PREFIX_PATH` 指向不存在的目录 | 使用 `scripts/build.ps1`（自动探测），或手动确认路径存在 |
| Qt 版本与 VS 编译器不匹配 | Qt msvc2019 配合 VS2022 编译器 | 确保 Qt 编译器版本与 VS 版本一致，脚本会自动匹配 |
| moc 异常退出 | Qt moc 大量项目时的已知 bug | 重新构建即可 |

### Linux

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| `qpa/qplatformnativeinterface.h: No such file or directory` | 缺少 `qt6-base-private-dev` | `sudo apt install qt6-base-private-dev` |
| `Could NOT find Qt6Core5Compat` | 缺少 Core5Compat 开发包 | `sudo apt install qt6-5compat-dev` |
| `OpenGL::GLU not found` | 缺少 GLU 开发库 | `sudo apt install libglu1-mesa-dev` |
| `Could NOT find Qt6LinguistTools` | 缺少 Qt6 Tools 开发包 | `sudo apt install qt6-tools-dev` |
| `QIODevice` incomplete type | Qt6 不再通过 `QDataStream` 隐式包含 | 手动添加 `#include <QIODevice>` |
| `uint64_t` ambiguous overload | Linux 上 `uint64_t` = `unsigned long` ≠ `unsigned long long` | 使用 `qulonglong` 或 `static_cast<qulonglong>()` |
| moc 异常退出 / 不完整类型 | 信号槽传递自定义类型指针时只有前向声明 | 在 .cpp 文件中 `#include` 完整头文件 |

---

## 注意事项

1. **第三方库仅需编译一次**：首次构建或第三方 submodule 有更新时执行 install 即可，日常开发无需重复
2. **Windows 务必使用 VS 生成器**：不要用 Ninja，PowerShell 中 MSVC 环境无法正确注入
3. **Qt 路径**：Windows 需指定 `CMAKE_PREFIX_PATH`；Linux apt 安装的 Qt6 无需指定
4. **Linux 必需包**：`qt6-base-private-dev` 是 ADS 在 Linux 上的硬性依赖，不可省略
5. **GCC `-fpermissive`**：项目在 `CMakeLists.txt` 中为 GCC 自动添加此选项，允许 MSVC 风格的命名空间额外限定
6. **Qt 运行时自动部署（Windows）**：`cmake --install build --config <Config>` 会在安装时自动执行
   windeployqt，把完整 Qt 运行时（含 WebEngine/Qml 整条依赖链、platforms 等插件目录、翻译）
   部署到安装目录 `bin/`，打包 `bin_<Config>_qt<X>_...` 目录时无需再手工运行 windeployqt。
   实现见 `cmake/daworkbench_utils.cmake` 的 `dafun_install_deploy_qt_runtime()`。
   注意：WebEngine/Qml 是 `DAGui.dll` 的传递依赖而非主程序的直接依赖，手工运行
   windeployqt 时必须把 `DAGui.dll` 一并传入，否则会漏掉整条依赖链
