
# damacro_import_SARibbonBar(${DA_LIB_NAME})
macro(damacro_import_SARibbonBar __target_name)
    find_package(SARibbonBar)
    if(SARibbonBar_FOUND)
        message(STATUS "  |-link SARibbonBar")
        message(STATUS "  | |-include dir:${SARibbonBar_INCLUDE_DIR}")
    endif()
    if(DEFINED DA_SRC_DIR)
        set(_src_dir ${DA_SRC_DIR})
    else()
        set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)
    endif()
    # target_include_directories(${__target_name} PUBLIC
    #     $<INSTALL_INTERFACE:include/SARibbonBar>
    #     $<INSTALL_INTERFACE:include/SARibbonBar/3rdparty/framelesshelper/include>
    #     $<BUILD_INTERFACE:${_src_dir}/3rdparty/SARibbon/src/SARibbonBar>
    #     $<BUILD_INTERFACE:${_src_dir}/3rdparty/SARibbon/src/SARibbonBar/3rdparty/framelesshelper/include>
    # )
    # 链接的第三方库
    target_link_libraries(${__target_name} PRIVATE
        SARibbonBar
    )
endmacro(damacro_import_SARibbonBar)


macro(damacro_import_DALiteCtk __target_name)
    find_package(DALiteCtk)
    if(DALiteCtk_FOUND)
        message(STATUS "  |-link DALiteCtk")
        message(STATUS "  | |-include dir:${DALiteCtk_INCLUDE_DIR}")
    endif()
    if(DEFINED DA_SRC_DIR)
        set(_src_dir ${DA_SRC_DIR})
    else()
        set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)
    endif()
    # target_include_directories(${__target_name} PUBLIC
    #     $<INSTALL_INTERFACE:include/DALiteCtk>
    #     $<BUILD_INTERFACE:${_src_dir}/3rdparty/ctk>
    # )
    # 链接的第三方库
    target_link_libraries(${__target_name} PRIVATE
        DALiteCtk
    )
endmacro(damacro_import_DALiteCtk)


macro(damacro_import_QtAdvancedDocking __target_name)
    find_package(qt${QT_VERSION_MAJOR}advanceddocking)
    if(qt${QT_VERSION_MAJOR}advanceddocking_FOUND)
        message(STATUS "  |-link qt${QT_VERSION_MAJOR}advanceddocking")
        message(STATUS "  | |-include dir:${qt${QT_VERSION_MAJOR}advanceddocking_INCLUDE_DIR}")
    endif()
    if(DEFINED DA_SRC_DIR)
        set(_src_dir ${DA_SRC_DIR})
    else()
        set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)
    endif()
    # target_include_directories(${__target_name} PUBLIC
    #     $<INSTALL_INTERFACE:include/qt${QT_VERSION_MAJOR}advanceddocking>
    #     $<BUILD_INTERFACE:${_src_dir}/3rdparty/ADS/src>
    # )
    target_link_libraries(${__target_name} PRIVATE
        ads::qt${QT_VERSION_MAJOR}advanceddocking
    )
endmacro(damacro_import_QtAdvancedDocking)


macro(damacro_import_qwt __target_name)
    # 3rdparty - qwt
    find_package(qwt)
    if(qwt_FOUND)
        message(STATUS "  |-link qwt")
        message(STATUS "  | |-include dir:${qwt_INCLUDE_DIR}")
    endif()
    if(DEFINED DA_SRC_DIR)
        set(_src_dir ${DA_SRC_DIR})
    else()
        set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)
    endif()
    # target_include_directories(${__target_name} PUBLIC
    #         $<INSTALL_INTERFACE:include/qwt>
    #         $<BUILD_INTERFACE:${_src_dir}/3rdparty/qwt/src>
    # )
    # 链接的第三方库
    target_link_libraries(${__target_name} PRIVATE
        qwt
    )
endmacro(damacro_import_qwt)


