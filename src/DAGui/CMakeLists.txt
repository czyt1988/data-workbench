
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAGui"
    "DA GUI | https://github.com/czyt1988"
    0
    1
    1
)

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Xml
    Svg
    OpenGL
    REQUIRED
)


# 子目录
set(DA_GUI_SUBDIR_Models ${CMAKE_CURRENT_SOURCE_DIR}/Models)
set(DA_GUI_SUBDIR_Dialog ${CMAKE_CURRENT_SOURCE_DIR}/Dialog)
set(DA_GUI_SUBDIR_MimeData ${CMAKE_CURRENT_SOURCE_DIR}/MimeData)
set(DA_GUI_SUBDIR_Commands ${CMAKE_CURRENT_SOURCE_DIR}/Commands)
set(DA_GUI_SUBDIR_ChartBackend)
if(DA_USE_QIM)
    # 使用qim
    set(DA_GUI_SUBDIR_ChartBackend "${CMAKE_CURRENT_SOURCE_DIR}/qim")
else()
    set(DA_GUI_SUBDIR_ChartBackend "${CMAKE_CURRENT_SOURCE_DIR}/qim")
endif()
########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
set(DA_GUI_HEADER_FILES
    DAGuiAPI.h
    DAAbstractOperateWidget.h
    DACacheWindowTableView.h
    DADataManagerComboBox.h
    DADataOperatePageWidget.h
    DADataTableView.h
    DAGraphicsPixmapItemSettingWidget.h
    DADataListView.h
    DADataManageTableView.h
    DADataManageWidget.h
    DADataManagerTreeWidget.h
    DADataOperateWidget.h
    DAGuiEnumStringUtils.h
    DAMessageLogViewWidget.h
    DANodeItemSettingWidget.h
    DANodeLinkItemSettingWidget.h
    DANodeListWidget.h
    DANodeMimeData.h
    DANodeSettingWidget.h
    DANodeTreeWidget.h
    DARecentFilesManager.h
    DAToolBox.h
    DAWorkFlowEditWidget.h
    DAWorkFlowGraphicsScene.h
    DAWorkFlowGraphicsView.h
    DAWorkFlowNodeListWidget.h
    DAWorkFlowNodeItemSettingWidget.h
    DAWorkFlowOperateWidget.h
    DASettingContainerWidget.h
    DAStatusBarWidget.h
    DAAbstractArchive.h
    DAAbstractArchiveTask.h
    DAZipArchive.h
    DAZipArchiveThreadWrapper.h
    DAZipArchiveTask_ByteArray.h
    DAZipArchiveTask_Xml.h
    DAZipArchiveTask_ArchiveFile.h
    DAXmlHelper.h
)
set(DA_GUI_SOURCE_FILES
    DAAbstractOperateWidget.cpp
    DACacheWindowTableView.cpp
    DADataListView.cpp
    DADataManageTableView.cpp
    DADataManagerTreeWidget.cpp
    DADataManageWidget.cpp
    DADataManagerComboBox.cpp
    DADataOperatePageWidget.cpp
    DADataOperateWidget.cpp
    DADataTableView.cpp
    DAGraphicsPixmapItemSettingWidget.cpp
    DAGuiEnumStringUtils.cpp
    DAMessageLogViewWidget.cpp
    DANodeItemSettingWidget.cpp
    DANodeLinkItemSettingWidget.cpp
    DANodeListWidget.cpp
    DANodeMimeData.cpp
    DANodeSettingWidget.cpp
    DANodeTreeWidget.cpp
    DARecentFilesManager.cpp
    DAToolBox.cpp
    DAWorkFlowEditWidget.cpp
    DAWorkFlowGraphicsScene.cpp
    DAWorkFlowGraphicsView.cpp
    DAWorkFlowNodeListWidget.cpp
    DAWorkFlowNodeItemSettingWidget.cpp
    DAWorkFlowOperateWidget.cpp
    DASettingContainerWidget.cpp
    DAStatusBarWidget.cpp
    DAAbstractArchive.cpp
    DAAbstractArchiveTask.cpp
    DAZipArchive.cpp
    DAZipArchiveThreadWrapper.cpp
    DAZipArchiveTask_ByteArray.cpp
    DAZipArchiveTask_Xml.cpp
    DAZipArchiveTask_ArchiveFile.cpp
    DAXmlHelper.cpp
)
set(DA_GUI_QT_UI_FILES
    DADataManageWidget.ui
    DADataManagerTreeWidget.ui
    DADataOperateWidget.ui
    DAGraphicsPixmapItemSettingWidget.ui
    DAMessageLogViewWidget.ui
    DANodeItemSettingWidget.ui
    DANodeLinkItemSettingWidget.ui
    DANodeSettingWidget.ui
    DAWorkFlowOperateWidget.ui
    DAWorkFlowEditWidget.ui
    DAWorkFlowNodeListWidget.ui
    DAWorkFlowNodeItemSettingWidget.ui
)
set(DA_GUI_QT_RC_FILES
    icon.qrc
)

