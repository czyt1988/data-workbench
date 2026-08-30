
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 源码目录Cmake文件

cmake_minimum_required(VERSION 3.16)


########################################################
# 自动化生成配置头文件
# 注意:此处必须无条件执行 configure_file，保证 DAConfigs.h 始终由
# DAConfigs.h.in 结合顶层 CMakeLists.txt 中的 DA_VERSION_* 变量生成，
# 从而让启动画面/关于对话框/--version 显示的版本号与 CMakeLists.txt 保持一致。
# （历史上此处曾以 DA_AUTO_UPDATE_CONFIG_INFO 作为开关，但该变量名与顶层
#   option(DA_AUTO_GENERATE_CONFIG_INFO) 不一致，导致生成从未被触发，
#   仓库中 DAConfigs.h 长期为陈旧版本。）
########################################################
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/DAConfigs.h.in"
    "${CMAKE_CURRENT_LIST_DIR}/DAConfigs.h"
)

########################################################
# 加载库
# 模块间依赖由各模块 target_link_libraries 表达，无需 add_dependencies
########################################################
add_subdirectory(DAShared)
add_subdirectory(DAUtils)

if(WIN32)
    # DAAxOfficeWrapper基于Axcontainer是windows独有的技术
    add_subdirectory(DAAxOfficeWrapper)
endif()

add_subdirectory(DAMessageHandler)
add_subdirectory(DAPyBindQt)
add_subdirectory(DAPyScripts)
add_subdirectory(DAPyCommonWidgets)
add_subdirectory(DAGraphicsView)
add_subdirectory(DAPyWorkFlow)
add_subdirectory(DAData)
add_subdirectory(DAFigure)
add_subdirectory(DAGui)
add_subdirectory(DAInterface)
add_subdirectory(DAAgent)
add_subdirectory(DAPluginSupport)
add_subdirectory(APP)
