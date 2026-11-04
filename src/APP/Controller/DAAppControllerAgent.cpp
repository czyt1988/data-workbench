#include "DAAppController.h"
// Qt
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTimer>
#include <QFontComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QApplication>
#include <QActionGroup>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QClipboard>
#include <QFile>
#include <QMimeData>
#include <QUrl>
// qwt
#include "qwt_figure.h"
#include "qwt_plot_series_data_picker.h"
#include <QwtPlotMarker>
#include <QwtPlotTextLabel>
#include <QwtText>
// SARibbon
#include "SARibbonGlobal.h"
#include "SARibbonMainWindow.h"

// API
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "Ribbon/DAAppRibbonArea.h"
#include "DAAppDockingArea.h"
#include "DAChartOperateWidget.h"
#include "DAAppCommand.h"
#include "DAUndoHistoryWidget.h"
#include "DAAppActions.h"
#include "DAAppDataManager.h"
#include "DAProjectInterface.h"
// plugin
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// command
// Widget
#include "DAWaitCursorScoped.h"
#include "Table/DADataOperateWidget.h"
#include "Table/DADataOperatePageWidget.h"
#include "Common/DAAbstractOperateWidget.h"
#include "DataManager/DADataManageWidget.h"
#include "DAAppChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "Chart/Setting/DAChartAxisSettingPanel.h"
#include "DAAppChartManageWidget.h"
#include "SettingPages/DASettingPageGeneral.h"
#include "Common/Settings/DASettingContainerWidget.h"
#include "Common/DARecentFilesManager.h"
#include "Chart/DAChartSettingWidget.h"
#include "Chart/DADataLinkTableWidget.h"
#include "DAColorTheme.h"
#include "Chart/Setting/DAFigureWidgetSettingPanel.h"
#include "Chart/Setting3D/DAChart3DSettingWidget.h"
// Dialog
#include "DAPluginManagerDialog.h"
#include "DAAppSettingDialog.h"
#include "Dialog/DAExportToPngSettingDialog.h"
#include "Dialog/DALayoutManagerDialog.h"
#include "Dialog/DAWorkbenchAboutDialog.h"
// DAWidgets
#include "DAFontEditPannelWidget.h"
#include "SARibbonColorToolButton.h"
#include "DAShapeEditPannelWidget.h"
// Table style
#include "Table/Metadata/DATableCellStyle.h"
// 表格元数据与数据撤销命令（004 T1.27 就地形状变换：Swap 命令 + 元数据束快照/清理）
#include "Table/Metadata/DATableMetadata.h"
#include "Table/Metadata/DATableMetadataRegistry.h"
#include "DADataUndoCommand.h"
// Workflow
#include "Workflow/DAPyWorkFlowOperateWidget.h"
#include "Workflow/DAPyWorkFlowGraphicsView.h"
#include "DAGraphicsDrawRectSceneAction.h"
#include "DAGraphicsDrawTextItemSceneAction.h"
// project
#include "DAAppProject.h"
#include "DAAppProjectActionPolicy.h"
// Py
#include "DAPybind11QtCaster.hpp"
#include "Table/Dialogs/DATxtFileImportDialog.h"
#include "DAPyDTypeComboBox.h"
#include "DAPyScripts.h"
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"
// Widget
#include "Table/DADataOperateOfDataFrameWidget.h"
#include "Table/Views/DADataHeaderView.h"
#include "Table/Views/DATableDisplayFormatComboBox.h"
#include "Table/Dialogs/DADialogTableDisplayFormat.h"
#include "Table/Views/DADataTableView.h"
// Python workflow
#include "DAPyWorkFlowScene.h"
// Stats plot widgets
#include "Chart/DADialogStatsChartGuide.h"
#include "Chart/AddItem/DAChartAddStatsHistplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsKdeplot1dWidget.h"
#include "Chart/AddItem/DAChartAddStatsKdeplot2dWidget.h"
#include "Chart/AddItem/DAChartAddStatsBoxplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsHeatmapWidget.h"
#include "Chart/AddItem/DAChartAddStatsScatterplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsBarplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsRegplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsEcdfplotWidget.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Chart/AddItem/DAAbstractStatsChartAddWidget.h"
#include "Chart/DAStatsPlotCoordinator.h"
// 类型化统计绘图请求（onStatsGuideAccepted 消费字段，需完整类型）
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
// 数据统计分析浮动 dock（PMF connect notifyDataChanged 需完整类型）
#include "Chart/DADataStatsWidget.h"
// 表内快速绘图（转入绘图区的编排器）
#include "Table/Chart/DAChartQuickPlot.h"
#include "Table/Chart/DATableChartTypes.h"
// Agent
#include "DAAgentModule.h"
#include "DAAgentInterface.h"        // PMF connect 到接口信号/方法需完整类型
#include "DAAgentDockWidget.h"       // DAAppDockingArea 仅前向声明；PMF connect 需完整类型（plan-02）
#include "DAAgentLinkDispatcher.h"   // Agent 本地跳转链接分发（da-<kind>: 协议注册表）
#include "DADataManagerInterface.h"  // da-data: 链接按名字/id 查找数据集
//
#include "SettingPages/DAAppConfig.h"
#include "DAAppLayoutManager.h"
#include "DALogCategory.h"