# --------------------------------------
# 绘图相关，更加当前绘图渲染端选择加载不同路径下的代码文件
# --------------------------------------

if(DA_USE_QIM)
    # 使用qim
    list(APPEND DA_GUI_HEADER_FILES_ChartWidgets
        ${DA_GUI_SUBDIR_ChartBackend}/DAAbstractPlotAddItemWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotManageWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotOperateWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureFactory.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotSettingWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureTreeView.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotNodeSettingWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAAbstractPlotItemSettingWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotCommonItemsSettingWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureWidgetSettingWidget.h
    )
    list(APPEND DA_GUI_SOURCE_FILES_ChartWidgets
        ${DA_GUI_SUBDIR_ChartBackend}/DAAbstractPlotAddItemWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotManageWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotOperateWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureFactory.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotSettingWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureTreeView.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotNodeSettingWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAAbstractPlotItemSettingWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotCommonItemsSettingWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureWidgetSettingWidget.cpp
    )
    list(APPEND DA_GUI_RESOURCE_FILES_ChartWidgets
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotManageWidget.ui
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotOperateWidget.ui
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotSettingWidget.ui
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotNodeSettingWidget.ui
        ${DA_GUI_SUBDIR_ChartBackend}/DAPlotCommonItemsSettingWidget.ui
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureWidgetSettingWidget.ui
    )
else()
    # 使用qwt，qwt相关的窗口在qwt文件夹下
    # 由于还需要进行python环境的判断，这里不能直接file(GLOB_RECURSE..)来扫描
    list(APPEND DA_GUI_HEADER_FILES_ChartWidgets
        ${DA_GUI_SUBDIR_ChartBackend}/DAAbstractChartAddItemWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartItemsManager.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartSettingWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartManageWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartOperateWidget.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAZipArchiveTask_ChartItem.h
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureFactory.h
    )
    list(APPEND DA_GUI_SOURCE_FILES_ChartWidgets
        ${DA_GUI_SUBDIR_ChartBackend}/DAAbstractChartAddItemWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartItemsManager.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartSettingWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartManageWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartOperateWidget.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAZipArchiveTask_ChartItem.cpp
        ${DA_GUI_SUBDIR_ChartBackend}/DAFigureFactory.cpp
    )
    list(APPEND DA_GUI_RESOURCE_FILES_ChartWidgets
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartSettingWidget.ui
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartManageWidget.ui
        ${DA_GUI_SUBDIR_ChartBackend}/DAChartOperateWidget.ui
    )
    # 下面这些要dataframe支持
    if(DA_ENABLE_PYTHON)
        list(APPEND DA_GUI_HEADER_FILES_ChartWidgets
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddXYSeriesWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddXYESeriesWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddOHLCSeriesWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddtGridRasterDataWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddCurveWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddBarWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddIntervalCurveWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddTradingCurveWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddSpectrogramWidget.h
            ${DA_GUI_SUBDIR_ChartBackend}/DADialogChartGuide.h
        )
        list(APPEND DA_GUI_SOURCE_FILES_ChartWidgets
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddXYSeriesWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddXYESeriesWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddOHLCSeriesWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddtGridRasterDataWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddCurveWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddBarWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddIntervalCurveWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddSpectrogramWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddTradingCurveWidget.cpp
            ${DA_GUI_SUBDIR_ChartBackend}/vDADialogChartGuide.cpp
        )
        list(APPEND DA_GUI_RESOURCE_FILES_ChartWidgets
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddXYSeriesWidget.ui
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddXYESeriesWidget.ui
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddOHLCSeriesWidget.ui
            ${DA_GUI_SUBDIR_ChartBackend}/DAChartAddtGridRasterDataWidget.ui
            ${DA_GUI_SUBDIR_ChartBackend}/DADialogChartGuide.ui
        )
    endif()
