# submodule

本项目的第三方库使用submodule进行管理

## submdule的拉取

首次拉取项目之后，需要执行：

```shell
git submodule update --init --recursive
```

对所有的submodule进行拉取

当然也可以逐个拉取：

```shell
git submodule update src/3rdparty/spdlog/spdlog
git submodule update src/3rdparty/SARibbon/SARibbon
git submodule update src/3rdparty/ADS/ADS
git submodule update src/3rdparty/pybind11/pybind11
```

## submodule的添加

第三方库统一放置到`/src/3rdparty/xxx`目录下，因此添加submodule如下：

- 注意3rdparty下的第三方库文件夹名字是两层，库名为libname，则目录是`/src/3rdparty/libname/libname`

添加2层目录目的是在第一层目录下面添加cmake文件，以便自动化编译

例如添加第三方库pybind11，先新建目录/src/3rdparty/pybind11

新建CMakeLists.txt文件，内容为：

```cmake
cmake_minimum_required(VERSION 3.5)

project(bilud-pybind11
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : 3rdparty build"
        )

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)


########################################################
# 第三方库 - pybind11
########################################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../../bin_qt${QT_VERSION}/build_3rdparty_lib/pybind11")
message(STATUS "3rdparty pybind11 will install to : ${CMAKE_INSTALL_PREFIX}")
# 安装pybind11
add_subdirectory(pybind11)
```

添加submodule:

DA添加submodule需要在src/3rdparty目录先建立linName文件夹，在linName文件夹下建立一个cmake文件，文件内容如下：

```cmake
cmake_minimum_required(VERSION 3.5)
project(bilud-{{linName}}
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : 3rdparty build"
        )

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)


########################################################
# 第三方库 - {{linName}}
########################################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../../bin_qt${QT_VERSION}/build_3rdparty_lib/{{linName}}")
message(STATUS "3rdparty pybind11 will install to : ${CMAKE_INSTALL_PREFIX}")
# 安装# {{linName}}
add_subdirectory({{linName}})
```

{{linName}}为占位符，改为第三方库名称即可

然后再git中添加，以pybind11举例

`git submodule add https://github.com/pybind/pybind11.git ./src/3rdparty/pybind11/pybind11`

- 注意最后路径是`./src/3rdparty/pybind11/pybind11`,库名称文件夹有两个

DA目前添加的submodule有如下：

```shell
git submodule add https://github.com/gabime/spdlog.git ./src/3rdparty/spdlog/spdlog
git submodule add https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git ./src/3rdparty/ADS/ADS
git submodule add https://github.com/czyt1988/SARibbon.git ./src/3rdparty/SARibbon/SARibbon
git submodule add https://github.com/pybind/pybind11.git ./src/3rdparty/pybind11/pybind11
git submodule add https://github.com/czyt1988/QtPropertyBrowser ./src/3rdparty/QtPropertyBrowser/QtPropertyBrowser
```


## submodule的更新

如果某个submodule更新了，使用`git submodule update --remote {submodule}`进行更新，如SARibbon项目更新了，可以执行：

```shell
git submodule update --remote src/3rdparty/SARibbon/SARibbon
```

## submodule的批量操作

单独进入某个submodule目录下执行git命令和普通的git仓库操作一样，也可以批量执行，如

`git submodule foreach '{需要执行的git命令}'`

如 git submodule foreach 'git checkout main'

# 针对中国地区访问github缓慢的问题

用编辑器打开

```
.git/config
```

把config的github地址替换为gitee的地址

github地址

```ini
[submodule "src/3rdparty/ADS/ADS"]
	active = true
	url = https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git
[submodule "src/3rdparty/SARibbon/SARibbon"]
	active = true
	url = https://github.com/czyt1988/SARibbon.git
[submodule "src/3rdparty/pybind11/pybind11"]
	active = true
	url = https://github.com/pybind/pybind11.git
[submodule "src/3rdparty/spdlog/spdlog"]
	active = true
	url = https://github.com/gabime/spdlog.git
```

gitee地址

```ini
[submodule "src/3rdparty/ADS/ADS"]
	active = true
	url = https://gitee.com/czyt1988/Qt-Advanced-Docking-System.git
[submodule "src/3rdparty/SARibbon/SARibbon"]
	active = true
	url = https://gitee.com/czyt1988/SARibbon.git
[submodule "src/3rdparty/pybind11/pybind11"]
	active = true
	url = https://gitee.com/czyt1988/pybind11.git
[submodule "src/3rdparty/spdlog/spdlog"]
	active = true
	url = https://gitee.com/czyt1988/spdlog.git
```

同时把根目录的`.gitmodules`用`.gitmodules-gitee`的内容替代