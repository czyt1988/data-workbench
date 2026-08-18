# 关键文件说明

本文档详细说明 DAWorkBench 项目中的关键文件及其用途，帮助开发者快速定位重要的配置和源文件。

## 主要功能特性

**特性**

- ✅ **根目录关键文件**：CMakeLists.txt、DAWorkbenchConfig.cmake.in、readme.md 等顶层文件
- ✅ **src 目录关键文件**：DAConfigs.h.in、DAGlobals.h 等源码核心文件
- ✅ **cmake 目录关键脚本**：daworkbench_utils.cmake 等构建辅助脚本
- ✅ **plugins 目录文件**：插件模板生成工具和配置文件
- ✅ **docs 目录文件**：文档首页、资源文件、样式文件
- ✅ **构建输出目录**：bin 目录结构和配置文件优先级
- ✅ **文件编辑注意事项**：可编辑和自动生成文件的区分

---

## 根目录关键文件

### CMakeLists.txt

**用途**：主构建配置文件，定义项目版本、编译选项、模块依赖和安装规则。

**关键配置项**：

下面的 CMake 示例展示了主配置文件的核心设置：

```cmake
# 版本定义 - 项目版本号设置
set(DA_VERSION_MAJOR 0)
set(DA_VERSION_MINOR 1)
set(DA_VERSION_PATCH 1)

# 编译选项 - 可自定义的构建开关
# Python 为强制依赖，始终参与构建，无开关
option(DA_ENABLE_AUTO_INSTALL_PYTHON_ENV "...自动部署 Python 环境" ON)  # 自动拷贝 Python DLL（Windows 推荐）
option(DA_ENABLE_AUTO_TRANSLATE "...自动调用 Linguist 翻译 ts" ON)        # 翻译文件自动处理
option(DA_AUTO_INSTALL_PREFIX "...自动安装到本地目录" ON)               # 自动安装路径
option(DA_AUTO_GENERATE_CONFIG_INFO "...自动生成 DAConfig.h" OFF)       # 仅库开发者需 ON，库使用者默认 OFF
option(DA_BUILD_PLUGINS "...构建 plugin" ON)                            # 插件构建开关
option(DA_ENABLE_TESTING "...运行测试" OFF)                             # 测试开关

# Qt 版本 - 最低 Qt 版本要求
set(DA_MIN_QT_VERSION 5.14)
```

上述配置项决定了项目的基本构建行为，可根据需求调整各选项。默认值汇总：`DA_ENABLE_AUTO_INSTALL_PYTHON_ENV=ON`、`DA_ENABLE_AUTO_TRANSLATE=ON`、`DA_AUTO_INSTALL_PREFIX=ON`、`DA_AUTO_GENERATE_CONFIG_INFO=OFF`、`DA_BUILD_PLUGINS=ON`、`DA_ENABLE_TESTING=OFF`。

**构建流程**：

1. 定义版本和项目信息
2. 配置编译选项
3. 查找 Qt 和第三方库
4. 设置模块路径
5. 添加子目录（各模块）
6. 配置安装规则

---

### DAWorkbenchConfig.cmake.in

**用途**：CMake 导出配置模板

**内容**：

```cmake
@PACKAGE_INIT@

find_dependency(Qt@QT_VERSION_MAJOR@ COMPONENTS Core Gui Widgets REQUIRED)
find_dependency(Qt@QT_VERSION_MAJOR@ COMPONENTS Xml Svg PrintSupport REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/DAWorkbenchTargets.cmake")

check_required_components(DAWorkbench)
```

**作用**：其他项目通过 `find_package(DAWorkbench)` 时加载此文件。

---

### readme.md

**用途**：项目说明文件

**内容**：

- 项目简介
- 软件设计目标
- 第三方库列表
- 编译说明
- 项目文档链接
- 界面截图

---

### mkdocs.yml

**用途**：MkDocs 文档配置

**关键配置**：

```yaml
site_name: DAWorkBench
theme:
  name: material
  language: zh
  
nav:
  - 概述: ...
  - 开发指南: ...
  - 插件开发: ...
  
markdown_extensions:
  - admonition
  - pymdownx.highlight
  - pymdownx.superfences
```

---

## src 目录关键文件

### DAConfigs.h.in

**用途**：配置头文件模板

**变量替换**：

```cpp
#define DA_VERSION_MAJOR @DA_VERSION_MAJOR@
#define DA_VERSION_MINOR @DA_VERSION_MINOR@
#define DA_VERSION_PATCH @DA_VERSION_PATCH@
#cmakedefine DA_VERSION "@DA_VERSION@"
#cmakedefine DA_COMPILE_DATETIME "@DA_COMPILE_DATETIME@"
#cmakedefine DA_PROJECT_NAME "@DA_PROJECT_NAME@"
```

