# DAAppConfigTest 的 CMakeLists.txt
# 测试应用配置对象 DAAppConfig（强类型字段 / JSON 读写 / 旧版 XML 一次性迁移 /
# 取值规范化 / 未知键保留 / 分节变更信号）。
#
# 配置类位于 APP 可执行目标中，这里直接编译其源文件（配置类不依赖界面模块），
# 链接 DAUtils（DADir）与 DAMessageHandler（da* 日志宏）。

find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DAAppConfigTest
    SOURCES main.cpp
            ${CMAKE_SOURCE_DIR}/src/APP/SettingPages/DAAppConfig.cpp
    QT_COMPONENTS Test Xml
    LINK_PRIVATE
        DAWorkbench::DAUtils
        DAWorkbench::DAMessageHandler
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Xml
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/APP/SettingPages
        ${CMAKE_SOURCE_DIR}/src/DAMessageHandler
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAUtils DAWorkbench::DAMessageHandler
    COPY_DLL_TARGETS spdlog::spdlog
)
