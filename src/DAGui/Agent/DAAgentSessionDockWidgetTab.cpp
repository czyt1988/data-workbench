// DAAgentSessionDockWidgetTab.cpp
#include "DAAgentSessionDockWidgetTab.h"
// Qt
#include <QMenu>
#include <QAction>
// ADS
#include "DockWidget.h"
#include "DockManager.h"
// DAGui
#include "DAAgentDockWidget.h"

namespace DA
{

//===================================================
// DAAgentSessionDockWidgetTab
//===================================================
DAAgentSessionDockWidgetTab::DAAgentSessionDockWidgetTab(ads::CDockWidget* dockWidget, QWidget* parent)
    : Super(dockWidget, parent)
{
}

DAAgentSessionDockWidgetTab::~DAAgentSessionDockWidgetTab()
{
}

/**
 * @brief 构造选项卡右键菜单
 *
 * 先调用基类填充 ADS 默认项（Detach / Pin / Close 等），再追加自定义项：
 * 重命名会话（全部会话）/ 停止会话（仅运行中/等待输入）/ 删除会话（带确认）。
 * @param menu 待填充的菜单，为空则基类新建
 * @return 填充后的菜单
 */
QMenu* DAAgentSessionDockWidgetTab::buildContextMenu(QMenu* menu)
{
    menu = Super::buildContextMenu(menu);
    if (!menu) {
        return nullptr;
    }
    DAAgentDockWidget* host = hostWidget();
    const QString sid = sessionId();
    if (!host || sid.isEmpty()) {
        return menu;  // unbound 视图 / 宿主不可达：不提供会话操作
    }
    menu->addSeparator();
    QAction* renameAct = menu->addAction(tr("Rename Session"));  // cn:重命名会话
    connect(renameAct, &QAction::triggered, this, [ host, sid ]() {
        host->requestRenameSession(sid);
    });
    if (host->isSessionActive(sid)) {
        QAction* stopAct = menu->addAction(tr("Stop Session"));  // cn:停止会话
        connect(stopAct, &QAction::triggered, this, [ host, sid ]() {
            host->requestStopSession(sid);
        });
    }
    QAction* deleteAct = menu->addAction(tr("Delete Session"));  // cn:删除会话
    connect(deleteAct, &QAction::triggered, this, [ host, sid ]() {
        host->requestDeleteSession(sid);
    });
    return menu;
}

/**
 * @brief 取宿主：沿 dock → 嵌套管理器 → 宿主部件链解析
 *
 * 会话 dock 由 DAAgentDockWidget::createSessionView 创建，其 dockManager()
 * 即宿主持有的嵌套管理器，父链再上一层即宿主。
 */
DAAgentDockWidget* DAAgentSessionDockWidgetTab::hostWidget() const
{
    ads::CDockWidget* dock = dockWidget();
    if (!dock || !dock->dockManager()) {
        return nullptr;
    }
    return qobject_cast< DAAgentDockWidget* >(dock->dockManager()->parentWidget());
}

/**
 * @brief 本 tab 所属会话 ID（dock objectName；unbound 视图返回空）
 *
 * bound 会话 dock 的 objectName 即会话 ID；unbound 视图为固定占位名
 * （da_agentUnboundSessionDock），此处识别并返回空——右键菜单对 unbound
 * 视图不提供会话操作（尚未落盘建会话，无可重命名/删除的实体）。
 */
QString DAAgentSessionDockWidgetTab::sessionId() const
{
    ads::CDockWidget* dock = dockWidget();
    if (!dock) {
        return QString();
    }
    const QString name = dock->objectName();
    if (name == QLatin1String("da_agentUnboundSessionDock")) {
        return QString();  // unbound 视图
    }
    return name;
}

}  // namespace DA