**生成**：CMake 配置时生成 `DAConfigs.h`。

---

### DAGlobals.h

**用途**：全局定义和宏（PIMPL 模式宏、Qt5/6 兼容宏）。

!!! warning "版本宏不在 DAGlobals.h"
    `DAGlobals.h` **不**定义任何版本字符串（无 `DA_VERSION_STRING`）。项目版本宏（`DA_VERSION_MAJOR/MINOR/PATCH`、字符串形式的 `DA_VERSION`，当前 `"0.1.1"`）统一定义在下方**编译生成**的 `DAConfigs.h` 中。`DAGlobals.h` 在文件开头 `#include "DAConfigs.h"`，因此版本信息经此头文件传递可用。

**关键内容**：

```cpp
// PIMPL 前置声明与 d_ptr/q_ptr 管理
#define DA_IMPL_FORWARD_DECL(ClassName) class ClassName##Private;
#define DA_DECLARE_PRIVATE(classname) ...
#define DA_DECLARE_PUBLIC(classname) ...
#define DA_PIMPL_CONSTRUCT d_ptr(std::make_unique< PrivateData >(this))

// Qt5 / Qt6 事件坐标兼容宏
#define Qt5Qt6Compat_QXXEvent_Pos(valuePtr)   /* Qt5: pos() / Qt6: position().toPoint() */
#define Qt5Qt6Compat_QXXEvent_x(valuePtr)     /* Qt5: pos().x() / Qt6: position().x() */
#define Qt5Qt6Compat_QXXEvent_y(valuePtr)     /* Qt5: pos().y() / Qt6: position().y() */
```

日志便捷宏（`daInfo`/`daDebug`/`daWarning`/`daCritical` 流式宏）位于 `DAMessageHandler/DALogCategory.h`，不在 `DAGlobals.h` 中。

---

### DAConfigs.h

**用途**：由 CMake 根据 `DAConfigs.h.in` 模板**无条件生成**的配置头文件，承载项目版本与编译时间信息。

```cpp
#define DA_VERSION_MAJOR 0
#define DA_VERSION_MINOR 1
#define DA_VERSION_PATCH 1
#define DA_VERSION "0.1.1"          // 字符串形式版本号
#define DA_COMPILE_DATETIME "..."   // 编译日期时间
#define DA_PROJECT_NAME "DAWorkbench"
```

!!! note "不要手动编辑"
    `src/DAConfigs.h` 在每次 CMake 配置时由 `configure_file()` 重新生成，任何手工改动都会被覆盖；如需调整版本号，请修改根 `CMakeLists.txt` 中的 `DA_VERSION_MAJOR/MINOR/PATCH`。

---

### src/DAAgent/

**用途**：L4 接口层 Agent 框架模块（无 GUI 依赖）。

`src/DAAgent/` 是平台内置的 Agent 框架源码目录，承载多供应商 LLM 调用、提示词库、工具注册与持久化会话等能力。主要类包括 `DAAgentInterface`（对外接口契约）、`DAAgentBridge`、`DAAgentModule`、`DAAgentManager`、`DAAgentPrompt`、`DAAgentSessionStore`、`DAAgentToolBase`。Agent 工具的具体实现以插件形式存在于 `plugins/DAAgentTools/`。

!!! info "更多细节"
    Agent 框架的架构、生命周期与扩展方式见 [:octicons-copilot-24: Agent 开发指南](./dev-guide/agent/index.md)。

---

### 运行时配置文件

下列 INI 文件位于用户配置目录（`DA::DADir::getConfigPath()` 返回路径），由 `src/APP/main.cpp` 的 `migrateSettingsFromRegistry()` 在首次启动时从注册表迁移生成：

| 文件 | 说明 |
|------|------|
| `agent-config.ini` | Agent LLM 配置，`[agent]` 节包含 `llm_base_url`、`llm_model`、`llm_api_key`（DPAPI 加密的 QByteArray）、`ready_timeout_sec`、`stop_timeout_sec` |
| `recent-files.ini` | 最近打开的工程文件列表（`RecentFiles` 键） |

---

## cmake 目录关键文件

### daworkbench_utils.cmake

**用途**：构建辅助工具函数

**关键函数**：

```cmake
# 设置 bin 目录名称
function(dafun_set_bin_name output_var)

# 添加 DA 模块
function(da_add_module module_name)

# 安装配置
function(da_install_config_files)
```

---

