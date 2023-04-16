#include "DADataManageWidget.h"
#include "ui_DADataManageWidget.h"
// qt
#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <QHeaderView>
#include <QDebug>

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DADataManageWidget
//===================================================
DADataManageWidget::DADataManageWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DADataManageWidget)
{
    ui->setupUi(this);

    //构建action
    mActionViewDataListByTable = new QAction(this);
    mActionViewDataListByTable->setObjectName("actionViewDataListByTable");
    mActionViewDataListByTable->setCheckable(true);
    mActionViewDataListByTable->setIcon(QIcon(":/icon/icon/showDataInList.svg"));
    mActionViewDataListByTree = new QAction(this);
    mActionViewDataListByTree->setObjectName("actionViewDataListByTree");
    mActionViewDataListByTree->setCheckable(true);
    mActionViewDataListByTree->setIcon(QIcon(":/icon/icon/showDataInTree.svg"));
    mActionGroup = new QActionGroup(this);
    mActionGroup->addAction(mActionViewDataListByTable);
    mActionGroup->addAction(mActionViewDataListByTree);
    mActionGroup->setExclusive(true);
    ui->toolButtonShowTableView->setDefaultAction(mActionViewDataListByTable);
    ui->toolButtonShowTreeView->setDefaultAction(mActionViewDataListByTree);
    mActionViewDataListByTable->setChecked(true);
    connect(mActionViewDataListByTable, &QAction::triggered, this, &DADataManageWidget::onActionTableViewTriggered);
    connect(mActionViewDataListByTree, &QAction::triggered, this, &DADataManageWidget::onActionTreeViewTriggered);
    connect(ui->dataMgrTableView, &DADataManageTableView::dataDbClicked, this, &DADataManageWidget::dataDbClicked);
    retranslateUi();
}

DADataManageWidget::~DADataManageWidget()
{
    delete ui;
}

/**
 * @brief 设置DataManager
 * @param dmgr
 */
void DADataManageWidget::setDataManager(DADataManager* dmgr)
{
    ui->dataMgrTableView->setDataManager(dmgr);
    ui->dataMgrTreeView->setDataManager(dmgr);
}

/**
 * @brief 获取一个选中的数据
 * @return
 */
DAData DADataManageWidget::getOneSelectData() const
{
    if (isTableView()) {
        return ui->dataMgrTableView->getOneSelectData();
    } else if (isTreeView()) {
        // TODO
    }
    return DAData();
}

/**
 * @brief 获取所有选中的数据
 * @return
 */
QList< DAData > DADataManageWidget::getSelectDatas() const
{
    if (isTableView()) {
        return ui->dataMgrTableView->getSelectDatas();
    } else if (isTreeView()) {
        // TODO
    }
    return QList< DAData >();
}

/**
 * @brief 判断当前是否为table模式
 * @return
 */
bool DADataManageWidget::isTableView() const
{
    return (mActionGroup->checkedAction() == mActionViewDataListByTable);
}

/**
 * @brief 判断当前是否为tree模式
 * @return
 */
bool DADataManageWidget::isTreeView() const
{
    return (mActionGroup->checkedAction() == mActionViewDataListByTree);
}

/**
 * @brief 获取当前的显示模式
 * @return
 */
DADataManageWidget::DataViewMode DADataManageWidget::getCurrentDataViewMode() const
{
    return (isTableView() ? ViewDataInTable : ViewDataInTree);
}

/**
 * @brief 添加数据文件夹
 */
void DADataManageWidget::addDataFolder()
{
    if (isTreeView()) {
        ui->dataMgrTreeView->addDataFolder();
    }
}

/**
 * @brief 获取mgr
 * @return
 */
DADataManager* DADataManageWidget::getDataManager() const
{
    return mDataManager;
}

/**
 * @brief 移除选中的数据
 */
void DADataManageWidget::removeSelectData()
{
    QList< DAData > d = getSelectDatas();
    if (d.size() <= 0) {
        qWarning() << tr("Please select the data item to remove");  // cn:请选择需要删除的数据条目
        return;
    }
    mDataManager->removeDatas_(d);
}

void DADataManageWidget::retranslateUi()
{
    mActionViewDataListByTable->setToolTip(tr("show datas in table view"));
    mActionViewDataListByTree->setToolTip(tr("show datas in tree view"));
}

void DADataManageWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        retranslateUi();
        break;

    default:
        break;
    }
}

void DADataManageWidget::onActionTableViewTriggered(bool on)
{
    if (on) {
        if (ui->stackedWidget->currentWidget() != ui->dataMgrTableView) {
            ui->stackedWidget->setCurrentWidget(ui->dataMgrTableView);
            emit dataViewModeChanged(ViewDataInTable);
        }
    }
}

void DADataManageWidget::onActionTreeViewTriggered(bool on)
{
    if (on) {
        if (ui->stackedWidget->currentWidget() != ui->dataMgrTreeView) {
            ui->stackedWidget->setCurrentWidget(ui->dataMgrTreeView);
            emit dataViewModeChanged(ViewDataInTree);
        }
    }
}