macro(damacro_import_QtPropertyBrowser __target_name)
    # 3rdparty - qwt
    find_package(QtPropertyBrowser)
    if(QtPropertyBrowser_FOUND)
        message(STATUS "  |-link QtPropertyBrowser")
        message(STATUS "  | |-include dir:${QtPropertyBrowser_INCLUDE_DIR}")
    endif()
    if(DEFINED DA_SRC_DIR)
        set(_src_dir ${DA_SRC_DIR})
    else()
        set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)
    endif()
    # target_include_directories(${__target_name} PUBLIC
    #         $<INSTALL_INTERFACE:include/qwt>
    #         $<BUILD_INTERFACE:${_src_dir}/3rdparty/qwt/src>
    # )
    # 链接的第三方库
    target_link_libraries(${__target_name} PRIVATE
        QtPropertyBrowser
    )
endmacro(damacro_import_QtPropertyBrowser)

macro(damacro_import_spdlog __target_name)
    if(DEFINED DA_SRC_DIR)
        set(_src_dir ${DA_SRC_DIR})
    else()
        set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)
    endif()
    # spdlog是header only
    target_include_directories(${__target_name} PUBLIC
        $<INSTALL_INTERFACE:include/spdlog>
        $<BUILD_INTERFACE:${_src_dir}/3rdparty/spdlog/include>
    )
    message(STATUS "  |-spdlog include: ${spdlog_DIR}/include")
endmacro(damacro_import_spdlog)

macro(damacro_import_Python __target_name)
    # Python
    # https://zhuanlan.zhihu.com/p/666367728
    # https://blog.csdn.net/weixin_40448140/article/details/112005184
    # 如果使用的是非系统目录下的 Python 可以通过指定 Python3_ROOT_DIR 改变查找路径
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    if(${Python3_FOUND})
        message(STATUS "  |-find python")
        message(STATUS "  | |-Python3_VERSION = ${Python3_VERSION}")
        message(STATUS "  | |-Python3_VERSION_MAJOR = ${Python3_VERSION_MAJOR}")
        message(STATUS "  | |-Python3_VERSION_MINOR = ${Python3_VERSION_MINOR}")
        message(STATUS "  | |-Python3_ROOT_DIR = ${Python3_ROOT_DIR}")
        message(STATUS "  | |-Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
        message(STATUS "  | |-Python3_LIBRARY_DIRS = ${Python3_LIBRARY_DIRS}")
        message(STATUS "  | |-Python3_LIBRARY = ${Python3_LIBRARY}")
        message(STATUS "  | |-Python3_LIBRARIES = ${Python3_LIBRARIES}")
        message(STATUS "  | |-Python3_RUNTIME_LIBRARY_DIRS = ${Python3_RUNTIME_LIBRARY_DIRS}")
        message(STATUS "  | |-Python3_EXECUTABLE = ${Python3_EXECUTABLE}")
        message(STATUS "  | |-Python3_COMPILER = ${Python3_COMPILER}")
        message(STATUS "  | |-Python3_STDARCH = ${Python3_STDARCH}")
        message(STATUS "  | |-Python3_STDLIB = ${Python3_STDLIB}")
    endif()
    target_link_libraries(${__target_name} PRIVATE ${Python3_LIBRARIES})
    target_include_directories(${__target_name} PRIVATE ${Python3_INCLUDE_DIRS})
endmacro(damacro_import_Python)

macro(damacro_import_pybind11 __target_name)
    # pybind11
    # pybind11是header only
    if(DEFINED DA_SRC_DIR)
        set(_src_dir ${DA_SRC_DIR})
    else()
        set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)
    endif()
    target_include_directories(${DA_LIB_NAME} PUBLIC
        $<INSTALL_INTERFACE:include/pybind11>
        $<BUILD_INTERFACE:${_src_dir}/3rdparty/pybind11/include>
    )
endmacro(damacro_import_pybind11)