endif()


# python about
if(DA_ENABLE_PYTHON)
    list(APPEND DA_GUI_HEADER_FILES
        DADataframeToVectorPointWidget.h
        DADataOperateOfDataFrameWidget.h
        DAPyDataFrameTableView.h
        DAPySeriesTableView.h
        DAPySeriesListView.h
    )
    list(APPEND DA_GUI_HEADER_FILES
        DADataframeToVectorPointWidget.cpp
        DADataOperateOfDataFrameWidget.cpp
        DAPyDataFrameTableView.cpp
        DAPySeriesTableView.cpp
        DAPySeriesListView.cpp
    )
    list(APPEND DA_GUI_QT_UI_FILES
        DADataframeToVectorPointWidget.ui
        DADataOperateOfDataFrameWidget.ui
    )
endif()


# Models
set(DA_GUI_HEADER_FILES_Models
    ${DA_GUI_SUBDIR_Models}/DAAbstractCacheWindowTableModel.h
    ${DA_GUI_SUBDIR_Models}/DADataManagerTreeModel.h
    ${DA_GUI_SUBDIR_Models}/DADataManagerTreeFilterProxyModel.h
    ${DA_GUI_SUBDIR_Models}/DATreeModel.h
    ${DA_GUI_SUBDIR_Models}/DADataTableModel.h
    ${DA_GUI_SUBDIR_Models}/DADataManagerTableModel.h
    ${DA_GUI_SUBDIR_Models}/DAMessageLogsModel.h
    ${DA_GUI_SUBDIR_Models}/DAVariantTableModel.h
)
set(DA_GUI_SOURCE_FILES_Models
    ${DA_GUI_SUBDIR_Models}/DAAbstractCacheWindowTableModel.cpp
    ${DA_GUI_SUBDIR_Models}/DADataManagerTreeModel.cpp
    ${DA_GUI_SUBDIR_Models}/DADataManagerTreeFilterProxyModel.cpp
    ${DA_GUI_SUBDIR_Models}/DATreeModel.cpp
    ${DA_GUI_SUBDIR_Models}/DADataTableModel.cpp
    ${DA_GUI_SUBDIR_Models}/DADataManagerTableModel.cpp
    ${DA_GUI_SUBDIR_Models}/DAMessageLogsModel.cpp
    ${DA_GUI_SUBDIR_Models}/DAVariantTableModel.cpp
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_GUI_HEADER_FILES_Models
        ${DA_GUI_SUBDIR_Models}/DAPySeriesTableModel.h
        ${DA_GUI_SUBDIR_Models}/DAPyDataFrameTableModel.h
        ${DA_GUI_SUBDIR_Models}/DAPyGridDataTableModel.h
    )
    list(APPEND DA_GUI_SOURCE_FILES_Models
        ${DA_GUI_SUBDIR_Models}/DAPySeriesTableModel.cpp
        ${DA_GUI_SUBDIR_Models}/DAPyDataFrameTableModel.cpp
        ${DA_GUI_SUBDIR_Models}/DAPyGridDataTableModel.cpp
    )
endif()

#Dialog
set(DA_GUI_HEADER_FILES_Dialog
    ${DA_GUI_SUBDIR_Dialog}/DARenameColumnsNameDialog.h
)

set(DA_GUI_SOURCE_FILES_Dialog
    ${DA_GUI_SUBDIR_Dialog}/DARenameColumnsNameDialog.cpp
)