#ifndef DAAPPRIBBONAREA_WINDOW_NAME
#define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppController", "DA", nullptr)
#endif

// #ifndef DAAPPRIBBONAREA_WINDOW_NAME
// #define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppController", u8"试验数据分析系统", nullptr)
// #endif

// 未实现的功能标记
#define DAAPPCONTROLLER_PASS()                                                                                         \
    QMessageBox::warning(                                                                                              \
        app(),                                                                                                         \
        QCoreApplication::translate("DAAppRibbonArea", "Warning", nullptr),                                             \
        QCoreApplication::translate("DAAppRibbonArea",                                                                 \
                                    "The current function is not implemented, only the UI is reserved, "               \
                                    "please pay attention: https://gitee.com/czyt1988/data-work-flow",                 \
                                    nullptr))

// 快速链接信号槽
#define DAAPPCONTROLLER_ACTION_BIND(actionname, functionname)                                                          \
    connect(actionname, &QAction::triggered, this, &DAAppController::functionname)


namespace DA
{

/**
 * @brief 注册 Agent 聊天窗口本地跳转链接处理器
 *
 * 所有 da-<kind>: 协议在此集中注册到 DAAgentLinkDispatcher，新增协议只需
 * 在此追加一行 registerHandler + 对应 handle 函数，传输链路（chat.js 拦截、
 * WebChannel、信号转发）无需改动。
 */
void DAAppController::setupAgentLinkHandlers()
{
    mAgentLinkDispatcher.registerHandler(QStringLiteral("da-figure"),
                                         [ this ](const QString& href) { handleFigureLink(href); });
    mAgentLinkDispatcher.registerHandler(QStringLiteral("da-data"),
                                         [ this ](const QString& href) { handleDataLink(href); });
}

/**
 * @brief Agent 本地跳转超链接点击处理（协议无关入口）
 *
 * 把 href 交给 DAAgentLinkDispatcher 按协议前缀分发到已注册的 handler；
 * 未注册的协议提示用户（agent 输出了程序不支持的链接类型）。
 * @param href 超链接 href，形如 da-figure:&lt;name&gt;、da-data:id=&lt;id&gt;
 */
void DAAppController::onLinkActivated(const QString& href)
{
    if (!mAgentLinkDispatcher.dispatch(href)) {
        daWarning << tr("Unsupported link type: %1").arg(href);  // cn:不支持的链接类型：%1
    }
}

/**
 * @brief da-figure: 协议处理：定位目标 figure 并 raise 绘图区
 *
 * 支持两种格式：
 *   - da-figure:&lt;figure_name&gt;  按 tab 文本定位（agent 默认，简单）
 *   - da-figure:id=&lt;uuid&gt;       按 figure_id 精确定位（抗重名/改名）
 *
 * href 来自浏览器 DOM：markdown-it 渲染时会对非 ASCII 与特殊字符做百分号
 * 编码（中文/空格名 → %E5%9B%BE1，UUID 花括号 → %7B...%7D），且 figure_id
 * 本身带 QUuid 花括号，这里先解码再归一化花括号，保证与原始名称可匹配。
 * @param href 超链接 href
 */
void DAAppController::handleFigureLink(const QString& href)
{
    static const QString kPrefix = QStringLiteral("da-figure:");
    if (!href.startsWith(kPrefix, Qt::CaseInsensitive)) {
        qWarning() << "[FigureLink] invalid href:" << href;
        return;
    }
    QString payload = href.mid(kPrefix.length());
    // markdown-it normalizeLink 的百分号编码还原（中文/空格 figure 名等）
    payload = QUrl::fromPercentEncoding(payload.toUtf8());
    const bool isIdForm = payload.startsWith(QStringLiteral("id="), Qt::CaseInsensitive);
    QString idOrName = isIdForm ? payload.mid(3) : payload;
    // QUuid::toString() 生成带花括号的 {xxx}，agent 引用时可能写裸 UUID：
    // id 形式双向归一化（先试裸形式再试花括号形式）
    if (isIdForm && idOrName.startsWith('{') && idOrName.endsWith('}')) {
        idOrName = idOrName.mid(1, idOrName.length() - 2);
    }
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    if (!chartopt) {
        qWarning() << "[FigureLink] chart operate widget is null";
        return;
    }
    DAFigureWidget* fig = nullptr;
    if (isIdForm) {
        // 精确格式：da-figure:id=<uuid>（先裸 UUID，再带花括号兜底）
        fig = chartopt->findFigure(idOrName);
        if (!fig && !idOrName.isEmpty()) {
            fig = chartopt->findFigure(QStringLiteral("{%1}").arg(idOrName));
        }
    } else if (!idOrName.isEmpty()) {
        // 简单格式：da-figure:<figure_name>，按 tab 文本遍历匹配
        const QList< DAFigureWidget* > figs = chartopt->getFigureList();
        for (DAFigureWidget* f : figs) {
            if (chartopt->getFigureName(f) == idOrName) {
                fig = f;
                break;
            }
        }
    }
    if (!fig) {
        daWarning << tr("Figure '%1' not found, it may have been closed or renamed").arg(idOrName);  // cn:未找到绘图"%1"，可能已关闭或被重命名
        return;
    }
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    chartopt->setCurrentFigure(fig);
}

/**
 * @brief da-data: 协议处理：定位目标数据集并打开对应数据表
 *
 * 支持两种格式：
 *   - da-data:&lt;data_name&gt;    按数据集名定位（findData，大小写敏感）
 *   - da-data:id=&lt;id&gt;         按数据集 id 精确定位（getDataById，抗重名/改名）
 *
 * 与 da-figure: 相同的百分号解码处理（中文/空格名）。
 * @param href 超链接 href
 */
void DAAppController::handleDataLink(const QString& href)
{
    static const QString kPrefix = QStringLiteral("da-data:");
    if (!href.startsWith(kPrefix, Qt::CaseInsensitive)) {
        qWarning() << "[DataLink] invalid href:" << href;
        return;
    }
    QString payload = href.mid(kPrefix.length());
    payload = QUrl::fromPercentEncoding(payload.toUtf8());
    const bool isIdForm = payload.startsWith(QStringLiteral("id="), Qt::CaseInsensitive);
    const QString idOrName = isIdForm ? payload.mid(3) : payload;

    DADataManagerInterface* di = mCore->getDataManagerInterface();
    if (!di) {
        qWarning() << "[DataLink] data manager interface is null";
        return;
    }
    DAData data;
    if (isIdForm) {
        // 精确格式：da-data:id=<id>（DAAbstractData::IdType 为 uint64_t）
        bool ok = false;
        const DAData::IdType id = idOrName.toULongLong(&ok);
        if (!ok) {
            daWarning << tr("Invalid dataset link: '%1'").arg(href);  // cn:无效的数据集链接："%1"
            return;
        }
        data = di->getDataById(id);
    } else if (!idOrName.isEmpty()) {
        // 简单格式：da-data:<data_name>，按名字查找
        data = di->findData(idOrName);
    }
    if (data.isNull()) {
        daWarning << tr("Dataset '%1' not found, it may have been removed").arg(idOrName);  // cn:未找到数据集"%1"，可能已被移除
        return;
    }
    DADataOperateWidget* dataopt = getDataOperateWidget();
    if (!dataopt) {
        qWarning() << "[DataLink] data operate widget is null";
        return;
    }
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataOperate);
    dataopt->showData(data);
}

}  // end DA
