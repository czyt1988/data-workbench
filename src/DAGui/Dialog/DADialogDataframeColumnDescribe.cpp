#include "DADialogDataframeColumnDescribe.h"
#include "ui_DADialogDataframeColumnDescribe.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DADialogDataframeColumnDescribe
//===================================================
DADialogDataframeColumnDescribe::DADialogDataframeColumnDescribe(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataframeColumnDescribe)
{
    ui->setupUi(this);
}

DADialogDataframeColumnDescribe::~DADialogDataframeColumnDescribe()
{
    delete ui;
}

void DADialogDataframeColumnDescribe::setColumnName(const QString& name)
{
    ui->labelColumnName->setText(tr("Column: %1").arg(name));  // cn:列：%1
}

void DADialogDataframeColumnDescribe::setColumnDType(const QString& dtype)
{
    ui->labelDType->setText(tr("Type: %1").arg(dtype));  // cn:类型：%1
}

void DADialogDataframeColumnDescribe::setStatistics(const QList<QPair<QString, QString>>& stats)
{
    QTableWidget* tw = ui->tableWidget;
    tw->clear();
    tw->setRowCount(static_cast<int>(stats.size()));
    tw->setColumnCount(2);
    tw->setHorizontalHeaderLabels({tr("Statistic"), tr("Value")});  // cn:统计项,值
    for (int i = 0; i < stats.size(); ++i) {
        QTableWidgetItem* item0 = new QTableWidgetItem(stats[ i ].first);
        QTableWidgetItem* item1 = new QTableWidgetItem(stats[ i ].second);
        item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
        item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
        tw->setItem(i, 0, item0);
        tw->setItem(i, 1, item1);
    }
    tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tw->verticalHeader()->setVisible(false);
}
