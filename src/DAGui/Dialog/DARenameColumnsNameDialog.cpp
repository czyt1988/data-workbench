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

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DARenameColumnsNameDialog::DARenameColumnsNameDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DARenameColumnsNameDialog)
{
    ui->setupUi(this);
    mModel = new QStandardItemModel(this);
    ui->tableView->setModel(mModel);
}

/**
 * @brief 析构函数
 */
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

/**
 * @brief 获取数据名
 * @return 数据名
 */
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
    mModel->clear();
    mModel->setHorizontalHeaderLabels({ tr("name") });  // cn:名称
    for (int i = 0; i < names.size(); ++i) {
        mModel->appendRow({ new QStandardItem(names[ i ]) });
    }
}

/**
 * @brief 获取列名
 * @return
 */
QList< QString > DARenameColumnsNameDialog::getColumnsName() const
{
    return mNewCols;
}

/**
 * @brief 从表格模型中读取列名
 * @return 列名列表
 */
QList< QString > DARenameColumnsNameDialog::getColumnsNameInternal() const
{
    QList< QString > res;
    const int r = mModel->rowCount();
    for (int i = 0; i < r; ++i) {
        res.append(mModel->item(i, 0)->text());
    }
    return res;
}

/**
 * @brief 语言变更事件处理
 * @param e 事件指针
 */
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

/**
 * @brief 确定按钮点击槽函数，校验列名唯一性后接受对话框
 */
void DARenameColumnsNameDialog::on_pushButtonOK_clicked()
{
    mNewCols = getColumnsNameInternal();
    //判断是否唯一
    for (int i = 0; i < mNewCols.size(); ++i) {
        if (mNewCols.count(mNewCols[ i ]) > 1) {
            QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Duplicate column name \"%1\", please reset the column name of column %2")  // cn:列名"%1"存在重复，请重新设置第%2列的列名
                                 .arg(mNewCols[ i ])
                                 .arg(i + 1));
            return;
        }
    }
    QDialog::accept();
}
