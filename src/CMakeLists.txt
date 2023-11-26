
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 源码目录Cmake文件

cmake_minimum_required(VERSION 3.5)

########################################################
# 加载库
########################################################
add_subdirectory(DAUtils)
add_subdirectory(DAAxOfficeWrapper)
add_dependencies(DAAxOfficeWrapper DAUtils)

#add_subdirectory(DAMessageHandler)
#add_subdirectory(DAPyBindQt)
#add_subdirectory(DAPyScripts)
#add_subdirectory(DAData)
#add_subdirectory(DACommonWidgets)
#add_subdirectory(DAGraphicsView)
#add_subdirectory(DAWorkFlow)
#add_subdirectory(DAFigure)
#add_subdirectory(DAPyCommonWidgets)
#add_subdirectory(DAGui)
#add_subdirectory(DAInterface)
#add_subdirectory(DAPluginSupport)
#add_subdirectory(APP)

########################################################
# install
########################################################
