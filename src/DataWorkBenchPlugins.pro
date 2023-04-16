include($$PWD/common.pri)
include($$PWD/function.pri)
# 运行此文件之前，先构建DataWorkFlow
TEMPLATE = subdirs
SUBDIRS += \
          $$PWD/DAPlugins/DAUtilNodePlugin \

OTHER_FILES += \
    $$PWD/../readme.md \
    $$PWD/../next-do.md

CONFIG   += ordered
