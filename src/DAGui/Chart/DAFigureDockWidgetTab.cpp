#include "DAFigureDockWidgetTab.h"
// Qt
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QPointer>
// ADS
#include "DockWidget.h"
// DAGui
#include "DAFigureDockWidget.h"
// DAFigure
#include "DAFigureWidget.h"
namespace DA
{
//===================================================
// DAFigureDockWidgetTab
//===================================================
DAFigureDockWidgetTab::DAFigureDockWidgetTab(ads::CDockWidget* dockWidget, QWidget* parent)
    : Super(dockWidget, parent)
{
}

DAFigureDockWidgetTab::~DAFigureDockWidgetTab()
{
}

/**
 * @brief 构造选项卡右键菜单
 *
 * 先调用基类填充 ADS 默认项（Detach / Pin / Close 等），再追加自定义项。
 * menu 为空时基类会 new QMenu(this)（parent 为本 tab），调用方负责执行。
 * @param menu 待填充的菜单，为空则基类新建
 * @return 填充后的菜单
 */
QMenu* DAFigureDockWidgetTab::buildContextMenu(QMenu* menu)
{
    menu = Super::buildContextMenu(menu);
    if (!menu) {
        return nullptr;
    }
    // 追加自定义项：重命名
    menu->addSeparator();
    QAction* renameAct = menu->addAction(tr("Rename Figure"));  // cn:重命名绘图
    QPointer< DAFigureWidget > fig = figureWidget();
    connect(renameAct, &QAction::triggered, this, [this, fig]() {
        if (!fig) {
            return;
        }
        bool ok = false;
        QString name = QInputDialog::getText(this,
                                             tr("Rename Figure"),  // cn:重命名绘图
                                             tr("Figure name:"),   // cn:绘图名称：
                                             QLineEdit::Normal,
                                             fig->windowTitle(),
                                             &ok);
        if (ok && !name.isEmpty()) {
            // 设置 figure 窗口标题 → windowTitleChanged →
            // DAChartOperateWidget::onFigureTitleChanged 同步 dock 标签
            fig->setWindowTitle(name);
        }
    });
    return menu;
}

/**
 * @brief 取本 tab 所属 dock 封装的绘图窗口
 * @return 对应的 DAFigureWidget，dock 内容不是 DAFigureDockWidget 时返回 nullptr
 */
DAFigureWidget* DAFigureDockWidgetTab::figureWidget() const
{
    ads::CDockWidget* dock = dockWidget();
    if (!dock) {
        return nullptr;
    }
    if (DAFigureDockWidget* fd = qobject_cast< DAFigureDockWidget* >(dock->widget())) {
        return fd->getFigureWidget();
    }
    return nullptr;
}

}  // namespace DA
