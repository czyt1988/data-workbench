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
    connect(ui->dataTreeWidget, &DADataManagerTreeWidget::dataDbClicked, this, &DADataManageWidget::dataDbClicked);
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
    mDataManager = dmgr;
    ui->dataTreeWidget->setDataManager(dmgr);
}

/**
 * @brief 获取一个选中的数据
 * @return
 */
DAData DADataManageWidget::getCurrentSelectData() const
{
    return ui->dataTreeWidget->getCurrentSelectData();
}

/**
 * @brief 获取所有选中的数据
 * @return
 */
QList< DAData > DADataManageWidget::getAllSelectDatas() const
{
    return ui->dataTreeWidget->getAllSelectDatas();
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
    QList< DAData > d = getAllSelectDatas();
    if (d.size() <= 0) {
        qWarning() << tr("Please select the data item to remove");  // cn:请选择需要删除的数据条目
        return;
    }
    mDataManager->removeDatas_(d);
}

void DADataManageWidget::retranslateUi()
{
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
