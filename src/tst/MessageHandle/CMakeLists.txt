# MessageHandleTest 的 CMakeLists.txt
# 测试 DAMessageHandler 模块的日志功能

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../../DAMessageHandler
)

add_executable(MessageHandleTest main.cpp)

target_link_libraries(MessageHandleTest PRIVATE
    DAWorkbench::DAMessageHandler
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖，测试目标不可直接引用
# 此处 find_package 仅为让 COPY_DLL_TARGETS 能解析到 spdlog 的 DLL 路径
find_package(spdlog CONFIG QUIET)

# 设置输出目录到 build/bin（与 DA DLLs 同目录），部署 Qt DLLs，复制 spdlog DLL
# DEPLOY_TARGETS: 传给 windeployqt 扫描的 DA DLL，以发现 DAUtilsd.dll → Qt6Xml / Qt6Core5Compat 的传递依赖
damacro_setup_test(MessageHandleTest
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAMessageHandler DAWorkbench::DAUtils
    COPY_DLL_TARGETS spdlog::spdlog
)

# 添加测试
add_test(NAME MessageHandleTest COMMAND MessageHandleTest)

# 添加运行此特定测试的自定义目标
add_custom_target(run_MessageHandleTest
    COMMAND $<TARGET_FILE:MessageHandleTest>
    DEPENDS MessageHandleTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:MessageHandleTest>
    COMMENT "Running MessageHandleTest"
)
