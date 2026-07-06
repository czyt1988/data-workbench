#include "DAPyDTypeComboBox.h"
#include "DAPybind11InQt.h"
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
    initItems();
    setCurrentIndex(-1);
    connect(this, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAPyDTypeComboBox::onComboxCurrentIndexChanged);
}

/**
 * @brief 初始化下拉框选项
 */
void DAPyDTypeComboBox::initItems()
{
    clear();
    addItem(getIconByDtypeChar('d'), tr("float64"), "d");  // cn:float64
    addItem(getIconByDtypeChar('f'), tr("float32"), "f");  // cn:float32
    addItem(getIconByDtypeChar('e'), tr("float16"), "e");  // cn:float16
    insertSeparator(count());
    addItem(getIconByDtypeChar('q'), tr("int64"), "q");  // cn:int64
    addItem(getIconByDtypeChar('Q'), tr("uint64"), "Q");  // cn:uint64
    addItem(getIconByDtypeChar('l'), tr("int32"), "l");  // cn:int32
    addItem(getIconByDtypeChar('L'), tr("uint32"), "L");  // cn:uint32
    addItem(getIconByDtypeChar('h'), tr("int16"), "h");  // cn:int16
    addItem(getIconByDtypeChar('H'), tr("uint16"), "H");  // cn:uint16
    addItem(getIconByDtypeChar('b'), tr("int8"), "b");  // cn:int8
    addItem(getIconByDtypeChar('B'), tr("uint8"), "B");  // cn:uint8
    insertSeparator(count());
    addItem(getIconByDtypeChar('U'), tr("str"), "U");  // cn:str
    insertSeparator(count());
    addItem(getIconByDtypeChar('?'), tr("bool"), "?");  // cn:bool
    insertSeparator(count());
    addItem(getIconByDtypeChar('F'), tr("complex64"), "F");  // cn:complex64
    addItem(getIconByDtypeChar('D'), tr("complex128"), "D");  // cn:complex128
    insertSeparator(count());
    addItem(getIconByDtypeChar('M'), tr("datetime64"), "M");  // cn:datetime64
    addItem(getIconByDtypeChar('m'), tr("timedelta64"), "m");  // cn:timedelta64
    insertSeparator(count());
    addItem(getIconByDtypeChar('S'), tr("bytes"), "S");  // cn:bytes
    addItem(getIconByDtypeChar('O'), tr("object"), "O");  // cn:object
    insertSeparator(count());
    addItem(getIconByDtypeChar('q'), tr("Int64 (nullable)"), "Int64");  // cn:Int64（可空）
    addItem(getIconByDtypeChar('l'), tr("Int32 (nullable)"), "Int32");  // cn:Int32（可空）
    addItem(getIconByDtypeChar('h'), tr("Int16 (nullable)"), "Int16");  // cn:Int16（可空）
    addItem(getIconByDtypeChar('b'), tr("Int8 (nullable)"), "Int8");  // cn:Int8（可空）
    addItem(getIconByDtypeChar('Q'), tr("UInt64 (nullable)"), "UInt64");  // cn:UInt64（可空）
    addItem(getIconByDtypeChar('L'), tr("UInt32 (nullable)"), "UInt32");  // cn:UInt32（可空）
    addItem(getIconByDtypeChar('H'), tr("UInt16 (nullable)"), "UInt16");  // cn:UInt16（可空）
    addItem(getIconByDtypeChar('B'), tr("UInt8 (nullable)"), "UInt8");  // cn:UInt8（可空）
    insertSeparator(count());
    addItem(getIconByDtypeChar('?'), tr("boolean (nullable)"), "boolean");  // cn:boolean（可空）
    addItem(getIconByDtypeChar('U'), tr("string (nullable)"), "string");  // cn:string（可空）
    addItem(getIconByDtypeChar('O'), tr("category"), "category");  // cn:category
}

/**
 * @brief 获取当前选中的 dtype
 * @return
 */
DAPyDType DAPyDTypeComboBox::selectedDType() const
{
    QString dtypeStr = currentData().toString();
    if (dtypeStr.isEmpty()) {
        return DAPyDType();
    }
    if (dtypeStr.length() == 1) {
        return DAPyDType(dtypeStr);
    }
    try {
        pybind11::module pd = pybind11::module::import("pandas");
        pybind11::object dtype_obj;
        if (dtypeStr == "string") {
            dtype_obj = pd.attr("StringDtype")();
        } else if (dtypeStr == "boolean") {
            dtype_obj = pd.attr("BooleanDtype")();
        } else if (dtypeStr == "category") {
            dtype_obj = pd.attr("CategoricalDtype")();
        } else if (dtypeStr.startsWith("Int") || dtypeStr.startsWith("UInt")) {
            std::string dtype_name = dtypeStr.toStdString() + "Dtype";
            if (pybind11::hasattr(pd, dtype_name.c_str())) {
                dtype_obj = pd.attr(dtype_name.c_str())();
            } else {
                pybind11::module arr = pybind11::module::import("pandas.core.arrays.integer");
                dtype_obj             = arr.attr(dtype_name.c_str())();
            }
        } else {
            return DAPyDType(dtypeStr);
        }
        return DAPyDType::fromObject(dtype_obj);
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return DAPyDType();
}

/**
 * @brief 通过 numpy.char 获取图标
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
 * @brief 通过 DAPyDType 获取图标
 * @param dt
 * @return
 */
QIcon DAPyDTypeComboBox::getIconByDType(const DAPyDType& dt)
{
    if (dt.isNone()) {
        return QIcon();
    }
    return getIconByDtypeChar(dt.kind());
}

/**
 * @brief 查找 dtype 对应的索引
 * @param dt
 * @return
 */
int DAPyDTypeComboBox::findDTypeIndex(const DAPyDType& dt) const
{
    if (dt.isNone()) {
        return -1;
    }
    QString name = dt.name();
    if (dt.isExtensionDtype()) {
        return findData(name);
    }
    char c = dt.char_();
    return findData(QString(QChar(c)));
}

/**
 * @brief 设置当前的 dtype
 *
 * 此操作将会发射 currentDTypeChanged 信号
 * @param dt
 */
void DAPyDTypeComboBox::setCurrentDType(const DAPyDType& dt)
{
    if (dt.isNone()) {
        setCurrentIndex(-1);
        return;
    }
    int index = findDTypeIndex(dt);
    if (index != -1) {
        setCurrentIndex(index);
    } else {
        QString name = dt.displayName();
        setItemText(currentIndex(), name);
        setItemData(currentIndex(), dt.name());
    }
}

void DAPyDTypeComboBox::onComboxCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    DAPyDType dt = selectedDType();
    emit currentDTypeChanged(dt);
}
