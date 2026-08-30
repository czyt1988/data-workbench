# Chart3DIntegrationTest 的 CMakeLists.txt
# 端到端集成测试：验证 3D 绘图功能的创建、管理、序列化、undo/redo

# DAFigure 模块以 PRIVATE 链接 qwt，头文件路径和库不传递到下游消费者。
# 测试直接 include qwt3d 头文件并使用 Qwt3D* 类型，必须显式导入 qwt 依赖。
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME Chart3DIntegrationTest
    SOURCES main.cpp
    QT_COMPONENTS Core Widgets Test OpenGL
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::OpenGL
        DAWorkbench::DAFigure
        DAWorkbench::DAGui
        DAWorkbench::DAUtils
        qwt::plot
        qwt::plot3d
    INCLUDE_PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../APP
        ${CMAKE_CURRENT_LIST_DIR}/../../DAFigure
        ${CMAKE_CURRENT_LIST_DIR}/../../DAFigure/Models
        ${CMAKE_CURRENT_LIST_DIR}/../../DAGui
        ${CMAKE_CURRENT_LIST_DIR}/../../DAUtils
        ${CMAKE_CURRENT_LIST_DIR}/../../DAShared
)
