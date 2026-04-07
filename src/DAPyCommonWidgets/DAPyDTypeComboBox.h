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
 * addItem(tr("float64"), "d");
 * addItem(tr("float32"), "f");
 * addItem(tr("float16"), "e");
 * insertSeparator(count());
 * addItem(tr("int64"), "q");
 * addItem(tr("uint64"), "Q");
 * addItem(tr("int32"), "l");
 * addItem(tr("uint32"), "L");
 * addItem(tr("int16"), "h");
 * addItem(tr("uint16"), "H");
 * addItem(tr("int8"), "b");
 * addItem(tr("uint8"), "B");
 * insertSeparator(count());
 * addItem(tr("str"), "U");
 * insertSeparator(count());
 * addItem(tr("bool"), "?");
 * insertSeparator(count());
 * addItem(tr("complex64"), "F");
 * addItem(tr("complex128"), "D");
 * insertSeparator(count());
 * addItem(tr("datetime64"), "M");
 * addItem(tr("timedelta64"), "m");
 * insertSeparator(count());
 * addItem(tr("bytes"), "S");
 * addItem(tr("object"), "O");
 * insertSeparator(count());
 * // pandas 扩展类型
 * addItem(tr("Int64 (nullable)"), "Int64");
 * addItem(tr("Int32 (nullable)"), "Int32");
 * addItem(tr("Int16 (nullable)"), "Int16");
 * addItem(tr("Int8 (nullable)"), "Int8");
 * addItem(tr("UInt64 (nullable)"), "UInt64");
 * addItem(tr("UInt32 (nullable)"), "UInt32");
 * addItem(tr("UInt16 (nullable)"), "UInt16");
 * addItem(tr("UInt8 (nullable)"), "UInt8");
 * insertSeparator(count());
 * addItem(tr("boolean (nullable)"), "boolean");
 * addItem(tr("string (nullable)"), "string");
 * addItem(tr("category"), "category");
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
