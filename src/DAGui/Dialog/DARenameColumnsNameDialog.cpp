#include "DARenameColumnsNameDialog.h"
#include "ui_DARenameColumnsNameDialog.h"
#include <QMessageBox>
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DARenameColumnsNameDialog
//===================================================
DARenameColumnsNameDialog::DARenameColumnsNameDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DARenameColumnsNameDialog)
{
    ui->setupUi(this);
    m_model = new QStandardItemModel(this);
    ui->tableView->setModel(m_model);
}

DARenameColumnsNameDialog::~DARenameColumnsNameDialog()
{
    delete ui;
}

/**
 * @brief 设置数据名
 * @param name
 */
void DARenameColumnsNameDialog::setDataName(const QString& name)
{
    ui->lineEdit->setText(name);
}

QString DARenameColumnsNameDialog::getDataName() const
{
    return ui->lineEdit->text();
}

/**
 * @brief 设置列名
 * @param names
 */
void DARenameColumnsNameDialog::setColumnsName(const QList< QString >& names)
{
    m_model->clear();
    m_model->setHorizontalHeaderLabels({ tr("name") });
    for (int i = 0; i < names.size(); ++i) {
        m_model->appendRow({ new QStandardItem(names[ i ]) });
    }
}

/**
 * @brief 获取列名
 * @return
 */
QList< QString > DARenameColumnsNameDialog::getColumnsName() const
{
    return m_newCols;
}

QList< QString > DARenameColumnsNameDialog::_getColumnsName() const
{
    QList< QString > res;
    const int r = m_model->rowCount();
    for (int i = 0; i < r; ++i) {
        res.append(m_model->item(i, 0)->text());
    }
    return res;
}

void DARenameColumnsNameDialog::changeEvent(QEvent* e)
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

void DARenameColumnsNameDialog::on_pushButtonOK_clicked()
{
    m_newCols = _getColumnsName();
    //判断是否唯一
    for (int i = 0; i < m_newCols.size(); ++i) {
        if (m_newCols.count(m_newCols[ i ]) > 1) {
            QMessageBox::warning(this,
                                 tr("warning"),  // cn: 警告
                                 tr("Duplicate column name \"%1\",Please reset the column name of column %2")  // cn 列名“%1”存在重复，请重新设置第%2列的列名
                                 .arg(m_newCols[ i ])
                                 .arg(i + 1));
            return;
        }
    }
    QDialog::accept();
}
