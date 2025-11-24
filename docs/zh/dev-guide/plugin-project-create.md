# 创建插件项目

`data-workbench`作为一个基础框架，业务功能应该由插件提供

## 插件项目结构组织方式

一个插件的工程结构应该如下所示：

```txt
[插件工程文件夹]
    |-CMakeLists.txt    # 插件的CMakeLists.txt
    |-[src]             # 插件的代码目录
    |-[data-workbench]  # data-workbench工程目录
```

如果你用`git`来管理，`data-workbench`应该作为你的插件项目的子项目，这是最推荐的做法

建议在你项目的根目录执行如下操作，即可添加`data-workbench`作为你项目的子模块

```bash
git submodule add https://gitee.com/czyt1988/data-workbench.git ./data-workbench
```

首次拉取插件项目，需要执行：

```shell
git submodule update --init --recursive
```

执行上面语句会把`data-workbench`以及它所有的第三方依赖都拉取

## 插件项目的CMake文件

针对上面的目录结构，你的插件项目需要写两个`CMakeLists.txt`文件，一个是顶层目录的组织，一个是`src`文件夹的组织

### 顶层目录CMake文件

顶层目录的`CMakeLists.txt`文件用于指定`data-workbench`目录记忆相关信息

你的顶层目录`CMakeLists.txt`文件需要有如下内容：

```cmake
# 确认系统的位数，data-workbench的安装目录会进行区分，因此需要此拼接data-workbench的安装目录
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(_platform_name "x86")
else()
    set(_platform_name "x64")
endif()

# Qt依赖
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

# 指定data-workbench安装文件夹目录名
set(DAWorkbench_INSTALL_FOLDER_NAME bin_${CMAKE_BUILD_TYPE}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${_platform_name})

# 指定data-workbench安装目录
set(DAWorkbench_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/data-workbench/${DAWorkbench_INSTALL_FOLDER_NAME}")

# 指定data-workbench的cmake文件搜索目录
set(DAWorkbench_DIR "${DAWorkbench_INSTALL_DIR}/lib/cmake/DAWorkbench")

# 引入daworkbench_plugin_utils辅助工具
include(${DAWorkbench_DIR}/daworkbench_plugin_utils.cmake)
```

有了上面内容，你就可以方便的引入`data-workbench`工程

### src目录CMake文件

src目录主要是你的代码，src目录由顶层目录通过`add_subdirectory(src)`添加

`src`目录的cmake文件会继承顶层目录的变量，因此`DAWorkbench_INSTALL_FOLDER_NAME`、`DAWorkbench_INSTALL_DIR`、`DAWorkbench_DIR`等变量都可以直接使用

`data-workbench`提供了一些宏，能让你快速组织一个插件项目，在顶层目录的`CMake`文件中已经引入了`daworkbench_plugin_utils.cmake`文件，`data-workbench`的插件辅助宏都在此文件里面

#### damacro_plugin_setting

插件设置宏

此宏如下定义：

```cmake
macro(damacro_plugin_setting _plugin_name _plugin_description _plugin_ver_major _plugin_ver_minor _plugin_ver_path _daworkbench_intall_dir)
```

输入参数：

- `_plugin_name` lib的名字，决定变量`DA_PLUGIN_NAME`
- `_plugin_description` lib的描述，决定变量`DA_PLUGIN_DESCRIPTION`
- `_plugin_ver_major` lib的主版本号，决定变量`DA_PLUGIN_VERSION_MAJOR`
- `_plugin_ver_minor` lib的次版本号，决定变量`DA_PLUGIN_VERSION_MINOR`
- `_plugin_ver_path` lib的末版本号，决定变量`DA_PLUGIN_VERSION_PATCH`

这个宏会生成下面这些变量：

- `DA_PLUGIN_NAME`: 项目名称，由第一个参数决定
- `DA_PLUGIN_VERSION_MAJOR`：主要版本号由`_plugin_ver_major`决定
- `DA_PLUGIN_VERSION_MINOR`：小版本号由`_plugin_ver_minor`决定
- `DA_PLUGIN_VERSION_PATCH`：次版本号由`_plugin_ver_path`决定
- `DA_PLUGIN_VERSION`：完整的版本名，是上面三个版本号通过`.`进行拼接
- `DA_PLUGIN_FULL_DESCRIPTION`：完整的项目描述，由`${DA_PLUGIN_NAME} ${DA_PLUGIN_VERSION} | ${DA_PLUGIN_DESCRIPTION}`三个变量拼接
- `DA_MIN_QT_VERSION`：最低qt版本要求

src目录的CMake文件首先应该这写：

```cmake
cmake_minimum_required(VERSION 3.5)
# 定义插件的信息，自动设置安装位置
damacro_plugin_setting(
    "MyDAPlugin"
    "Plugin For DAWorkbench"
    0
    0
    1
    ${DAWorkbench_INSTALL_DIR}
)
```

上面这段语句，会自动设置好下面这些参数：

