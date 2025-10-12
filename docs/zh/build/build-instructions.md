# 构建说明

`data-workbench`通过cmake来建立复杂的依赖，通过git submodule来管理第三方库，在构建`data-workbench`之前，需要构建第三方库

整个构建过程需要加载3次cmake文件

- 首先需要编译zlib库（如果你开发环境已经有zlib，可以跳过此步骤），用cmake打开`./src/3rdparty/zlib/CMakeLists.txt`,编译完成后需要执行安装命令`install`对zlib库进行安装

- 然后加载`./src/3rdparty/CMakeLists.txt`完成所有第三方库的编译，编译完成后需要执行安装命令`install`，否则第三步无法找到第三方库

- 最后是`./CMakeLists.txt`完成`DataWorkbench`编译，编译完成后需要执行安装命令`install`

- 把zlib库的zd.dll手动复制到bin目录下

如果没有构建第三方库，直接打开`data-workbench`下的`CMakeLists.txt`将报错

!!! info "提示"
    第三方库中的quazip依赖zlib，因此需要先编译zlib库，并把zd.dll复制到bin目录下

第二部可以设置不构建plugin，如果不构建plugin，plugin板块可以单独构建，前提是前两步已经完成且安装好，单独构建插件需要运行`plugins/CMakeLists.txt`

## 第三方库拉取

!!! warning "注意"
    编译前请确保已经拉取了第三方库

由于`data-workbench`使用的是`git submodule`方式管理大部分第三方库，因此不能直接下载zip包，你需要克隆仓库，然后执行`git submodule update`

```shell
git submodule update --init --recursive
```

!!! tips "提示"
    第三方库`SARibbon`里面也是用`submodule`组织了第三方库`QWindowKit`,本项目的第三方库都在gitee托管，方便国内网络下载，但`QWindowKit`托管在github上，如果你的网络无法拉取，你可以在拉取第三方库的时候把`--recursive`参数取消，也就是执行下面语句即可：
    ```shell
    git submodule update --init
    ```

    关于SARibbon的说明，你可以参阅文档[https://czyt1988.github.io/SARibbon/zh/](https://czyt1988.github.io/SARibbon/zh/)

也可以逐个拉取第三方库：

```shell
git submodule update src/3rdparty/zlib
git submodule update src/3rdparty/quazip
git submodule update src/3rdparty/spdlog
git submodule update src/3rdparty/SARibbon
git submodule update src/3rdparty/ADS
git submodule update src/3rdparty/pybind11
git submodule update src/3rdparty/QtPropertyBrowser
git submodule update src/3rdparty/ordered-map
```


## bin目录

DA项目编译好的二进制文件统一生成到`bin{Debug/Release}_qt{$$QT_VERSION}_{MSVC/GNU}_{x64/x86}`目录下，如：使用qt5.14.2, msvc版本debug模式64位编译，将生成`bin_Debug_qt5.14.2_MSVC_x64`文件夹

用户也可以自定义安装路径，可以把`CMakeLists.txt`的`DA_AUTO_INSTALL_PREFIX`参数设置为OFF，那么将不会使用预设的安装路径