set(DA_GUI_QT_UI_FILES_Dialog
    ${DA_GUI_SUBDIR_Dialog}/DARenameColumnsNameDialog.ui
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_GUI_HEADER_FILES_Dialog
        ${DA_GUI_SUBDIR_Dialog}/DATxtFileImportDialog.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogInsertNewColumn.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogPythonArgs.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameSeriesSelector.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameDataSelect.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameQueryDatas.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameDataSearch.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameEvalDatas.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameSort.h
        ${DA_GUI_SUBDIR_Dialog}/DADialogCreatePivotTable.h
		
    )
    list(APPEND DA_GUI_SOURCE_FILES_Dialog
        ${DA_GUI_SUBDIR_Dialog}/DATxtFileImportDialog.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogInsertNewColumn.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogPythonArgs.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameSeriesSelector.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameDataSelect.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameQueryDatas.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameDataSearch.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameEvalDatas.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameSort.cpp
        ${DA_GUI_SUBDIR_Dialog}/DADialogCreatePivotTable.cpp
    )
    list(APPEND DA_GUI_QT_UI_FILES_Dialog
        ${DA_GUI_SUBDIR_Dialog}/DATxtFileImportDialog.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogInsertNewColumn.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameSeriesSelector.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameDataSelect.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameQueryDatas.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameDataSearch.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameEvalDatas.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogDataFrameSort.ui
        ${DA_GUI_SUBDIR_Dialog}/DADialogCreatePivotTable.ui
    )
