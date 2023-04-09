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
    addItem(getIconByDtypeChar('d'), tr("float64"), "d");
    addItem(getIconByDtypeChar('f'), tr("float32"), "f");
    addItem(getIconByDtypeChar('e'), tr("float16"), "e");
    insertSeparator(count());
    addItem(getIconByDtypeChar('q'), tr("int64"), "q");
    addItem(getIconByDtypeChar('Q'), tr("uint64"), "Q");
    addItem(getIconByDtypeChar('l'), tr("int32"), "l");
    addItem(getIconByDtypeChar('L'), tr("uint32"), "L");
    addItem(getIconByDtypeChar('h'), tr("int16"), "h");
    addItem(getIconByDtypeChar('H'), tr("uint16"), "H");
    addItem(getIconByDtypeChar('b'), tr("int8"), "b");
    addItem(getIconByDtypeChar('B'), tr("uint8"), "B");
    insertSeparator(count());
    addItem(getIconByDtypeChar('U'), tr("str"), "U");
    insertSeparator(count());
    addItem(getIconByDtypeChar('?'), tr("bool"), "?");
    insertSeparator(count());
    addItem(getIconByDtypeChar('F'), tr("complex64"), "F");
    addItem(getIconByDtypeChar('D'), tr("complex128"), "D");
    insertSeparator(count());
    addItem(getIconByDtypeChar('M'), tr("datetime64"), "M");
    addItem(getIconByDtypeChar('m'), tr("timedelta64"), "m");
    insertSeparator(count());
    addItem(getIconByDtypeChar('S'), tr("bytes"), "S");
    addItem(getIconByDtypeChar('O'), tr("object"), "O");
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
 * @brief 通过numpy.char获取图标
 * @param c
 * @return
 */
QIcon DAPyDTypeComboBox::getIconByDtypeChar(char c)
{
    switch (c) {
    case 'd':
    case 'f':
    case 'e': {
        static QIcon s_float(":/PyCommonWidgets/icon/float.svg");
        return s_float;
    } break;
    case 'q':
    case 'Q':
    case 'l':
    case 'L':
    case 'h':
    case 'H':
    case 'b':
    case 'B': {
        static QIcon s_int(":/PyCommonWidgets/icon/int.svg");
        return s_int;
    } break;
    case 'U': {
        static QIcon s_str(":/PyCommonWidgets/icon/str.svg");
        return s_str;
    } break;
    case 'M': {
        static QIcon s_str(":/PyCommonWidgets/icon/datetime.svg");
        return s_str;
    } break;
    case 'O': {
        static QIcon s_str(":/PyCommonWidgets/icon/obj.svg");
        return s_str;
    } break;
    default:
        break;
    }
    return QIcon();
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
