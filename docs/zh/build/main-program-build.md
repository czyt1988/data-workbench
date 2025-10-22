# 构建`data-workbench`

本文介绍如何构建`data-workbench`，在构建`data-workbench`之前，请确认已经完成了第三方库的构建并执行了`install`命令，构建第三方库的方法见：[构建第三方库](./构建第三方库.md)

## 基于CMake构建data-workbench

这里介绍如何使用Qt Creator使用cmake构建主程序

### 1. 打开项目

打开Qt Creator，文件->打开文件或项目（`Ctrl+O`），选择`CMakeLists.txt`(dataworkbench顶层目录下的cmake)文件

![build-daworkbench-cmake-qtc](../../assets/PIC/build-daworkbench-cmake-qtc-01.png)

### 2. 选择构建模式

切换到项目模式（`Ctrl+5`）,Build步骤选择all，如果不安装，可不勾选install

![build-daworkbench-cmake-qtc](../../assets/PIC/build-daworkbench-cmake-qtc-02.png)

### 3. 设置第三方库的安装路径

!!! info "提示"
    如果你没有改动安装路径，那么这一步可省略

`data-workbench`的`./CMakeLists.txt`文件已经设置了第三方库路径，如果你在编译过程中没有调整第三方库的安装路径，那么在编译过程中你无需设置，如果调整了第三方库的安装路径，那么在编译过程中需要设置到你第三库的位置

```cmake
########################################################
# 定义第三方库路径
########################################################
set(SARibbonBar_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/SARibbonBar)
message(STATUS "SARibbonBar_DIR=${SARibbonBar_DIR}")

set(DALiteCtk_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/DALiteCtk)
message(STATUS "DALiteCtk_DIR=${DALiteCtk_DIR}")

set(qwt_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/qwt)
message(STATUS "qwt_DIR=${qwt_DIR}")

set(QtPropertyBrowser_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/QtPropertyBrowser)
message(STATUS "QtPropertyBrowser_DIR=${QtPropertyBrowser_DIR}")

set(spdlog_DIR ${DA_INSTALL_LIB_CMAKE_PATH}/spdlog)
message(STATUS "spdlog_DIR=${spdlog_DIR}")

# 注意tsl-ordered-map的安装位置在share/cmake,而不是lib/cmake
set(tsl-ordered-map_DIR ${DA_INSTALL_LIB_SHARE_PATH}/tsl-ordered-map)
message(STATUS "tsl-ordered-map_DIR=${tsl-ordered-map_DIR}")

set(qt${QT_VERSION_MAJOR}advanceddocking_DIR  ${DA_INSTALL_LIB_CMAKE_PATH}/qt${QT_VERSION_MAJOR}advanceddocking)
message(STATUS "qt${QT_VERSION_MAJOR}advanceddocking_DIR=${qt${QT_VERSION_MAJOR}advanceddocking_DIR}")
```

### 4. 编译和安装

点击运行（`Ctrl+R`）进行编译和安装

!!! tips "提示"
    编译完的首次运行会报错，因为第三方库的dll没有复制到`build`目录下，你需要手动把第三方库的dll复制构建目录下的bin文件夹中
    还有`zlib.dll`(有些会编译为`z.dll`)是zlib库，它是`quazip`的依赖，这个库也需要手动复制到build目录下，否则无法运行