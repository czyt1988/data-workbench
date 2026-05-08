# 构建指引

## 构建环境

### Windows

- CMake 3.15+
- Visual Studio 2019（MSVC 14.29+）
- Qt 6.7+ (msvc2019\_64) 或 Qt 5.14+

### Linux / WSL

- CMake 3.15+
- GCC 9+ (推荐 GCC 13+)
- Qt 6.x（通过 apt 安装 `qt6-base-dev` 等）或 Qt 5.14+（手动安装）
- Python 3.x（含开发头文件）

#### Ubuntu 24.04 (WSL) apt 依赖安装

```bash
# Qt6 核心开发包
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-svg-dev

# Qt6 扩展模块（ctk 需要 Core5Compat，翻译需要 LinguistTools）
sudo apt install qt6-5compat-dev qt6-tools-dev

# Qt6 private headers（ADS 在 Linux 上必需）
sudo apt install qt6-base-private-dev

# OpenGL / GLU（qwt plot3d 模块需要）
sudo apt install libgl-dev libglu1-mesa-dev

# 其他依赖
sudo apt install pkg-config libxkbcommon-dev zlib1g-dev ninja-build

# Python 开发头文件（pybind11 需要）
sudo apt install python3-dev libpython3-dev
```

> **⚠️ `qt6-base-private-dev` 是 Linux 构建的必需包**：Qt-Advanced-Docking-System (ADS) 在 Linux 上使用了 Qt private headers (`qpa/qplatformnativeinterface.h`)，缺少此包 ADS 无法编译。

## 构建步骤

项目分为两步构建：**先编译第三方库并安装**，再编译主项目。

### 第一步：编译第三方库并安装

第三方库位于 `src/3rdparty/`，需要独立配置、编译并执行 `install`，所有依赖将安装到项目根目录下的 `bin_<BuildType>_qt<QtVersion>_<Compiler>_<Arch>/` 目录。

#### Windows (Visual Studio 生成器)

```powershell
# 在项目根目录下
cd src/3rdparty
mkdir build && cd build

# 配置（使用 Visual Studio 2019 生成器）
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="D:/Qt/6.7.3/msvc2019_64"

# 编译并安装
cmake --build . --config Release
cmake --install . --config Release
```

#### Windows (Ninja 生成器，需初始化 MSVC 环境)

```powershell
# 初始化 MSVC 环境（vcvarsall.bat 会设置 INCLUDE、LIB、PATH 等关键变量）
cmd /c '"D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 & set' | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}

# 然后使用 Ninja 构建
cd src/3rdparty
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/Qt/6.7.3/msvc2019_64"
cmake --build .
cmake --install .
```

#### Linux / WSL (Unix Makefiles 或 Ninja)

```bash
# 在项目根目录下
cd src/3rdparty
mkdir build-linux && cd build-linux

# 配置（Ninja 生成器，编译更快）
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# 或使用 Unix Makefiles
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

# 编译并安装
cmake --build . --parallel
cmake --install .
```

> **注意**：Linux 上系统 Qt6 通过 apt 安装后无需指定 `CMAKE_PREFIX_PATH`，cmake 会自动找到。如使用自定义 Qt 安装路径，需添加 `-DCMAKE_PREFIX_PATH=<Qt路径>`。

### 第二步：编译主项目

#### Windows (Visual Studio 生成器)

```powershell
# 在项目根目录下
mkdir build && cd build

# 配置
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="D:/Qt/6.7.3/msvc2019_64"

# 构建（Debug 或 Release）
cmake --build . --config Release --parallel
```

#### Linux / WSL (Unix Makefiles 或 Ninja)

```bash
# 在项目根目录下
mkdir build-linux && cd build-linux

# 配置
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build . --parallel
```

## 注意事项

1. **必须先编译安装第三方库**：主项目依赖第三方库的 cmake 配置文件，未执行 install 会导致 find_package 失败
2. **生成器选择**：Windows 推荐使用 Visual Studio 生成器（自动处理 MSVC 环境）；Linux/WSL 推荐 Ninja（编译更快）
3. **Qt路径**：Windows 需指定 `CMAKE_PREFIX_PATH`；Linux apt 安装的 Qt6 无需指定
4. **Linux 必需包**：`qt6-base-private-dev` 是 ADS 在 Linux 上的硬性依赖，不可省略
5. **清理构建**：如需重新配置，删除 `build-linux` 目录重新 cmake 即可
6. **安装目录命名**：自动生成的安装目录名为 `bin_<BuildType>_qt<QtVersion>_<Compiler>_<Arch>`，例如 Windows 下为 `bin_Release_qt6.4.2_MSVC_x64`，Linux 下为 `bin_Release_qt6.4.2_GNU_x64`
7. **GCC `-fpermissive`**：项目在 `CMakeLists.txt` 中为 GCC 自动添加 `-fpermissive` 编译选项，允许 MSVC 风格的命名空间额外限定（`DA::DAEnumTraits` 在 DA namespace 内使用）。如不需此选项可移除，但需同步修改 `DAEnumStringUtils.hpp` 宏定义

## 常见问题

### Linux 构建常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| `qpa/qplatformnativeinterface.h: No such file or directory` | 缺少 `qt6-base-private-dev` | `sudo apt install qt6-base-private-dev` |
| `Could NOT find Qt6Core5Compat` | 缺少 Core5Compat 开发包 | `sudo apt install qt6-5compat-dev` |
| `OpenGL::GLU not found` | 缺少 GLU 开发库 | `sudo apt install libglu1-mesa-dev` |
| `Could NOT find Qt6LinguistTools` | 缺少 Qt6 Tools 开发包 | `sudo apt install qt6-tools-dev` |
| `QIODevice` incomplete type | 缺少 `#include <QIODevice>` | Qt6 不再通过 `QDataStream` 隐式包含，需手动添加 |
| `uint64_t` ambiguous overload | Linux 上 `uint64_t` = `unsigned long` ≠ `unsigned long long` | 使用 `qulonglong` 或 `static_cast<qulonglong>()` |
| moc 异常退出 / 不完整类型 | Qt 信号槽传递自定义类型指针时只有前向声明 | 在 .cpp 文件中 `#include` 完整头文件而非仅前向声明 |

### Windows 构建常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Ninja 生成器找不到标准库头文件 | 未初始化 MSVC 环境变量 | 先运行 `vcvarsall.bat x64` 或使用 VS 生成器 |
| moc 异常退出 | Qt moc 大量项目时的已知 bug | 重新构建即可 |