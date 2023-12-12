#include "DAPluginManagerDialog.h"
#include "ui_DAPluginManagerDialog.h"
#include <QTreeWidgetItem>
#include <QDebug>
#include "DAAbstractNodePlugin.h"
#include "DAAppPluginManager.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPluginManagerDialog
//===================================================
DAPluginManagerDialog::DAPluginManagerDialog(QWidget* parent) : QDialog(parent), ui(new Ui::DAPluginManagerDialog)
{
    ui->setupUi(this);
    init();
}

DAPluginManagerDialog::~DAPluginManagerDialog()
{
    delete ui;
}

void DAPluginManagerDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    default:
        break;
    }
}

void DAPluginManagerDialog::init()
{
    ui->treeWidget->setColumnCount(4);  //插件名称，插件b版本，是否加载
    ui->treeWidget->setHeaderLabels({ tr("name"), tr("version"), tr("is loaded"), tr("description") });
    DAAppPluginManager& plugin            = DAAppPluginManager::instance();
    QList< DAAbstractNodePlugin* > nodeplugins = plugin.getNodePlugins();
    QTreeWidgetItem* rootItem                  = new QTreeWidgetItem(ui->treeWidget);

    //节点插件
    rootItem->setText(0, tr("node plugin"));
    ui->treeWidget->insertTopLevelItem(0, rootItem);
    for (DAAbstractNodePlugin* p : nodeplugins) {
        qDebug() << tr("plugin name is %1").arg(p->getName());
        QTreeWidgetItem* item = new QTreeWidgetItem(rootItem);
        item->setText(0, p->getName());
        item->setText(1, p->getVersion());
        item->setText(2, tr("is load"));
        item->setText(3, p->getDescription());
    }
    ui->treeWidget->expandAll();
}
