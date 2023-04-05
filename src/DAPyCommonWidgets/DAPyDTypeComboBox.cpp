#include "DAPyDTypeComboBox.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyDTypeComboBox
//===================================================
DAPyDTypeComboBox::DAPyDTypeComboBox(QWidget* parent) : QComboBox(parent)
{
    setEditable(false);
    clear();
    addItem(tr("float64"), "d");
    addItem(tr("float32"), "f");
    addItem(tr("float16"), "e");
    insertSeparator(count());
    addItem(tr("int64"), "q");
    addItem(tr("uint64"), "Q");
    addItem(tr("int32"), "l");
    addItem(tr("uint32"), "L");
    addItem(tr("int16"), "h");
    addItem(tr("uint16"), "H");
    addItem(tr("int8"), "b");
    addItem(tr("uint8"), "B");
    insertSeparator(count());
    addItem(tr("str"), "U");
    insertSeparator(count());
    addItem(tr("bool"), "?");
    insertSeparator(count());
    addItem(tr("complex64"), "F");
    addItem(tr("complex128"), "D");
    insertSeparator(count());
    addItem(tr("datetime64"), "M");
    addItem(tr("timedelta64"), "m");
    insertSeparator(count());
    addItem(tr("bytes"), "S");
    addItem(tr("object"), "O");
    setCurrentIndex(-1);
    connect(this, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAPyDTypeComboBox::onComboxCurrentIndexChanged);
}

/**
 * @brief 获取当前选中的dtype
 * @return
 */
DAPyDType DAPyDTypeComboBox::selectedDType() const
{
    QString dtypeChar = currentData().toString();
    return DAPyDType(dtypeChar);
}

/**
 * @brief 设置当前的dtype
 *
 * 此操作将会发射currentDTypeChanged信号
 * @param dt
 */
void DAPyDTypeComboBox::setCurrentDType(const DAPyDType& dt)
{
    if (dt.isNone()) {
        setCurrentIndex(-1);
        return;
    }
    int index = findData(QString(dt.char_()));
    if (index != -1) {
        setCurrentIndex(index);
    }
}

void DAPyDTypeComboBox::onComboxCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    DAPyDType dt = selectedDType();
    emit currentDTypeChanged(dt);
}
