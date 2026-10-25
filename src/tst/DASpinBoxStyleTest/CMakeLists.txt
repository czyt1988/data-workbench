# DASpinBoxStyleTest 的 CMakeLists.txt
# 验证 SpinBox 代理样式：上下按钮竖排、数值区保留宽度、其它控件不变

da_add_test(
    NAME DASpinBoxStyleTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAGui
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui
)
