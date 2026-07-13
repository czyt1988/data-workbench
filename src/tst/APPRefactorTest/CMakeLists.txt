# APPRefactorTest 的 CMakeLists.txt

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Test REQUIRED)

add_executable(APPRefactorTest main.cpp)

target_include_directories(APPRefactorTest PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../APP
)

target_link_libraries(APPRefactorTest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(APPRefactorTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

add_test(NAME APPRefactorTest COMMAND APPRefactorTest)

set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} APPRefactorTest PARENT_SCOPE)

add_custom_target(run_APPRefactorTest
    COMMAND $<TARGET_FILE:APPRefactorTest>
    DEPENDS APPRefactorTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:APPRefactorTest>
    COMMENT "Running APPRefactorTest"
)
