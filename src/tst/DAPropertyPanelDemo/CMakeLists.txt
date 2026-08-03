# DAPropertyPanelDemo 的 CMakeLists.txt
# 独立 GUI 演示程序，展示 DAPropertyPanelWidget 全部属性类型

add_executable(DAPropertyPanelDemo main.cpp)

target_link_libraries(DAPropertyPanelDemo PRIVATE
    DAWorkbench::DAGui
    Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(DAPropertyPanelDemo PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)
