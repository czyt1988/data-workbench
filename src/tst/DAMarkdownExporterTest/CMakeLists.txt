# DAMarkdownExporterTest 的 CMakeLists.txt
# 测试 DAGui 的 DAMarkdownExporter（markdown 统一导出引擎）：
# 纯函数（图片路径归一化/data URI 内嵌/字体内联/Word src 转换）+
# WebEngine 回归（HTML/PDF 导出产物形态）。
#
# 导出引擎内部使用 QWebEnginePage（渲染壳从 qrc 加载），故链接 DAWorkbench::DAGui
# 并启用 WebEngine 组件；测试 exe 输出到 ${CMAKE_BINARY_DIR}/bin（与 DA DLL 同目录），
# 经 windeployqt 部署 Qt DLL（WebEngine 资源随依赖扫描部署）。
#
# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖（DAGui 传递依赖），
# 运行时需 spdlog.dll，find_package 仅为让 COPY_DLL_TARGETS 解析 DLL 路径
# （仿 DAAgentToolSpecTest）。

find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DAMarkdownExporterTest
    SOURCES main.cpp
    QT_COMPONENTS Test WebEngineWidgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)
