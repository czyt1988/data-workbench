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
    _actionViewDataListByTable = new QAction(this);
    _actionViewDataListByTable->setObjectName("actionViewDataListByTable");
    _actionViewDataListByTable->setCheckable(true);
    _actionViewDataListByTable->setIcon(QIcon(":/icon/icon/showDataInList.svg"));
    _actionViewDataListByTree = new QAction(this);
    _actionViewDataListByTree->setObjectName("actionViewDataListByTree");
    _actionViewDataListByTree->setCheckable(true);
    _actionViewDataListByTree->setIcon(QIcon(":/icon/icon/showDataInTree.svg"));
    _actionGroup = new QActionGroup(this);
    _actionGroup->addAction(_actionViewDataListByTable);
    _actionGroup->addAction(_actionViewDataListByTree);
    _actionGroup->setExclusive(true);
    ui->toolButtonShowTableView->setDefaultAction(_actionViewDataListByTable);
    ui->toolButtonShowTreeView->setDefaultAction(_actionViewDataListByTree);
    _actionViewDataListByTable->setChecked(true);
    connect(_actionViewDataListByTable, &QAction::triggered, this, &DADataManageWidget::onActionTableViewTriggered);
    connect(_actionViewDataListByTree, &QAction::triggered, this, &DADataManageWidget::onActionTreeViewTriggered);
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
    return (_actionGroup->checkedAction() == _actionViewDataListByTable);
}

/**
 * @brief 判断当前是否为tree模式
 * @return
 */
bool DADataManageWidget::isTreeView() const
{
    return (_actionGroup->checkedAction() == _actionViewDataListByTree);
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
    return _dataMgr;
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
    _dataMgr->removeDatas_(d);
}

void DADataManageWidget::retranslateUi()
{
    _actionViewDataListByTable->setToolTip(tr("show datas in table view"));
    _actionViewDataListByTree->setToolTip(tr("show datas in tree view"));
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
