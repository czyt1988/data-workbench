# Qwt3DRTTITest 的 CMakeLists.txt
# 验证 qwt3d 库的 RTTI 机制和值类型序列化

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# 显式查找 qwt 包，获取 qwt::plot3d 目标
find_package(qwt CONFIG REQUIRED)

# 添加包含目录（qwt3d 头文件通过 find_package(qwt) 获取）
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../../../3rdparty/qwt/src/plot3d
)

# 创建测试可执行文件
add_executable(Qwt3DRTTITest main.cpp)

# 设置目标属性
set_target_properties(Qwt3DRTTITest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

target_link_libraries(Qwt3DRTTITest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    qwt::plot3d
)

# 添加测试
add_test(NAME Qwt3DRTTITest COMMAND Qwt3DRTTITest)

# 将测试可执行文件添加到全局变量
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} Qwt3DRTTITest PARENT_SCOPE)

# 添加一个运行此特定测试的自定义目标
add_custom_target(run_Qwt3DRTTITest
    COMMAND $<TARGET_FILE:Qwt3DRTTITest>
    DEPENDS Qwt3DRTTITest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:Qwt3DRTTITest>
    COMMENT "Running Qwt3DRTTITest"
)
