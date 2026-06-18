# APPRefactorTest 的 CMakeLists.txt

add_executable(APPRefactorTest main.cpp)

target_include_directories(APPRefactorTest PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../APP
)

target_link_libraries(APPRefactorTest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(APPRefactorTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(NAME APPRefactorTest COMMAND APPRefactorTest)

set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} APPRefactorTest PARENT_SCOPE)

add_custom_target(run_APPRefactorTest
    COMMAND $<TARGET_FILE:APPRefactorTest>
    DEPENDS APPRefactorTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running APPRefactorTest"
)
