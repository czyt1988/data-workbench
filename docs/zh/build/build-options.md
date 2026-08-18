# 构建选项参考

本页面列出 DAWorkBench 项目的所有 CMake 构建选项，说明其作用、默认值和影响范围。

## 主要功能特性

- ✅ **完整选项列表**：6 个 CMake 选项的详细说明
- ✅ **影响范围**：每个选项影响的模块和构建行为
- ✅ **使用示例**：常见构建场景的选项组合
- ✅ **选项交互**：选项间的依赖和冲突关系

---

## 选项总览

| 选项 | 默认值 | 简述 | 影响范围 |
|------|:------:|------|---------|
| `DA_ENABLE_AUTO_INSTALL_PYTHON_ENV` | `ON` | 自动搜索并部署 Python DLL | APP（仅 Windows） |
| `DA_ENABLE_AUTO_TRANSLATE` | `ON` | 自动编译翻译文件（.ts → .qm） | i18n |
| `DA_AUTO_INSTALL_PREFIX` | `ON` | 自动安装到本地目录 | 所有模块的 install 行为 |
| `DA_AUTO_GENERATE_CONFIG_INFO` | `OFF` | 自动生成 DAConfigs.h（已改为无条件生成） | 库开发者专用 |
| `DA_BUILD_PLUGINS` | `ON` | 构建 plugins/ 目录下的插件 | plugins/ 子目录 |
| `DA_ENABLE_TESTING` | `OFF` | 启用测试构建（enable_testing + src/tst） | src/tst 子目录 |

---

## 选项详解

!!! info "Python 为强制依赖"
    Python 不再是可选构建选项，而是强制依赖，始终参与编译，无法禁用。相关模块包括 DAPyBindQt、DAPyScripts、DAPyCommonWidgets、DAPyWorkFlow、DAData。如需配置 Python 环境，参见 [Python 环境配置](./python-environment.md)。

### DA_ENABLE_AUTO_INSTALL_PYTHON_ENV

- **默认值**：`ON`
- **作用**：构建完成后自动搜索 Python 环境，将必要的 DLL 复制到 `bin/` 目录
- **影响范围**：仅 Windows 平台，仅 APP 模块
- **适用场景**：Windows 用户未将 Python 目录设为系统环境变量时

```bash
# 启用自动部署（推荐 Windows 用户）
cmake -DDA_ENABLE_AUTO_INSTALL_PYTHON_ENV=ON ...

# 禁用自动部署（已配置 Python 环境变量时）
cmake -DDA_ENABLE_AUTO_INSTALL_PYTHON_ENV=OFF ...
```

!!! tip "Linux 用户"
    此选项在 Linux 上无效。Linux 通过系统包管理器或 virtualenv 管理 Python 环境。

### DA_ENABLE_AUTO_TRANSLATE

- **默认值**：`ON`
- **作用**：构建时自动调用 Qt Linguist 工具，将 `.ts` 翻译源文件编译为 `.qm` 二进制翻译文件
- **依赖**：需要安装 Qt Linguist 工具（`lrelease`）
- **影响范围**：i18n 翻译文件

```bash
# 启用自动翻译（默认）
cmake -DDA_ENABLE_AUTO_TRANSLATE=ON ...

# 禁用自动翻译（加速构建，无翻译需求时）
cmake -DDA_ENABLE_AUTO_TRANSLATE=OFF ...
```

### DA_AUTO_INSTALL_PREFIX

- **默认值**：`ON`
- **作用**：自动将构建结果安装到本地目录（`bin_<Config>_qt<Ver>_<Compiler>_<Arch>/`）
- **影响范围**：所有模块的 `install` 行为

```bash
# 启用自动安装（默认，推荐）
cmake -DDA_AUTO_INSTALL_PREFIX=ON ...

# 禁用自动安装，使用 CMAKE_INSTALL_PREFIX
cmake -DCMAKE_INSTALL_PREFIX=/custom/path -DDA_AUTO_INSTALL_PREFIX=OFF ...
```

### DA_AUTO_GENERATE_CONFIG_INFO

- **默认值**：`OFF`
- **作用**：历史上用于控制是否生成 `DAConfigs.h` 配置头文件（包含版本号等编译时信息）
- **现状**：`DAConfigs.h` 现已改为**无条件**生成。`src/CMakeLists.txt` 通过 `configure_file` 从 `DAConfigs.h.in` 结合顶层 `CMakeLists.txt` 中的 `DA_VERSION_*` 变量生成 `DAConfigs.h`，确保启动画面/关于对话框/`--version` 显示的版本号始终与 CMake 定义保持同步
- **背景**：历史上此处曾以 `DA_AUTO_UPDATE_CONFIG_INFO` 作为开关，但该变量名与顶层 `option(DA_AUTO_GENERATE_CONFIG_INFO)` 不一致，导致生成从未被触发，仓库中 `DAConfigs.h` 长期为陈旧版本。该开关已移除，生成现无条件执行
- **影响范围**：`DAConfigs.h` 文件生成（实际已与该选项无关，选项保留仅为历史兼容）

