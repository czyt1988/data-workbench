macro(damacro_import_x_x x_namespace x_libname __target_name)
    find_package(${x_libname})
    if(${x_libname}_FOUND)
        message(STATUS "  |-finded ${x_libname}")
    else()
        message(STATUS "  |-can not find ${x_libname}")
        if(DEFINED DA_INSTALL_LIB_CMAKE_PATH)
            set(_lib_dir ${DA_INSTALL_LIB_CMAKE_PATH}/${x_libname})
            message(STATUS "  |-try to find in ${_lib_dir}")
            find_package(${x_libname} PATHS ${_lib_dir})
        endif()
    endif()
    # 链接的第三方库
    if(${x_libname}_FOUND)
        target_link_libraries(${__target_name} PRIVATE
            ${x_namespace}::${x_libname}
        )
        message(STATUS "  |-link ${x_namespace}::${x_libname}")
    else()
        message(FATAL_ERROR "  can not find ${x_libname}")
    endif()
endmacro(damacro_import_x_x)

# 这个宏是引入share/cmake目录的库，例如tsl-ordered-map
macro(damacro_import_x_x_sharepath x_namespace x_libname __target_name)
    find_package(${x_libname})
    if(${x_libname}_FOUND)
        message(STATUS "  |-finded ${x_libname}")
    else()
        message(STATUS "  |-can not find ${x_libname}")
        if(DEFINED DA_INSTALL_LIB_SHARE_PATH)
            set(_lib_dir ${DA_INSTALL_LIB_SHARE_PATH}/${x_libname})
            message(STATUS "  |-try to find in ${_lib_dir}")
            find_package(${x_libname} PATHS ${_lib_dir})
        endif()
    endif()
    # 链接的第三方库
    if(${x_libname}_FOUND)
        target_link_libraries(${__target_name} PRIVATE
            ${x_namespace}::${x_libname}
        )
        message(STATUS "  |-link ${x_namespace}::${x_libname}")
    else()
        message(FATAL_ERROR "  can not find ${x_libname}")
    endif()
endmacro(damacro_import_x_x_sharepath)

macro(damacro_import_x x_libname __target_name)
    damacro_import_x_x(${x_libname} ${x_libname} ${__target_name})
endmacro(damacro_import_x)

# damacro_import_SARibbonBar(${DA_LIB_NAME})
#macro(damacro_import_SARibbonBar __target_name)
#    find_package(SARibbonBar)
#    if(SARibbonBar_FOUND)
#        message(STATUS "  |-finded SARibbonBar")
#        message(STATUS "  | |-include dir:${SARibbonBar_INCLUDE_DIR}")
#    else()
#        message(STATUS "  |-can not find SARibbonBar")
#        if(DEFINED DA_INSTALL_LIB_CMAKE_PATH)
#            set(_lib_dir ${DA_INSTALL_LIB_CMAKE_PATH}/SARibbonBar)
#            message(STATUS "  |-try to find in ${_lib_dir}")
#            find_package(SARibbonBar PATHS ${_lib_dir})
#        endif()
#    endif()
#    # 链接的第三方库
#    if(SARibbonBar_FOUND)
#        target_link_libraries(${__target_name} PRIVATE
#            SARibbonBar::SARibbonBar
#        )
#        message(STATUS "  |-link SARibbonBar::SARibbonBar")
#    else()
#        message(ERROR "  can not find SARibbonBar")
#    endif()
#endmacro(damacro_import_SARibbonBar)

macro(damacro_import_SARibbonBar __target_name)
    damacro_import_x(SARibbonBar ${__target_name})
endmacro(damacro_import_SARibbonBar)

macro(damacro_import_DALiteCtk __target_name)
    damacro_import_x(DALiteCtk ${__target_name})
endmacro(damacro_import_DALiteCtk)

macro(damacro_import_QtAdvancedDocking __target_name)
    set(_lib_name qt${QT_VERSION_MAJOR}advanceddocking)
    damacro_import_x_x(ads ${_lib_name} ${__target_name})
endmacro(damacro_import_QtAdvancedDocking)

macro(damacro_import_qwt __target_name)
    damacro_import_x(qwt ${__target_name})
endmacro(damacro_import_qwt)

macro(damacro_import_QtPropertyBrowser __target_name)
    damacro_import_x(QtPropertyBrowser ${__target_name})
endmacro(damacro_import_QtPropertyBrowser)

macro(damacro_import_spdlog __target_name)
    damacro_import_x(spdlog ${__target_name})
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

macro(damacro_import_orderedmap __target_name)
    # tsl-ordered-map的安装位置不是在lib/cmake
    # 而是在share/cmake下面
    # tsl-ordered-map无法使用damacro_import_x_x_sharepath或者damacro_import_x_x这些宏
    # 因为他的package名称为tsl-ordered-map，他的库名称为ordered_map
    find_package(tsl-ordered-map)
    if(tsl-ordered-map_FOUND)
        message(STATUS "  |-finded tsl-ordered-map")
    else()
        message(STATUS "  |-can not find tsl-ordered-map")
        if(DEFINED DA_INSTALL_LIB_SHARE_PATH)
            set(_lib_dir ${DA_INSTALL_LIB_SHARE_PATH}/tsl-ordered-map)
            message(STATUS "  |-try to find in ${_lib_dir}")
            find_package(tsl-ordered-map PATHS ${_lib_dir})
        endif()
    endif()
    # 链接的第三方库
    if(tsl-ordered-map_FOUND)
        target_link_libraries(${__target_name} PUBLIC tsl::ordered_map)
        message(STATUS "  |-link tsl::ordered_map")
    else()
        message(FATAL_ERROR "  can not find tsl-ordered-map")
    endif()
endmacro(damacro_import_orderedmap)
