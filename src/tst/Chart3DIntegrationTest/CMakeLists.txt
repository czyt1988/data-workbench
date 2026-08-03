# Chart3DIntegrationTest 的 CMakeLists.txt
# 端到端集成测试：验证 3D 绘图功能的创建、管理、序列化、undo/redo

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Test OpenGL REQUIRED)

add_executable(Chart3DIntegrationTest main.cpp)

target_include_directories(Chart3DIntegrationTest PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../APP
    ${CMAKE_CURRENT_LIST_DIR}/../../DAFigure
    ${CMAKE_CURRENT_LIST_DIR}/../../DAFigure/Models
    ${CMAKE_CURRENT_LIST_DIR}/../../DAGui
    ${CMAKE_CURRENT_LIST_DIR}/../../DAUtils
    ${CMAKE_CURRENT_LIST_DIR}/../../DAShared
)

target_link_libraries(Chart3DIntegrationTest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::OpenGL
    DAWorkbench::DAFigure
    DAWorkbench::DAGui
    DAWorkbench::DAUtils
)

# DAFigure 模块通过 damacro_import_qwt 以 PRIVATE 链接 qwt3d，头文件路径和库不传递到下游消费者。
# 测试直接 include qwt3d 头文件并使用 Qwt3D* 类型，必须显式导入 qwt3d 依赖。
# 直接 find_package + target_link_libraries，避免 macro 在测试子目录的兼容性问题。
find_package(qwt CONFIG REQUIRED)
target_link_libraries(Chart3DIntegrationTest PRIVATE qwt::plot qwt::plot3d)

set_target_properties(Chart3DIntegrationTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

add_test(NAME Chart3DIntegrationTest COMMAND Chart3DIntegrationTest)

set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} Chart3DIntegrationTest PARENT_SCOPE)

add_custom_target(run_Chart3DIntegrationTest
    COMMAND $<TARGET_FILE:Chart3DIntegrationTest> -o test_result.txt
    DEPENDS Chart3DIntegrationTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:Chart3DIntegrationTest>
    COMMENT "Running Chart3DIntegrationTest"
)