```bash
# 该选项当前对生成行为无实质影响，DAConfigs.h 已无条件生成
cmake -DDA_AUTO_GENERATE_CONFIG_INFO=ON ...
```

!!! info "翻译源文件更新"
    若需提取新增的 `tr()` 字符串到 `.ts` 翻译源文件，不再通过 CMake 选项触发，而是构建 `update_translations` 自定义目标：

    ```bash
    # 手动更新 .ts 翻译源文件（不加入 ALL，避免每次构建重写 .ts）
    cmake --build <build-dir> --target update_translations
    ```

### DA_BUILD_PLUGINS

- **默认值**：`ON`
- **作用**：是否构建 `plugins/` 目录下的插件
- **影响范围**：`plugins/` 子目录下的所有插件项目

```bash
# 构建插件（默认）
cmake -DDA_BUILD_PLUGINS=ON ...

# 仅构建主程序，跳过插件
cmake -DDA_BUILD_PLUGINS=OFF ...
```

### DA_ENABLE_TESTING

- **默认值**：`OFF`
- **作用**：开启时调用 `enable_testing()` 并 `add_subdirectory(src/tst)`，构建 `src/tst` 下的各测试工程
- **影响范围**：`src/tst` 子目录（如 `DAPyWorkFlowTests`、`DADataFrameTest` 等）
- **配套工具**：`scripts/build.ps1 -Test` 在构建完成后调用 `ctest` 运行测试。注意 `-Test` 仅负责执行 `ctest`，测试目标本身需在配置阶段通过 `DA_ENABLE_TESTING=ON` 才会参与编译

```bash
# 启用测试构建
cmake -DDA_ENABLE_TESTING=ON ...

# 构建并运行测试（使用项目脚本）
.\scripts\build.ps1 -Target DAPyWorkFlow -Test
```

!!! warning "默认关闭"
    测试工程不参与日常构建。仅当需要运行/开发测试时，在配置阶段显式设为 `ON`，否则 `src/tst` 不会被加入构建。

---

## 选项交互关系

### 依赖关系

```mermaid
flowchart TD
    D["DA_ENABLE_AUTO_TRANSLATE"] -->|ON 时需要| E["Qt Linguist (lrelease)"]
    U["update_translations 目标"] -->|手动触发需要| G["Qt Linguist (lupdate)"]
```

!!! info "update_translations 是 CMake 目标而非选项"
    `lupdate` 的触发已从 CMake 选项改为自定义目标 `update_translations`，不加入 `ALL`，避免每次构建重写 `.ts` 文件。仅在新增 `tr()` 字符串后手动构建该目标。

### 冲突与约束

| 组合 | 结果 |
|------|------|
| `DA_ENABLE_AUTO_TRANSLATE=ON` 但无 Qt Linguist | 构建报错，找不到 `lrelease` |
| 手动构建 `update_translations` 目标但无 Qt Linguist | 构建报错，找不到 `lupdate` |

---

## 常见构建场景

### 场景 1：完整开发构建（推荐）

```bash
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64" \
  -DDA_ENABLE_AUTO_INSTALL_PYTHON_ENV=ON \
  -DDA_ENABLE_AUTO_TRANSLATE=ON \
  -DDA_BUILD_PLUGINS=ON
```

### 场景 2：快速构建（跳过翻译和插件）

```bash
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64" \
  -DDA_ENABLE_AUTO_TRANSLATE=OFF \
  -DDA_BUILD_PLUGINS=OFF
```

### 场景 3：翻译更新

新增 `tr()` 字符串后，通过 `update_translations` 目标手动更新 `.ts` 文件，再正常构建以编译为 `.qm`：

```bash
# 配置项目（保持 DA_ENABLE_AUTO_TRANSLATE=ON 以编译 .ts -> .qm）
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64" \
  -DDA_ENABLE_AUTO_TRANSLATE=ON

# 手动更新 .ts 翻译源文件（提取新增的 tr() 字符串）
cmake --build build --target update_translations

# 正常构建会编译 .ts -> .qm 并部署
cmake --build build --config Release --parallel
```

---

## 相关文档

- [构建说明](./build-instructions.md) — 完整构建流程
- [第三方库构建](./third-party-build.md) — 第三方库编译
- [主程序构建](./main-program-build.md) — 主程序构建详解
- [Python 环境配置](./python-environment.md) — Python 环境说明
- [构建常见错误](./common-build-errors.md) — 构建问题排查
- [工程组织](../large-cmake-project-guide.md) — CMake 工程组织指南
