# 基于cmake的大型工程组织和构建

在大型C++项目中，构建系统的选择直接影响到项目的可维护性、可扩展性以及第三方开发的友好度。一个成熟的工程不仅仅包含应用程序本身，还包括为其核心功能抽象出的库。这些库一方面服务于项目自身的模块化解耦，另一方面也可能作为插件化开发的基础库提供给第三方开发者。此外，项目还不可避免地依赖于众多第三方库。

一个典型的大型工程通常由以下几部分组成：

-   **第三方库：** 如 `OCCT`、`VTK`、`Qt` 等。
-   **自定义库：** 项目自身抽象出的功能模块。
-   **可执行程序：** 如 GUI 程序、命令行工具等。
-   **静态资源：** 脚本、图片、配置文件等。

针对这些大型的工程，如果用一些简单的构建工具，是很难做到一键编译一键安装的，例如 `qmake`，缺少强大的安装和依赖管理功能，这也就是为什么 Qt6 弃用 `qmake`，全面转向 `cmake`。目前来说，在C++领域，最适合进行构建管理的还是`cmake`，虽然`cmake` 有非常非常多的缺点，但它凭借强大的功能，依然是当前 C++ 领域构建管理的事实标准。

通过 `cmake`，我们可以实现：

-   **高效组织** 庞大且复杂的工程结构。
-   **自动化编译** 第三方依赖库。
-   **按依赖关系** 自动构建项目所有组件。
-   **一键式安装** 部署整个项目。
-   **生成** 便于第三方集成的插件开发环境。

本文将结合实践经验，深入探讨如何利用 `cmake` 组织和构建一个大型工业级软件项目，最终生成一个可供第三方开发者一键引入、便捷地进行二次开发的完整环境。同时，本文也介绍了通过`git submodule` 来方便管理第三方库。

## 工程的目录结构

一个清晰、标准的目录结构是大型工程良好管理的开端，工程的顶层文件夹应该包含如下几个文件夹：

- src 文件夹，这个文件夹用来放置你所有的源代码
- docs 文件夹，这个文件夹用来放置你所有的文档
- 3rdparty 文件夹，这个文件夹用来放置你所有的第三方库，这个文件夹可以放在 src 文件夹里面，也可以放在外层目录
- 针对整个工程的 `CMakeLists.txt` 文档
- cmake 文件夹，这个文件夹放置了一些封装好的 cmake 文件，用来方便你的 cmake 的集成

上面的这些文件夹和文件是一个工程比较通用的组织结构

一般而言，在工程的顶层目录下，还会有`.clang-format`用于规范编码，`.clang-tidy`和`.clazy`用于代码检查这些按需提供，但作为一个开源的项目，还是建议提供的

因此一个相对标准的源码目录如下所示：

```
MyProject/
├── .clang-format          # 代码格式化配置 (强烈建议有)
├── .clang-tidy            # 静态代码检查配置 (可选)
├── .clazy                 # Clang代码检查配置 (可选)
├── CMakeLists.txt         # 顶层CMake构建文件
├── 3rdparty/              # 第三方库源码 (也可置于 src 下)
├── cmake/                 # 项目自定义的 CMake 模块和工具脚本
├── docs/                  # 项目文档
└── src/                   # 项目自身源代码
    ├── CMakeLists.txt
    ├── LibA/              # 自定义库 A
    ├── LibB/              # 自定义库 B
    ├── App/               # 主应用程序
    └── ...
```

## 第三方库的管理

`3rdparty` 文件夹用来放置所有的第三方库的源代码，通常来讲，第三方库源代码不应该下载下来，放进 `3rdparty` 文件夹，而是通过 git 的 `submodule` 添加进去，通过 `submodule` 方式添加进去的源代码，可以随时更新到远程仓库上的最新版本，也可以指定这个第三方库是某个固定分支或者是某个 tag

例如我这里需要使用ribbon界面，添加了SARibbon作为第三方库

```shell
git submodule add https://github.com/czyt1988/SARibbon.git ./src/3rdparty/SARibbon
# 或者放在 ./3rdparty/SARibbon
```

> 注意，对于使用`submodule`管理第三方库的方式，首次拉取项目之后，需要执行：
> 
> ```shell
> git submodule update --init --recursive
> ```
> 
> 把所有库拉取下来
>
> 也可以clone的时候使用--recursive参数
> 
> ```shell
> git clone --recursive
> ```

