#include "DADialogInsertNewColumn.h"
#include "ui_DADialogInsertNewColumn.h"

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
    if (dt.isNone()) {
        bool isok = false;
        {
            int v = str.toInt(&isok);
            if (isok) {
                return v;
            }
        }
        {
            double v = str.toDouble(&isok);
            if (isok) {
                return v;
            }
        }
        return str;
    }
    switch (dt.char_()) {
    case '?': {
        if (str.toLower() == "false" || str == "0") {
            return false;
        } else {
            return true;
        }
        break;
    }
    case 'b':
    case 'h': {
        bool isok = false;
        short v   = str.toShort(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'l': {
        bool isok = false;
        int v     = str.toInt(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'q': {
        bool isok   = false;
        long long v = str.toLongLong(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'B':
    case 'H': {
        bool isok        = false;
        unsigned short v = str.toUShort(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'L': {
        bool isok      = false;
        unsigned int v = str.toUInt(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'Q': {
        bool isok            = false;
        unsigned long long v = str.toULongLong(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'e':
    case 'f': {
        bool isok = false;
        float v   = str.toFloat(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'd': {
        bool isok = false;
        double v  = str.toDouble(&isok);
        if (isok) {
            return v;
        }
    } break;
    case 'M': {
        // 日期
        QDateTime datetime = QDateTime::fromString(str, "yyyy-MM-dd HH:mm:ss");
        if (datetime.isValid()) {
            return datetime;
        }
    } break;
    default:
        break;
    }
    return str;
}

void DADialogInsertNewColumn::onCurrentDtypeChanged(const DAPyDType& dt)
{
    if (!dt.isNumeral() || !dt.isDatetime()) {
        // 如果不是数字也不是日期，range不允许设置
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