```
DA_PLUGIN_NAME = MyDAPlugin
DA_PLUGIN_VERSION_MAJOR = 0
DA_PLUGIN_VERSION_MINOR = 0
DA_PLUGIN_VERSION_PATCH = 1
DA_PLUGIN_VERSION = 0.0.1
DA_PLUGIN_FULL_DESCRIPTION = MyDAPlugin 0.0.0|Plugin For DAWorkbench
```

#### damacro_import_*

`damacro_import_*`相关的宏用于导入第三方库

如：`damacro_import_SARibbonBar`就是导入`SARibbonBar`到项目中

由于`damacro_plugin_setting`已经设置好了变量，你可以直接复制下面这段话到你的`src`目录`cmake`文件中而不需要进行改动

```cmake
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtPropertyBrowser(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_orderedmap(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
```

#### damacro_plugin_install

`damacro_plugin_install`宏用于插件的安装，无需任何参数，只需要在`src`目录`cmake`文件最后添加即可

## 插件项目CMake文件示例

一个较为完整的插件项目`CMake`文件示例如下

根目录`CMakeLists.txt`文件：

```cmake
cmake_minimum_required(VERSION 3.5)
project(MyDAPlugin
        DESCRIPTION "this cmake file is the top cmake file of MyDAPlugin"
)

########################################################
# 获取data-workbench安装目录名称
########################################################
set(DA_MIN_QT_VERSION 5.14)

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(_platform_name "x86")
else()
    set(_platform_name "x64")
endif()
# Qt依赖
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
# 指定data-workbench安装文件夹目录名
set(DAWorkbench_INSTALL_FOLDER_NAME bin_${CMAKE_BUILD_TYPE}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${_platform_name})
# 指定data-workbench安装目录
set(DAWorkbench_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/data-workbench/${DAWorkbench_INSTALL_FOLDER_NAME}")
# 指定data-workbench的cmake文件搜索目录
set(DAWorkbench_DIR "${DAWorkbench_INSTALL_DIR}/lib/cmake/DAWorkbench")

list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_INSTALL_DIR})
list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_DIR})
include(${DAWorkbench_DIR}/daworkbench_plugin_utils.cmake)
# 设置安装目录为data-workbench安装目录
set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})
# 添加src文件夹
add_subdirectory(src)

if(MSVC)
    # 为 MSVC 设置链接器标志以禁止生成清单文件
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
endif()
```

src目录的`CMakeLists.txt`文件：

```cmake
cmake_minimum_required(VERSION 3.5)
# 定义插件的信息，自动设置安装位置
damacro_plugin_setting(
    "MyDAPlugin"
    "Plugin For Gree BigData Workbench "
    0
    0
    1
    ${DAWorkbench_INSTALL_DIR}
)
########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Xml
    Svg
    PrintSupport
    AxContainer
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

add_library(${DA_PLUGIN_NAME} SHARED
            ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_QT_UI_FILES}
            ${DA_PLUGIN_QT_RC_FILES}
)	

########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::AxContainer
)

# 以下这些宏由daworkbench_plugin_utils.cmake提供，此文件会在DAWorkBench安装的时候一同安装到lib/cmake目录下
# 只需要把DAWorkbench的cmake文件安装目录(lib/cmake)append到CMAKE_MODULE_PATH即可调用
# list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_CMake_DIR})
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtPropertyBrowser(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_orderedmap(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
# -------------link DAWorkBench--------------------------
find_package(DAWorkbench COMPONENTS 
    DAUtils 
    DAMessageHandler
    DAAxOfficeWrapper
    DAData
    DAGraphicsView
    DAWorkFlow
    DAFigure
    DACommonWidgets 
    DAGui 
    DAInterface 
    DAPluginSupport
)

target_link_libraries(${DA_PLUGIN_NAME} PUBLIC
    DAWorkbench::DAUtils 
    DAWorkbench::DAMessageHandler
    DAWorkbench::DAAxOfficeWrapper
    DAWorkbench::DAData
    DAWorkbench::DAGraphicsView
    DAWorkbench::DAWorkFlow
    DAWorkbench::DAFigure
    DAWorkbench::DACommonWidgets 
    DAWorkbench::DAGui 
    DAWorkbench::DAInterface 
    DAWorkbench::DAPluginSupport
)
########################################################
# Qt的moc
########################################################
set_target_properties(${DA_PLUGIN_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_PLUGIN_VERSION}
    EXPORT_NAME ${DA_PLUGIN_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${DAWorkbench_INSTALL_DIR}/bin/plugins"
)

########################################################
# 安装插件
########################################################
damacro_plugin_install()
```

通过上面的操作，可以构建一个基于`data-workbench`的插件

## 工程文件生成

插件工程文件可通过`plugins/plugin-template/make-plugin.py`脚本文件生成

只需要配置`plugins/plugin-template/template.json`即可生成工程文件

配置文件格式如下：

```json
{
	"plugin-base-name":"My",
    "plugin-display-name":"My Plugin",
    "plugin-description":"This is My Plugin",
    "plugin-iid":"Plugin.MyPlugin",
    "factory-prototypes":"My.Factory",
    "factory-name":"My Factory",
    "factory-description":"My Plugin Node Factory"
}
```

配置后，运行`make-plugin.py`脚本，会在上级目录生成插件工程文件