endif()
#MimeData
file(GLOB DA_GUI_HEADER_FILES_MimeData ${DA_GUI_SUBDIR_MimeData}/*.h)
file(GLOB DA_GUI_SOURCE_FILES_MimeData ${DA_GUI_SUBDIR_MimeData}/*.cpp)

#Commands

set(DA_GUI_HEADER_FILES_Commands
    ${DA_GUI_SUBDIR_Commands}/DACommandWithRedoCount.h
    ${DA_GUI_SUBDIR_Commands}/DACommandsForWorkFlow.h
)

set(DA_GUI_SOURCE_FILES_Commands
    ${DA_GUI_SUBDIR_Commands}/DACommandWithRedoCount.cpp
    ${DA_GUI_SUBDIR_Commands}/DACommandsForWorkFlow.cpp
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_GUI_HEADER_FILES_Commands
        ${DA_GUI_SUBDIR_Commands}/DACommandWithTemporaryData.h
        ${DA_GUI_SUBDIR_Commands}/DACommandsDataFrame.h
    )
    list(APPEND DA_GUI_SOURCE_FILES_Commands
        ${DA_GUI_SUBDIR_Commands}/DACommandWithTemporaryData.cpp
        ${DA_GUI_SUBDIR_Commands}/DACommandsDataFrame.cpp
    )
endif()



add_library(${DA_LIB_NAME} SHARED
            #Root
            ${DA_GUI_HEADER_FILES}
            ${DA_GUI_SOURCE_FILES}
            ${DA_GUI_QT_UI_FILES}
            ${DA_GUI_QT_RC_FILES}
            #Models
            ${DA_GUI_HEADER_FILES_Models}
            ${DA_GUI_SOURCE_FILES_Models}
            #Dialog
            ${DA_GUI_HEADER_FILES_Dialog}
            ${DA_GUI_SOURCE_FILES_Dialog}
            ${DA_GUI_QT_UI_FILES_Dialog}
            #MimeData
            ${DA_GUI_HEADER_FILES_MimeData}
            ${DA_GUI_SOURCE_FILES_MimeData}
            #Commands
            ${DA_GUI_HEADER_FILES_Commands}
            ${DA_GUI_SOURCE_FILES_Commands}
            #绘图相关
            ${DA_GUI_HEADER_FILES_ChartWidgets}
            ${DA_GUI_SOURCE_FILES_ChartWidgets}
            ${DA_GUI_RESOURCE_FILES_ChartWidgets}
)   

# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAGUI_BUILDLIB)#!!!!** 注意变更 **!!!!
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::OpenGL
)
if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat OpenGLWidgets)
    target_link_libraries(${DA_LIB_NAME} PRIVATE
        Qt6::Core5Compat
        Qt6::OpenGLWidgets
    )
endif()



# -------------link DAWorkBench--------------------------
# DAUtils
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到${DA_PROJECT_NAME}
# find_package(${DA_PROJECT_NAME} COMPONENTS
#     DAUtils
#     DAMessageHandler
#     DAData
#     DACommonWidgets
#     DAGraphicsView
#     DAWorkFlow
#     DAFigure
# )
# target_link_libraries(${DA_LIB_NAME} PUBLIC
#     ${DA_PROJECT_NAME}::DAUtils
#     ${DA_PROJECT_NAME}::DAMessageHandler
#     ${DA_PROJECT_NAME}::DAData
#     ${DA_PROJECT_NAME}::DACommonWidgets
#     ${DA_PROJECT_NAME}::DAGraphicsView
#     ${DA_PROJECT_NAME}::DAWorkFlow
#     ${DA_PROJECT_NAME}::DAFigure
# )
find_package(${DA_PROJECT_NAME} COMPONENTS
    DAUtils
    DAMessageHandler
    DAData
    DACommonWidgets
    DAWorkFlow
)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAUtils")
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAMessageHandler")
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAData")
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DACommonWidgets")
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAWorkFlow")
endif()
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
    ${DA_PROJECT_NAME}::DAMessageHandler
    ${DA_PROJECT_NAME}::DAData
    ${DA_PROJECT_NAME}::DACommonWidgets
    ${DA_PROJECT_NAME}::DAWorkFlow
)
if(DA_USE_QIM)
    target_link_libraries(${DA_LIB_NAME} PUBLIC
        ${DA_PROJECT_NAME}::DAPlot
    )
else()
    target_link_libraries(${DA_LIB_NAME} PUBLIC
        ${DA_PROJECT_NAME}::DAFigure
    )
endif()


if(DA_ENABLE_PYTHON)
    find_package(${DA_PROJECT_NAME} COMPONENTS
        DAPyBindQt
        DAPyScripts
        DAPyCommonWidgets
    )
    if(${DA_PROJECT_NAME}_FOUND)
        message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAPyCommonWidgets")
    endif()
    target_link_libraries(${DA_LIB_NAME} PUBLIC
        ${DA_PROJECT_NAME}::DAPyBindQt
        ${DA_PROJECT_NAME}::DAPyScripts
        ${DA_PROJECT_NAME}::DAPyCommonWidgets
    )

    # -------------link python--------------------------
    damacro_import_Python(${DA_LIB_NAME})
    # -------------link 3rdparty pybind11--------------------------
    damacro_import_pybind11(${DA_LIB_NAME})
endif()

# -------------link SARibbon--------------------------
damacro_import_SARibbonBar(${DA_LIB_NAME})

# -------------link QtAdvancedDockingSystem--------------------------
damacro_import_QtAdvancedDocking(${DA_LIB_NAME})

# -------------link ctk--------------------------
damacro_import_DALiteCtk(${DA_LIB_NAME})

# -------------link quazip--------------------------
damacro_import_quazip(${DA_LIB_NAME})
########################################################
# 设置通用库属性
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})


if(DA_USE_QIM)
    damacro_import_qim(${DA_LIB_NAME})
    target_compile_definitions(${DA_LIB_NAME} PUBLIC DA_USE_QIM=1)
else()
    # -------------link qwt--------------------------
    damacro_import_qwt(${DA_LIB_NAME})
endif()
########################################################
# 附加编译选项
########################################################
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    # 这里是为 MSVC 编译器设置的选项
    target_link_options(${DA_LIB_NAME} PRIVATE /VERBOSE)
    # 或者设置其他 MSVC 特定的编译或链接选项
else()
    # 这里是为非 MSVC 编译器设置的选项（如果有的话）
endif()
########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_GUI_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)
#Models
install(FILES
    ${DA_GUI_HEADER_FILES_Models}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)
#Dialog
install(FILES
    ${DA_GUI_HEADER_FILES_Dialog}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)
#MimeData
install(FILES
    ${DA_GUI_HEADER_FILES_MimeData}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)
#Commands
install(FILES
    ${DA_GUI_HEADER_FILES_Commands}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)
#Chart相关
install(FILES
    ${DA_GUI_HEADER_FILES_ChartWidgets}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)

