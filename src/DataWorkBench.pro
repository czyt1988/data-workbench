include($$PWD/common.pri)
include($$PWD/function.pri)
# C++版本要求最低为17

system(mkdir $$saFixPath($${BIN_PLUGIN_DIR}))#创建插件路径
# 运行此文件之前，先构建第三方库3rdparty.pro
TEMPLATE = subdirs
SUBDIRS += \
          $$PWD/DAUtils \
          $$PWD/DAMessageHandler \
          $$PWD/DAPyBindQt \
          $$PWD/DAPyScripts \
          $$PWD/DAData \
          $$PWD/DACommonWidgets \
          $$PWD/DAGraphicsView \
          $$PWD/DAWorkFlow \
          $$PWD/DAFigure \
          $$PWD/DAPyCommonWidgets \
          $$PWD/DAGui \
          $$PWD/DAInterface \
          $$PWD/DAPluginSupport \
          $$PWD/APP


CONFIG   += ordered
CODECFORTR = utf-8

OTHER_FILES += ../next-do.md \
../readme.md

TRANSLATIONS += da_zh_CN.ts \
                da_en_US.ts