### daworkbench_3rdparty.cmake

**用途**：第三方库配置

**关键内容**：

```cmake
# 第三方库路径设置
set(SARibbonBar_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/SARibbonBar)
set(qwt_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/qwt)
# ...
```

---

### daworkbench_plugin_utils.cmake

**用途**：插件构建辅助宏

**关键宏**：

```cmake
# 设置插件信息
macro(damacro_plugin_setting name desc major minor patch install_dir)

# 导入第三方库
macro(damacro_import_SARibbonBar target install_dir)
macro(damacro_import_qwt target install_dir)
# ...

# 安装插件
macro(damacro_plugin_install)
```

---

### create_win32_resource_version.cmake

**用途**：为 Windows（MSVC）目标生成包含版本信息与图标的资源文件（`.rc`）。

**关键函数**：

```cmake
# 生成 ${TARGET}_res.rc，写入 FILEVERSION/PRODUCTVERSION 等
function(create_win32_resource_version
         TARGET VERSION [COMPANY_NAME] [COPYRIGHT]
         [DESCRIPTION] [FILE_EXTENSION] [ICONS])
```

仅在 `MSVC` 下生效，用于让最终生成的 `DAWorkbench.exe` 在文件属性中显示版本号、公司、版权与图标。

---

## plugins 目录关键文件

### plugins/CMakeLists.txt

**用途**：插件构建配置

**内容**：添加子目录构建各个插件。

---

### plugins/plugin-template/make-plugin.py

**用途**：插件模板生成脚本

**使用**：

```bash
python make-plugin.py
```

**读取**：`template.json` 配置文件

**输出**：完整的插件项目结构

---

### plugins/plugin-template/template.json

**用途**：插件模板配置

**格式**：

```json
{
    "plugin-base-name": "My",
    "plugin-display-name": "My Plugin",
    "plugin-description": "Description",
    "plugin-iid": "Plugin.MyPlugin",
    "factory-prototypes": "My.Factory",
    "factory-name": "My Factory",
    "factory-description": "Factory Description"
}
```

---

## docs 目录关键文件

### docs/zh/index.md

**用途**：文档首页

**内容**：项目简介、编译说明、第三方库、界面截图。

---

### docs/assets/

**用途**：文档资源文件

**内容**：

- `PIC/` - 图片资源（UML 图、流程图）
- `screenshot/` - 界面截图
- `icon.png` - 文档图标
- `icon.ico` - favicon

---

### docs/stylesheets/extra.css

**用途**：文档自定义样式

---

## stubs 目录关键文件

**用途**：Python stubs 文件，用于 IDE 类型提示。

---

## i18n 目录关键文件

**用途**：国际化翻译文件（.ts 格式）

---

## 构建输出目录

### bin_{BuildType}_qt{QtVersion}_{Compiler}_{Platform}/

**结构**：

```text
bin_Release_qt5.15_MSVC_x64/
├── bin/
│   ├── DAWorkbench.exe    # 主程序
│   ├── plugins/           # 插件目录
│   │   ├── DataAnalysis.dll
│   │   └── *.dll
│   ├── *.dll              # 动态库
│   └── python/            # Python 环境
├── lib/
│   ├── cmake/             # CMake 配置
│   │   ├── DAWorkbench/
│   │   ├── SARibbonBar/
│   │   ├── qwt/
│   │   └── ...
│   ├── *.lib              # 静态库
├── share/
│   └── cmake/             # CMake 模块
└── include/               # 头文件（安装后）
```

---

## 配置文件优先级

| 配置文件 | 加载时机 | 优先级 |
|----------|----------|--------|
| 命令行参数 | 程序启动 | 最高 |
| 项目配置 (.dawproj) | 项目打开 | 高 |
| 全局配置 (settings.ini) | 程序启动 | 中 |
| 默认配置 | 代码内置 | 低 |

---

## 文件编辑注意事项

### 可编辑文件

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 可修改编译选项 |
| `readme.md` | 可更新项目说明 |
| `docs/**/*.md` | 可更新文档内容 |
| `src/**/*.h/cpp` | 可修改源代码 |

### 自动生成文件（勿手动编辑）

| 文件 | 生成方式 |
|------|----------|
| `src/DAConfigs.h` | CMake 配置生成 |
| `bin_*/` | 编译输出 |
| `doxygen/` | Doxygen 自动生成 |

---

## 下一步

- [:material-folder: 项目结构详解](./project-structure.md) - 目录结构说明
- [:material-tools: 构建说明](./build/build-instructions.md) - 构建流程
- [:material-file-cog: 配置说明](./configuration.md) - 配置文件详解
