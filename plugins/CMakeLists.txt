
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench Plugins 顶层Cmake文件
# 在运行次CMakeLists.txt之前，请确保DA-WorkBench已经进行了install，DA-WorkBench位于../CMakeLists.txt
# 请先单独编译第三方库后，安装，并把安装目录设置到和此安装目录一致

#3.16是qt6要求的最低版本
cmake_minimum_required(VERSION 3.12)

########################################################
# 版本定义
########################################################
set(DA_PLUGINS_VERSION_MAJOR 0)
set(DA_PLUGINS_VERSION_MINOR 0)
set(DA_PLUGINS_VERSION_PATCH 1)
set(DA_PLUGINS_VERSION "${DA_PLUGINS_VERSION_MAJOR}.${DA_PLUGINS_VERSION_MINOR}.${DA_PLUGINS_VERSION_PATCH}")
string(TIMESTAMP DA_COMPILE_DATETIME %y%m%d)
string(TIMESTAMP DA_COMPILE_DATETIME_YEAR %y)
string(TIMESTAMP DA_COMPILE_DATETIME_MONTH %m)
string(TIMESTAMP DA_COMPILE_DATETIME_DAY %d)
message(STATUS "DA Plugins Version is ${DA_PLUGINS_VERSION}")
########################################################
# 可选的变量
########################################################

########################################################
# 定义工程
########################################################
set(DA_PROJECT_NAME "DAWorkbench-Plugins")

project(${DA_PROJECT_NAME} 
        VERSION ${DA_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench Plugins"
        )

########################################################
# 子工程加载
########################################################
add_subdirectory(DataAnalysis)