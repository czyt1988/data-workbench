# 快速上手指南

本指南帮助您快速开始使用 DAWorkBench。

---

## 前置要求

| 要求 | 版本 | 说明 |
|------|------|------|
| **Qt** | 5.14+ 或 6.x | GUI 框架 |
| **Python** | 3.8+ | 数据处理（可选） |
| **CMake** | 3.12+ | 构建系统 |
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

### 第一步：构建 zlib

```bash
cmake -S src/3rdparty/zlib -B build/zlib -DCMAKE_BUILD_TYPE=Release
cmake --build build/zlib --config Release
cmake --install build/zlib --prefix ./bin_Release
```

### 第二步：构建第三方库

```bash
cmake -S src/3rdparty -B build/3rdparty -DCMAKE_BUILD_TYPE=Release
cmake --build build/3rdparty --config Release
cmake --install build/3rdparty --prefix ./bin_Release
```

!!! tip "提示"
    构建完成后，所有第三方库将安装到 `bin_Release` 目录。

### 第三步：构建主程序

```bash
cmake -S . -B build/main -DCMAKE_BUILD_TYPE=Release
cmake --build build/main --config Release
cmake --install build/main --prefix ./bin_Release
```

---

## 运行程序

### Windows

```bash
# 进入 bin 目录
cd bin_Release_qt5.x_MSVC_x64/bin

# 运行程序
DAWorkbench.exe
```

### Linux

```bash
# 进入 bin 目录
cd bin_Release_qt5.x_GNU_x64/bin

# 运行程序
./DAWorkbench
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

### 问题 3：Qt 版本不匹配

**原因**：系统安装了多个 Qt 版本

**解决**：指定 Qt 路径：
```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/msvc2019_64 ...
```

---

## 完整构建命令（一键脚本）

### Windows PowerShell

```powershell
# 设置 Qt 路径（根据实际安装位置修改）
$QtPath = "C:/Qt/5.15.2/msvc2019_64"

# 构建所有
cmake -S src/3rdparty/zlib -B build/zlib -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$QtPath
cmake --build build/zlib --config Release
cmake --install build/zlib --prefix ./bin_Release

cmake -S src/3rdparty -B build/3rdparty -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$QtPath
cmake --build build/3rdparty --config Release
cmake --install build/3rdparty --prefix ./bin_Release

cmake -S . -B build/main -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$QtPath
cmake --build build/main --config Release
cmake --install build/main --prefix ./bin_Release
```

### Linux Bash

```bash
# 设置 Qt 路径
export QtPath=/opt/Qt5.15.2

cmake -S src/3rdparty/zlib -B build/zlib -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$QtPath
cmake --build build/zlib
cmake --install build/zlib --prefix ./bin_Release

cmake -S src/3rdparty -B build/3rdparty -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$QtPath
cmake --build build/3rdparty
cmake --install build/3rdparty --prefix ./bin_Release

cmake -S . -B build/main -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$QtPath
cmake --build build/main
cmake --install build/main --prefix ./bin_Release
```