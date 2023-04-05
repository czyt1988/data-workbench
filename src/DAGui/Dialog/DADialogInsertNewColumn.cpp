#include "DADialogInsertNewColumn.h"
#include "ui_DADialogInsertNewColumn.h"
#include "DAPybind11QtTypeCast.h"
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DADialogInsertNewColumn
//===================================================
DADialogInsertNewColumn::DADialogInsertNewColumn(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogInsertNewColumn)
{
    ui->setupUi(this);
    ui->radioButtonFillInSame->setChecked(true);
    ui->groupBoxRange->setVisible(false);
    connect(ui->comboBoxDType, &DAPyDTypeComboBox::currentDTypeChanged, this, &DADialogInsertNewColumn::onCurrentDtypeChanged);
}

DADialogInsertNewColumn::~DADialogInsertNewColumn()
{
    delete ui;
}

DAPyDType DADialogInsertNewColumn::getDType() const
{
    return ui->comboBoxDType->selectedDType();
}

bool DADialogInsertNewColumn::isFullValueMode() const
{
    return ui->radioButtonFillInSame->isChecked();
}

bool DADialogInsertNewColumn::isRangeMode() const
{
    return ui->radioButtonRange->isChecked();
}

/**
 * @brief 获取列名
 * @return
 */
QString DADialogInsertNewColumn::getName() const
{
    return ui->lineEditName->text();
}

/**
 * @brief 获取默认值
 * @return
 */
QVariant DADialogInsertNewColumn::getDefaultValue() const
{
    return fromString(ui->lineEditDefaultValue->text(), getDType());
}

QVariant DADialogInsertNewColumn::getStartValue() const
{
    DAPyDType dt = getDType();
    if (dt.isDatetime()) {
        return fromString(ui->dateTimeEditStart->text(), getDType());
    }
    return fromString(ui->lineEditStart->text(), dt);
}

QVariant DADialogInsertNewColumn::getStopValue() const
{
    DAPyDType dt = getDType();
    if (dt.isDatetime()) {
        return fromString(ui->dateTimeEditStop->text(), getDType());
    }
    return fromString(ui->lineEditStop->text(), dt);
}

/**
 * @brief 根据dtype把字符串转换为QVariant
 * @param str
 * @param dt
 * @return
 */
QVariant DADialogInsertNewColumn::fromString(const QString& str, const DAPyDType& dt) const
{
    if (str.isEmpty()) {
        return QVariant();
    }
    return DA::PY::toVariant(str, dt);
}

void DADialogInsertNewColumn::onCurrentDtypeChanged(const DAPyDType& dt)
{
    if (!dt.isNumeral() || !dt.isDatetime()) {
        //如果不是数字也不是日期，range不允许设置
        if (ui->radioButtonRange->isChecked()) {
            ui->radioButtonFillInSame->setChecked(true);
        }
    }
    if (dt.isDatetime()) {
        ui->stackedWidget->setCurrentIndex(1);
    } else {
        ui->stackedWidget->setCurrentIndex(0);
    }
}