大部分的第三方库都提供了 `cmake`，如果不提供的话，我会 fork 一个，写一个带有 `cmake` 的版本，例如 [qwt库](https://github.com/czyt1988/QWT)，[QtPropertyBroswer库](https://github.com/czyt1988/QtPropertyBrowser)，3rdparty 文件夹下会写一个 `cmake` 文件，用来集中编译所有的第三方库，一般我会在 `cmake` 中就指定安装目录，确保第三方库的安装目录和我的程序的安装目录是一致的，这样的好处是，如果你的程序需要给其他人进行二次开发的话，能保证你程序编译出来的库和第三方库是在一个安装环境下，这样可以解决第三方库和你自身程序库的依赖问题，不需要用户在编译你的程序之前先进行大量的第三方库的编译，只需要一次统一的编译即可把所有的第三方库安装到固定目录下,最后install后，形成一个完整的开发环境


连同第三方库一起发布的开发环境bin目录

![完整开发环境](../assets/PIC/cmake-after-install.png)

连同第三方库一起发布的开发环境lib目录
![完整开发环境2](../assets/PIC/cmake-after-install2.png)

作为第三方开发者，这个完整开发环境里面包含了所有的库，第三方开发者只需知道安装目录，就可以加载所有的依赖

下面就介绍一下，如何通过cmake实现这种大型工程的组织

## 大型工程的cmake写法

这里不会教你如何写cmake，而是着重讲讲大型工程的cmake要注意事项，工程顶层会有个`CMakeLists.txt`文件，这个文件定义了整个工程的信息、可选项、总体的安装步骤等，实现整个工程的构建，顶层的`CMakeLists.txt`通过`add_subdirectory`添加子目录，一般会添加src目录，以我自己的一个[仿真集成平台data-workbench举例](https://github.com/czyt1988/data-workbench)，介绍如何通过cmake组织一个大型的工程

> 上述的仿真集成平台不提供业务逻辑，所有业务逻辑都是通过插件实现，插件的开发就需要依赖此集成平台和所有第三方库

要驾驭大型工程的构建，必须深入理解和熟练运用 `cmake` 的 `install` 命令

`install` 命令的主要功能：
1.  **复制文件/目录** 到指定位置。
2.  **导出目标**，生成 `{库名}Targets.cmake` 文件，供其他 `cmake` 项目 `find_package`。
3.  **为当前项目** 的其他模块提供依赖支持。

`cmake` 强大的一个地方在于它能通过 `$<BUILD_INTERFACE:` 和 `$<INSTALL_INTERFACE:` 生成器表达式，优雅地区分 **构建环境**（源代码目录）和 **安装环境**（安装目录），确保头文件路径和依赖关系在不同场景下都能正确工作。

`cmake`的`install`用法是比较固定的，按照一个例子或者模板非常简单的就能实现自己的安装和部署，针对大型系统一个多组件的安装是必须的，类似于QT的包引入，能进行模块的划分，不需要整个QT所有库都一起引进工程里面，针对自己的大型系统也应该实现类似的引入，因此，下面将着重介绍如何进行模块化的`install`

### 规范的安装路径

使用规范的安装路径，能让你工程的库以及第三方库安装在同一个目录下，这样你的工程就很容易被第三方使用者集成起来进行二次开发，因此，安装路径尽量使用规范化的安装路径，而不是过于自由的进行定制，一般规范化的安装路径如下：

-   `bin/`: 存放可执行文件和 Windows 下的 DLL 文件。
-   `lib/`: 存放静态库（.a, .lib）和动态库的导入库（.lib）。
-   `lib/cmake/<ProjectName>/`: 存放项目的 CMake 配置文件（如 `*Config.cmake`, `*Targets.cmake`）。
-   `include/<ProjectName>/`: 存放项目的公共头文件。

通常不建议在cmake里硬编码上诉路径，`GNUInstallDirs` 模块定义的标准路径

使用 `include(GNUInstallDirs)` 后，你可以使用 `CMAKE_INSTALL_BINDIR`、`CMAKE_INSTALL_LIBDIR`、`CMAKE_INSTALL_INCLUDEDIR` 等变量，确保路径的规范性。

下面是常见的cmake安装后的文件夹

![](../assets/PIC/cmake-standard-install-dir.png)

基本上大部分的第三方库都是按照这个目录结构进行安装，这样当你的工程包含了大量的第三方库，以及你自身的库的情况下，最终所有的dll都会安装在`bin`录下，所有的库文件都会安装在`lib`目录下，所有的头文件都会在`include`文件夹下面对应的自身库名的文件夹下面，所有`cmake`需要用的文件都在`lib/cmake`文件夹下对应的自身库名的文件夹下面

以这种标准化的形式构建，第三方开发者可以很方便的使用你的工程

这里举一个例子，假如你的库名叫`SARibbonBar`，那么它安装后在windows系统下应该生成如下结构

```
bin
  |-SARibbonBar.dll
include
  |-SARibbonBar
     |-SARibbonBar.h
     |-...所有头文件都在此文件夹下
lib
  |-SARibbonBar.lib
  |-cmake
    |-SARibbonBar
       |-SARibbonBarConfig.cmake
       |-SARibbonBarConfigVersion.cmake
       |-SARibbonBarTargets.cmake
       |-SARibbonBarTargets-debug.cmake
```

### 单模块库的 `install` 标准写法

如果你作为一个库开发者，这个库只有一个模块，那么写法相对固定，单一模块的install写法基本就是如下步骤：

#### 1.定义库名和版本

这里定义一些基本信息，后续的步骤可使用这些变量

```cmake
set(LIB_NAME MyLib)
set(LIB_VERSION_MAJOR 1)
set(LIB_VERSION_MINOR 0)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION "${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH}")
```

#### 2.配置目标属性（关键！）

使用 `target_include_directories` 并利用生成器表达式区分构建和安装环境。

```cmake
add_library(${LIB_NAME} ...)
target_include_directories(${LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  # 构建时：源码中的 include 目录
    $<INSTALL_INTERFACE:include/${LIB_NAME}>                # 安装后：相对于安装前缀的 include/MyLib 目录
)
target_compile_definitions(${LIB_NAME} PUBLIC ...) # 如果有公共宏定义，也用 target_ 形式
```

`target_include_directories`和`target_compile_definitions`这两个是cmake的核心函数，它告诉了cmake这个目标有哪些头文件和哪些预定义宏，并把信息传递给使用库的人

#### 3.安装公共头文件

通过install可以复制任意内容，把你要提供的头文件、甚至脚本、资源都移动到指定安装目录下

```cmake
set(PUBLIC_HEADERS ...) # 列出所有公共头文件
install(FILES ${PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME} # 安装到 include/MyLib
)
```

#### 4.安装目标并导出（关键！）

`EXPORT` 关键字将目标的信息保存到一个名为 `${LIB_NAME}Targets` 的导出集中。

```cmake
install(TARGETS ${LIB_NAME}
    EXPORT ${LIB_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # bin目录，主要为DLL 文件
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # lib目录，主要为共享库 (.so, .dylib)
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
)
```

> 这里不得不吐槽cmake，把install命令赋予了太多功能，导致理解困难

#### 5.生成 `Config.cmake` 文件

这里比较抽象但必不可少，会用到`write_basic_package_version_file`和`configure_package_config_file`两个函数，用于生成find_package所必须的Config.cmake文件

首先，创建一个模板文件 `${LIB_NAME}Config.cmake.in`，一般内容可如下：

```cmake
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/@LIB_NAME@Targets.cmake")

set_and_check(@LIB_NAME@_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/@CMAKE_INSTALL_INCLUDEDIR@/@LIB_NAME@")
set_and_check(@LIB_NAME@_LIBRARY_DIR "${PACKAGE_PREFIX_DIR}/@CMAKE_INSTALL_LIBDIR@")

check_required_components(@LIB_NAME@)
```

上面的${LIB_NAME}Config.cmake.in是你为了生成Config.cmake文件使用的内嵌文件，具体位置视情况而定

然后，在主 `CMakeLists.txt` 中使用 `CMakePackageConfigHelpers` 模块生成最终文件，这里比较抽象但写法固定。

```cmake
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${LIB_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR  # 传递路径变量供 set_and_check 使用
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}ConfigVersion.cmake"
    VERSION ${LIB_VERSION}
    COMPATIBILITY SameMajorVersion
)
```

#### 6.安装生成的 CMake 文件

上面的文件会在编译过程生成在${CMAKE_CURRENT_BINARY_DIR}目录下面，你要在安装过程中把这个文件复制到lib/cmake/你的库名的配置目录下，通常写法如下：

```cmake
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}
)
install(EXPORT ${LIB_NAME}Targets
    FILE ${LIB_NAME}Targets.cmake
    NAMESPACE ${LIB_NAME}::                     # 可选，添加命名空间
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}
)
```

----

完成以上步骤后，其他项目就可以通过简单的 `find_package(MyLib)` 来使用你的库了。

使用这个库仅仅需要以下步骤：

```cmake
set(${YOUR_LIB_NAME}_DIR "your-lib-install-dir/lib/cmake")
find_package(${YOUR_LIB_NAME})
```

### 多模块的install写法

当一个项目包含多个库（如 Qt 的 Core、Gui、Widgets）时，我们需要将所有模块的导出信息合并到一个总的导出目标中，并提供一个顶层的 `Config.cmake` 文件

Qt就是一个多模块的例子，Qt模块的引入是这样写的：

```cmake
find_package(QT NAMES Qt6 Qt5 COMPONENTS 
    Core
    Gui
    Widgets
)
```

多模块和单模块的区别就是导出这一步(`install(TARGETS xx EXPORT xxx ...)`)，多模块在每个模块的安装导出需要导出到同一个目标中，每个模块不需要再调用`write_basic_package_version_file`和`configure_package_config_file`

多模块的文件组织示例如下

```
root
├CMakeLists.txt
├src
│├─CMakeLists.txt
│├─module-1
││ └─CMakeLists.txt
│├─module-2
││ └─CMakeLists.txt
│...
│└─module-n
│   └─CMakeLists.txt
└cmake
 └─{MyPackageName}Config.cmake.in
```

为了更好的组织大型项目，一般会在项目的根目录下创建一个cmake文件夹，常用的cmake文件会统一放在此目录下

多模块的install写法有如下步骤：

#### 1.顶层`CMakeLists.txt`写法

顶层的`CMakeLists.txt`里需要进行安装导出目标，它主要处理如下事情

-   定义总包名 `MyPackage` 和总导出目标名 `MyPackageTargets`。
-   负责生成和安装顶层的 `MyPackageConfig.cmake` 和 `MyPackageConfigVersion.cmake` 文件。
-   **不安装任何具体的目标**，只导出所有子模块累积到 `MyPackageTargets` 中的信息。

对于模块化的cmake，首先要有个总的进入文件，以MyPackage命名，像Qt5就叫Qt5Config.cmake，自己模块就叫{MyPackageName}Config.cmake

和单一模块类似，{MyPackageName}Config.cmake会通过{MyPackageName}Config.cmake.in模板生成，一个相对通用的写法如下：

```cmake
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/{MyPackageName}Targets.cmake")

set_and_check({MyPackageName}_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/@CMAKE_INSTALL_INCLUDEDIR@")
set_and_check({MyPackageName}_LIBRARY_DIR "${PACKAGE_PREFIX_DIR}/@CMAKE_INSTALL_LIBDIR@")

check_required_components({MyPackageName})
```

上面{MyPackageName}需要替换为你的包名

在顶层的`CMakeLists.txt`里需要进行安装导出目标，顶层`CMakeLists.txt`的安装写法如下

```cmake
set(MY_PACKAGE_PROJECT_NAME "MyPackageName")
# 这是所有模块的总targets，所有模块都向这个target导出
set(MY_PACKAGE_TARGET_NAME "MyPackageNameTargets")

# ... 添加子目录 add_subdirectory(src) ...
...

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${MY_PACKAGE_PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${DA_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/{MyPackageName}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${MY_PACKAGE_PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MY_PACKAGE_PROJECT_NAME}
)
#Unified export of all module targets
install(EXPORT ${DA_TARGET_NAME}
    FILE ${MY_PACKAGE_TARGET_NAME}.cmake
    NAMESPACE ${MY_PACKAGE_PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MY_PACKAGE_PROJECT_NAME}
)
```

顶层的`CMakeLists.txt`负责导出Config文件，后续所有模块的安装都往这个目标添加

#### 2.子模块`CMakeLists.txt`写法

多模块的子模块安装时不需要生成config文件，只需要将自己的目标**追加**到顶层定义的总导出集中（上诉例子的导出集名为MY_PACKAGE_TARGET_NAME）。

多模块的子模块安装示例

```
# src/ModuleA/CMakeLists.txt
add_library(ModuleA ...)
add_library(${MyPackageName}::ModuleA ALIAS ModuleA) # 推荐使用${MyPackageName}::ModuleA别名，这个别名可以在模块内部间方便调用

# 安装目标到总导出集 MY_PACKAGE_TARGET_NAME
install(TARGETS ${子模块名字}    # 这里是你这个库库的名字
        EXPORT ${MY_PACKAGE_TARGET_NAME}   # 注意：这里是顶层的导出集名称
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
```

MY_PACKAGE_TARGET_NAME是在顶层cmake定义的总的导出集，子模块的安装都导出到此导出集即可，这样就能像Qt一样，通过`find_package(MyPackageName COMPONENTS ModuleA ModuleB)`找到对应的库

## 工程的组织

至此，单模块和多模块的安装都已介绍完成，大型工程的组织和安装就是这两者的组合

工程各个模块安装到固定目录下，连同第三方库指定同一个安装路径，最终形成一个完整的开发环境

这里以实际例子举例，例子源码位于：

[github:https://github.com/czyt1988/data-workbench](https://github.com/czyt1988/data-workbench)

[gitee镜像:https://gitee.com/czyt1988/data-workbench](https://gitee.com/czyt1988/data-workbench)

源码目录结构(这里为了便于显示，文件夹用[]扩起)：

```
[root]
├[src]
│ ├─[3rdparty]
│ │ ├─[spdlog]
│ │ ...
│ │ ├─[SARibbon]
│ │ └─CMakeLists.txt(用于构建和安装第三方库)
│ ├─[DAUtils]
│ │ └─CMakeLists.txt
│ ├─[DAGui]
│ │ └─CMakeLists.txt
│ ...
│ ├─[APP]
│ │  └─CMakeLists.txt
│ └─CMakeLists.txt
├─CMakeLists.txt
└─[cmake]
     └─DAWorkbenchConfig.cmake.in(用于生成总包的Config.cmake文件)
```

1. 指定统一的安装目录

这一步可以使得第三方库和工程安装的位置一致，对于linux有比较规范的安装路径，但windows不一样，默认是在`C:\Program Files\xxx`这样的位置，没有统一放lib的地方，因此，windows下，个人习惯指定工程的自身目录下建立一个安装目录，以bin_{Debug/Release}_{x32/x64}的方式命名，如果有`Qt`，还会加上`Qt`的版本以作区分，如下所示：

```cmake
# 获取qt版本
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
# 平台判断
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(my_platform_name "x86")
else()
    set(my_platform_name "x64")
endif()
# 生成安装目录名称
set(my_install_dir_name bin_qt${QT_VERSION}_${CMAKE_BUILD_TYPE}_${my_platform_name})
# 设置固定的安装目录路径，具体位置具体设置，这里设置为当前cmake文件所在目录
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/${my_install_dir_name}")
```

!!! tips "提示"
    这样操作对库开发还有个好处，可以有效区分不同版本qt，不同编译器的结果

2. 第三方库

如前文所述，第三方库都在`src/3rdparty`下面，首先需要的是对第三方库的编译，`3rdparty`有个`CMakeLists.txt`文件夹用于编译安装所有第三方库，个人习惯不把`3rdparty`下的`CMakeLists.txt`纳入顶层工程的`subdirectory`中，因为不保证所有第三方库的cmake写的都正常，第三方库的`CMakeLists.txt`指定了`CMAKE_INSTALL_PREFIX`和顶层工程一致，确保安装路径一致

3. 组织顶层工程

顶层工程`CMakeLists`主要负责做以下事情：

- 定义`option`
- 定义工程名称
- 做全局的编译设置，如c++版本要求，编译环境的POSTFIX设置
- 通过`add_subdirectory`完成整个工程的组织
- 工程模块化的安装（见多模块的install写法）
- 工程的完整安装

## 第三方用户引入的方式

对于第三方插件开发者来说，首先需要clone你的工程，并进行编译，先编译第三方库，并进行安装（install），再编译工程，并进行安装（install）,这时候，第三方开发者就可以有一个完整的开发环境了

![完整开发环境2](../assets/PIC/cmake-after-install2.png)
