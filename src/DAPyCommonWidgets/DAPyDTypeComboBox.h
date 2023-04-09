#ifndef DAPYDTYPECOMBOBOX_H
#define DAPYDTYPECOMBOBOX_H
#include "numpy/DAPyDType.h"
#include "DAPyCommonWidgetsAPI.h"
#include <QComboBox>
#include <QCoreApplication>
namespace DA
{
/**
 * @brief dtype选择器
 * 此combox构建时默认插入如下信息:
 *
 * @code
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
 * @endcode
 */
class DAPYCOMMONWIDGETS_API DAPyDTypeComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit DAPyDTypeComboBox(QWidget* parent = nullptr);

public:
    //获取选中的dtype
    DAPyDType selectedDType() const;
    //通过char获取图标
    static QIcon getIconByDtypeChar(char c);
public slots:
    //设置当前的dtype
    void setCurrentDType(const DAPyDType& dt);
private slots:
    void onComboxCurrentIndexChanged(int index);
signals:
    /**
     * @brief 当前选择的dtype改变信号
     * @param dt
     */
    void currentDTypeChanged(const DAPyDType& dt);
};
}  // namespace DA
#endif  // DAPYDTYPECOMBOBOX_H
