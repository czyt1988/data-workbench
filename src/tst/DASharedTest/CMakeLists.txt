# DASharedTest 的 CMakeLists.txt
# 这个文件管理 DAShared 的单元测试

da_add_test(
    NAME DASharedTest
    SOURCES main.cpp
    QT_COMPONENTS Test
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../DAShared
)
