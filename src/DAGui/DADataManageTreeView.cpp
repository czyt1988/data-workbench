#include "DADataManageTreeView.h"
// Qt
#include <QMenu>
#include <QEvent>
#include <QItemSelectionModel>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
// APP
#include "DADataManagerTreeModel.h"
// DAUtils
#include "DAStringUtil.h"

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DADataManageTreeView
//===================================================
DADataManageTreeView::DADataManageTreeView(QWidget* parent) : QTreeView(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    //设置可拖拽性质
    setDefaultDropAction(Qt::MoveAction);
    setDragDropMode(QAbstractItemView::DragDrop);

    //装载数据
    mModel = new DADataManagerTreeModel();
    setModel(mModel);
    //建立菜单
    mActionAddDataFolder = createAction("actionAddDataFolder", ":/icon/icon/folder.svg");
    mMenu               = new QMenu(this);
    mMenu->addAction(mActionAddDataFolder);
    //建立信号
    connect(this, &DADataManageTreeView::customContextMenuRequested, this, &DADataManageTreeView::onCustomContextMenuRequested);
    connect(mActionAddDataFolder, &QAction::triggered, this, &DADataManageTreeView::onActionAddDataFolderTriggered);
    retranslateUi();
}

DADataManageTreeView::~DADataManageTreeView()
{
}

void DADataManageTreeView::setDataManager(DADataManager* mgr)
{
    mModel->setDataManager(mgr);
}

DADataManager* DADataManageTreeView::getDataManager() const
{
    return mModel->getDataManager();
}

void DADataManageTreeView::retranslateUi()
{
    mActionAddDataFolder->setText(tr("Add Folder"));  // cn:新建文件夹
}

/**
 * @brief 添加一个文件夹
 * @param name
 */
DADataManagerTreeItem* DADataManageTreeView::addDataFolder(const QString& name)
{
    QString n = name;
    if (n.isEmpty()) {
        n = tr("untitle floder");
    }
    //获取当前选中的item
    DADataManagerTreeItem* parentFolder = getCurrentSelectFolder();
    //获取这个item下的所有名字
    QSet< QString > names = QSet< QString >::fromList(mModel->getChildItemNames(parentFolder));
    //获取唯一名字
    n = DA::makeUniqueString(names, n);
    //构建目录
    return mModel->addFolder(n, parentFolder);
}

/**
 * @brief 移除文件夹
 */
void DADataManageTreeView::removeCurrentSelectDataFloder()
{
    DADataManagerTreeItem* f = getCurrentSelectFolder();
    if (nullptr == f) {
        return;
    }
    mModel->removeFolder(f);
}

/**
 * @brief 获取当前选中的文件夹
 * @return 如果没有选中，返回nullptr
 */
DADataManagerTreeItem* DADataManageTreeView::getCurrentSelectFolder() const
{
    QItemSelectionModel* selModel = selectionModel();
    QModelIndexList sm            = selModel->selectedRows();
    for (const QModelIndex& i : qAsConst(sm)) {
        QStandardItem* treeitem = mModel->itemFromIndex(i);
        if (nullptr == treeitem) {
            continue;
        }
        if (DAAPPDATAMANAGERTREEITEM_USERTYPE == treeitem->type()) {
            DADataManagerTreeItem* ti = static_cast< DADataManagerTreeItem* >(treeitem);
            if (ti->isFolder()) {
                return ti;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 获取选中的数据
 * @return
 */
QList< DADataManagerTreeItem* > DADataManageTreeView::getCurrentSelectDatas() const
{
    QList< DADataManagerTreeItem* > res;
    QItemSelectionModel* selModel = selectionModel();
    QModelIndexList sm            = selModel->selectedRows();
    for (const QModelIndex& i : qAsConst(sm)) {
        QStandardItem* treeitem = mModel->itemFromIndex(i);
        if (nullptr == treeitem) {
            continue;
        }
        if (DAAPPDATAMANAGERTREEITEM_USERTYPE == treeitem->type()) {
            res.append(static_cast< DADataManagerTreeItem* >(treeitem));
        }
    }
    return res;
}

void DADataManageTreeView::dragEnterEvent(QDragEnterEvent* ev)
{
    //    ev->setDropAction(Qt::MoveAction);
    //    QList< QTreeWidgetItem* > selectedItem = selectedItems();
    //    m_dragItemVec.clear();
    //    for (int i = 0; i < selectedItem.size(); i++) {
    //        if (selectedItem[ i ] && selectedItem[ i ]->parent()) {
    //            m_dragItemVec.push_back(selectedItem[ i ]);
    //        }
    //    }
    //    if (m_dragItemVec.size() > 0) {
    //        ev->acceptProposedAction();
    //        QTreeWidget::dragEnterEvent(ev);
    //        return;
    //    }
    //    ev->ignore();
    if (ev->mimeData()->hasFormat("da/tree-data")) { }
    QTreeView::dragEnterEvent(ev);
}

void DADataManageTreeView::dragMoveEvent(QDragMoveEvent* ev)
{
    QTreeView::dragMoveEvent(ev);
}

void DADataManageTreeView::dropEvent(QDropEvent* ev)
{
    QTreeView::dropEvent(ev);
}

void DADataManageTreeView::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        retranslateUi();
        break;

    default:
        break;
    }
}

void DADataManageTreeView::onCustomContextMenuRequested(const QPoint& pos)
{
    mMenu->exec(mapToGlobal(pos));
}

void DADataManageTreeView::onActionAddDataFolderTriggered()
{
    addDataFolder();
}

QAction* DADataManageTreeView::createAction(const char* objname, const char* iconpath)
{
    QAction* act = new QAction(this);
    act->setObjectName(QString::fromUtf8(objname));
    QIcon icon(iconpath);
    act->setIcon(icon);
    return act;
}
