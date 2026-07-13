# DASharedTest 的 CMakeLists.txt
# 这个文件管理 DAShared 的单元测试

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# 添加包含目录
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../../DAShared
)
# 创建测试可执行文件
add_executable(DASharedTest main.cpp)
# 设置目标属性
set_target_properties(DASharedTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

target_link_libraries(DASharedTest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

# 添加测试
add_test(NAME DASharedTest COMMAND DASharedTest)

# 将测试可执行文件添加到全局变量，以便在顶级测试目录中使用
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} DASharedTest PARENT_SCOPE)

# 添加一个运行此特定测试的自定义目标
add_custom_target(run_DASharedTest
    COMMAND $<TARGET_FILE:DASharedTest>
    DEPENDS DASharedTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:DASharedTest>
    COMMENT "Running DASharedTest"
)
