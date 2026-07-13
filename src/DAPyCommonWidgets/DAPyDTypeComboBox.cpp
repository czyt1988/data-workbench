#include "DAPyDTypeComboBox.h"
#include "DAPybind11InQt.h"
#include "DALogCategory.h"
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
 * @param c 注意：调用方传入的是 DAPyDType::kind()（kind 字符），int 系列为 'i'、
 *          uint 系列为 'u'，需与 char code（'q'/'l'/'h'/'b' 等）同时覆盖
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
    case 'i':  // int 系列 kind（int8/int16/int32/int64）
    case 'u':  // uint 系列 kind（uint8/uint16/uint32/uint64）
    case 'q':
    case 'Q':
    case 'l':
    case 'L':
    case 'h':
    case 'H':
    case 'b':   // int8 的 char
    case 'B': { // uint8 的 char
        static QIcon s_int(":/PyCommonWidgets/icon/int.svg");
        return s_int;
    } break;
    case '?': { // bool 的 char（bool.svg 不存在，复用 int 图标兜底）
        static QIcon s_bool(":/PyCommonWidgets/icon/int.svg");
        return s_bool;
    } break;
    case 'F':   // complex64
    case 'D': { // complex128（complex.svg 不存在，复用 float 图标兜底）
        static QIcon s_complex(":/PyCommonWidgets/icon/float.svg");
        return s_complex;
    } break;
    case 'U': { // str
        static QIcon s_str(":/PyCommonWidgets/icon/str.svg");
        return s_str;
    } break;
    case 'M': { // datetime64
        static QIcon s_datetime(":/PyCommonWidgets/icon/datetime.svg");
        return s_datetime;
    } break;
    case 'm': { // timedelta64（复用 datetime 图标）
        static QIcon s_datetime(":/PyCommonWidgets/icon/datetime.svg");
        return s_datetime;
    } break;
    case 'O': { // object
        static QIcon s_obj(":/PyCommonWidgets/icon/obj.svg");
        return s_obj;
    } break;
    case 'S': { // bytes（bytes.svg 不存在，复用 str 图标兜底）
        static QIcon s_str(":/PyCommonWidgets/icon/str.svg");
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
    // 基于语义判断，而非裸字符，避免 kind 和 char 语义混淆
    // 注意：bool 的 kind 是 'b'，但 int8 的 char 也是 'b'，直接用 kind 会导致 bool 误入 int 分支
    if (dt.isBool() || dt.isNullableBool()) {
        return getIconByDtypeChar('?');
    }
    if (dt.isInt() || dt.isUInt() || dt.isNullableInt() || dt.isNullableUInt()) {
        return getIconByDtypeChar('i');
    }
    if (dt.isFloat()) {
        return getIconByDtypeChar('f');
    }
    if (dt.isComplex()) {
        return getIconByDtypeChar('F');
    }
    if (dt.isStr() || dt.isNullableString()) {
        return getIconByDtypeChar('U');
    }
    if (dt.isDatetime() || dt.isTimedelta()) {
        return getIconByDtypeChar('M');
    }
    if (dt.isCategorical()) {
        return getIconByDtypeChar('O');
    }
    // 其他类型（object/bytes 等）使用 obj 图标
    return getIconByDtypeChar('O');
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
        // dtype 不在预置列表中，追加一个新条目，避免覆盖已有预置项或静默丢弃
        daWarning << "DType not in preset list, appending temporary item:" << dt.name().toStdString();
        QString name = dt.displayName();
        QIcon icon   = getIconByDType(dt);
        addItem(icon, name, dt.name());
        setCurrentIndex(count() - 1);
    }
}

void DAPyDTypeComboBox::onComboxCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    DAPyDType dt = selectedDType();
    emit currentDTypeChanged(dt);
}
