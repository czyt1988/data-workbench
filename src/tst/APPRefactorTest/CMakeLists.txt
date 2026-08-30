# APPRefactorTest 的 CMakeLists.txt

da_add_test(
    NAME APPRefactorTest
    SOURCES main.cpp
    QT_COMPONENTS Core Widgets Test
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../APP
)
