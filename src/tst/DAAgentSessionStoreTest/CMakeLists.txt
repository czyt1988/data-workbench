# DAAgentSessionStoreTest 的 CMakeLists.txt
# 测试 DAAgent 库的 DAAgentSessionStore 持久化层（JSONL 读写 / 索引 / 清理 / last_active）
#
# 与 DADataFrameTest 的差异：本测试只测 C++ 逻辑，不调 Python，
# 故不链接 DAPyScripts/DAPyBindQt/Python3/pybind11，只链接 DAWorkbench::DAAgent。
# DAAgent 是 SHARED 库，exe 运行时需 DAAgent.dll 及其传递依赖 DLL，
# damacro_setup_test 把 exe 输出到 ${CMAKE_BINARY_DIR}/bin（与 DA DLL 同目录）并
# 经 windeployqt 部署 Qt DLL（DEPLOY_TARGETS DAWorkbench::DAAgent 让其扫描 DAAgent.dll 的 Qt 依赖）。

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖（DAAgent 经 DAInterface 传递依赖
# DAMessageHandler，运行时需 spdlog.dll）。测试目标不可直接引用，此处 find_package
# 仅为让 COPY_DLL_TARGETS 能解析到 spdlog 的 DLL 路径（仿 MessageHandle 测试）。
find_package(spdlog CONFIG QUIET)

add_executable(DAAgentSessionStoreTest main.cpp)

target_link_libraries(DAAgentSessionStoreTest PRIVATE
    DAWorkbench::DAAgent
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

# 显式补齐 DAAgentSessionStore.h 传递依赖的头文件路径（DADir.h / DAGlobals.h）。
# DAAgent 的 PUBLIC include 理论上已传递，但仿 DADataFrameTest 显式声明更稳健。
target_include_directories(DAAgentSessionStoreTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/DAAgent
    ${CMAKE_SOURCE_DIR}/src/DAUtils
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(DAAgentSessionStoreTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

# 部署 Qt DLLs 和 DAAgent.dll 依赖（仿 DADataFrameTest / MessageHandle 测试）。
# windeployqt 只扫描直接传入文件的 Qt 依赖，不递归扫描同目录的 DA DLL 的二次 Qt 依赖，
# 故须把携带 Qt 依赖的 DA DLL 都显式传入：
#   - DAInterface → Qt5Xml（PRIVATE 链接）
#   - DAGui → Qt5Network / Qt5WebChannel / Qt5WebEngineWidgets（WebEngine UI 层）
# 否则测试 exe 启动报 0xC0000135（DLL not found）。DAAgent 是 SHARED 库亦需传入。
# COPY_DLL_TARGETS spdlog::spdlog：DAMessageHandler（经 DAInterface 传递依赖）运行时需
# spdlog.dll，须显式复制（damacro 只复制 SHARED_LIBRARY 类型的 imported target）。
damacro_setup_test(DAAgentSessionStoreTest
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAAgent DAWorkbench::DAInterface DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)

# 添加测试
add_test(NAME DAAgentSessionStoreTest COMMAND DAAgentSessionStoreTest)

# 将测试可执行文件添加到全局变量，以便在顶级测试目录中使用
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} DAAgentSessionStoreTest PARENT_SCOPE)

# 添加运行此特定测试的自定义目标
add_custom_target(run_DAAgentSessionStoreTest
    COMMAND $<TARGET_FILE:DAAgentSessionStoreTest>
    DEPENDS DAAgentSessionStoreTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:DAAgentSessionStoreTest>
    COMMENT "Running DAAgentSessionStoreTest"
)
