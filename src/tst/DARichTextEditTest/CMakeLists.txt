da_add_test(
    NAME DARichTextEditTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAFigure
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAFigure
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAFigure
)
# 测试要按类型查找字体面板、背景色按钮与格式按钮，需要 DAWidgets 的头文件与导入库
da_link_3rdparty(DARichTextEditTest LIBS DAWidgets)
