#ifndef DAPYDTYPECOMBOBOX_H
#define DAPYDTYPECOMBOBOX_H
#include "numpy/DAPyDType.h"
#include "DAPyCommonWidgetsAPI.h"
#include <QComboBox>
#include <QCoreApplication>
namespace DA
{
/**
 * @brief dtype 选择器
 *
 * 此 ComboBox 支持两种类型的 dtype 选择：
 * 1. numpy dtype - 标准 numpy 数据类型
 * 2. pandas 扩展类型 - 如 StringDtype, Int64Dtype, BooleanDtype 等
 *
 * 构建时默认插入如下信息:
 *
 * @code
 * // numpy dtype
 * addItem(tr("float64"), "d");  // cn:float64
 * addItem(tr("float32"), "f");  // cn:float32
 * addItem(tr("float16"), "e");  // cn:float16
 * insertSeparator(count());
 * addItem(tr("int64"), "q");  // cn:int64
 * addItem(tr("uint64"), "Q");  // cn:uint64
 * addItem(tr("int32"), "l");  // cn:int32
 * addItem(tr("uint32"), "L");  // cn:uint32
 * addItem(tr("int16"), "h");  // cn:int16
 * addItem(tr("uint16"), "H");  // cn:uint16
 * addItem(tr("int8"), "b");  // cn:int8
 * addItem(tr("uint8"), "B");  // cn:uint8
 * insertSeparator(count());
 * addItem(tr("str"), "U");  // cn:str
 * insertSeparator(count());
 * addItem(tr("bool"), "?");  // cn:bool
 * insertSeparator(count());
 * addItem(tr("complex64"), "F");  // cn:complex64
 * addItem(tr("complex128"), "D");  // cn:complex128
 * insertSeparator(count());
 * addItem(tr("datetime64"), "M");  // cn:datetime64
 * addItem(tr("timedelta64"), "m");  // cn:timedelta64
 * insertSeparator(count());
 * addItem(tr("bytes"), "S");  // cn:bytes
 * addItem(tr("object"), "O");  // cn:object
 * insertSeparator(count());
 * // pandas 扩展类型
 * addItem(tr("Int64 (nullable)"), "Int64");  // cn:Int64（可空）
 * addItem(tr("Int32 (nullable)"), "Int32");  // cn:Int32（可空）
 * addItem(tr("Int16 (nullable)"), "Int16");  // cn:Int16（可空）
 * addItem(tr("Int8 (nullable)"), "Int8");  // cn:Int8（可空）
 * addItem(tr("UInt64 (nullable)"), "UInt64");  // cn:UInt64（可空）
 * addItem(tr("UInt32 (nullable)"), "UInt32");  // cn:UInt32（可空）
 * addItem(tr("UInt16 (nullable)"), "UInt16");  // cn:UInt16（可空）
 * addItem(tr("UInt8 (nullable)"), "UInt8");  // cn:UInt8（可空）
 * insertSeparator(count());
 * addItem(tr("boolean (nullable)"), "boolean");  // cn:boolean（可空）
 * addItem(tr("string (nullable)"), "string");  // cn:string（可空）
 * addItem(tr("category"), "category");  // cn:category
 * @endcode
 */
class DAPYCOMMONWIDGETS_API DAPyDTypeComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit DAPyDTypeComboBox(QWidget* parent = nullptr);

public:
    DAPyDType selectedDType() const;
    static QIcon getIconByDtypeChar(char c);
    static QIcon getIconByDType(const DAPyDType& dt);

public Q_SLOTS:
    void setCurrentDType(const DAPyDType& dt);

private Q_SLOTS:
    void onComboxCurrentIndexChanged(int index);

Q_SIGNALS:
    void currentDTypeChanged(const DAPyDType& dt);

private:
    void initItems();
    int findDTypeIndex(const DAPyDType& dt) const;
};
}  // namespace DA
#endif  // DAPYDTYPECOMBOBOX_H
