# Qwt3DRTTITest 的 CMakeLists.txt
# 验证 qwt3d 库的 RTTI 机制和值类型序列化

# 显式查找 qwt 包，获取 qwt::plot3d 目标
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME Qwt3DRTTITest
    SOURCES main.cpp
    QT_COMPONENTS Test
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        qwt::plot3d
    # qwt3d 头文件通过 find_package(qwt) 获取，此处补源码树中的 plot3d 头文件
    INCLUDE_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../3rdparty/qwt/src/plot3d
)
