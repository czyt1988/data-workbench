# 概述

本文讲述如何构建`data-workbench`，`data-workbench`通过cmake来建立复杂的依赖，通过git submodule来管理第三方库，在构建`data-workbench`之前，需要构建第三方库

如果没有构建第三方库，直接打开`data-workbench`下的CMakeLists.txt将报错

# 构建第三方库

第三方库的cmake文件位于:

```txt
src\3rdparty\CMakeLists.txt
```

此文件不属于上级工程，构建第三方库直接用cmake打开此文件构建即可，这里面已经把第三方库的基本设置配置好，构建后需要进行安装（install），安装完成后会在工程目录下生成bin_qt{version}_{MSVC/GNU}_x{64/32}的安装目录，`data-workbench`需要依赖此目录

## 使用Qt Creator基于CMake构建所有第三方库

1. 打开Qt Creator，文件->打开文件或项目（`Ctrl+O`），选择`src\3rdparty\CMakeLists.txt`文件

![](./PIC/build-3rdparty-cmake-qtc-01.png)

2. 切换到项目模式（`Ctrl+5`）,Build步骤选择all和install

![](./PIC/build-3rdparty-cmake-qtc-02.png)

3. 点击运行（`Ctrl+R`）进行编译和安装

![](./PIC/build-3rdparty-cmake-qtc-03.png)

编译完成后你能看到bin_qt{version}_{MSVC/GNU}_x{64/32}的安装目录，假如是用qt6.4+msvc,将生成`bin_qt6.4.0_MSVC_x64`这样的目录

目录里是所有第三方库的必要内容

# 构建`data-workbench`

## 使用Qt Creator基于CMake构建data-workbench

1. 打开Qt Creator，文件->打开文件或项目（`Ctrl+O`），选择`CMakeLists.txt`文件

![](./PIC/build-daworkbench-cmake-qtc-01.png)

2. 切换到项目模式（`Ctrl+5`）,Build步骤选择all，如果不安装，可不勾选install

![](./PIC/build-daworkbench-cmake-qtc-02.png)

3. 设置第三方库的安装路径

`data-workbench`依赖的第三方库路径需要设置

```cmake
########################################################
# 定义第三方库路径
########################################################
set(SARibbonBar_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/SARibbonBar)
set(DALiteCtk_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/DALiteCtk)
set(qwt_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/qwt)
set(DAWorkbench_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/${DA_PROJECT_NAME})
set(qt${QT_VERSION_MAJOR}advanceddocking_DIR  ${DA_INSTALL_LIB_CMAKE_PATH}/qt${QT_VERSION_MAJOR}advanceddocking)
```

3. 点击运行（`Ctrl+R`）进行编译和安装