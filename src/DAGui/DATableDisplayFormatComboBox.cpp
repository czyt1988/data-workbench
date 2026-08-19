#include "DATableDisplayFormatComboBox.h"
#include "numpy/DAPyDType.h"
#include <QSignalBlocker>
namespace DA
{

/**
 * @brief 构造函数：装载全部类别项并接内部信号转发
 * @param parent 父控件
 */
DATableDisplayFormatComboBox::DATableDisplayFormatComboBox(QWidget* parent) : QComboBox(parent)
{
    populate();
    connect(this,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            [ this ](int index) {
                if (index < 0) {
                    return;
                }
                QVariant v = itemData(index);
                if (!v.isValid()) {
                    return;
                }
                Q_EMIT currentFormatCategoryChanged(
                    static_cast< DATableDisplayFormat::Category >(v.toInt()));
            });
}

/**
 * @brief 装载全部类别项（block signals 避免初始填充触发信号）
 */
void DATableDisplayFormatComboBox::populate()
{
    QSignalBlocker b(this);
    Q_UNUSED(b);
    clear();
    addItem(tr("General"), static_cast< int >(DATableDisplayFormat::General));            // cn:常规
    addItem(tr("Number"), static_cast< int >(DATableDisplayFormat::Number));              // cn:数值
    addItem(tr("Scientific"), static_cast< int >(DATableDisplayFormat::Scientific));      // cn:科学计数法
    addItem(tr("Percentage"), static_cast< int >(DATableDisplayFormat::Percent));        // cn:百分比
    addItem(tr("Date/Time"), static_cast< int >(DATableDisplayFormat::DateTime));         // cn:日期时间
    addItem(tr("Datetime as Number"), static_cast< int >(DATableDisplayFormat::DatetimeAsNumber));  // cn:时间显示为数字
    addItem(tr("Text"), static_cast< int >(DATableDisplayFormat::Text));                  // cn:文本
    setCurrentIndex(0);
}

/**
 * @brief 按列 dtype 重建类别项
 *
 * 仅保留适用类别：General/Text 永远在；Number/Scientific/Percentage 仅数值列；
 * Date/Time/Datetime as Number 仅日期时间列。dtype 无效（混合选择）时仅 General/Text。
 * 重建后保持当前选中类别（若仍存在），否则回退 General。
 * @param dt 列 dtype
 */
void DATableDisplayFormatComboBox::updateDType(const DAPyDType& dt)
{
    bool noneType = dt.isNone();
    bool numeric  = !noneType && (dt.isFloat() || dt.isInt() || dt.isUInt() || dt.isNullableInt()
                                 || dt.isNullableUInt());
    bool datetime = !noneType && (dt.isDatetime() || dt.isDatetimeTZ());

    // 记录当前类别，重建后尝试保持
    DATableDisplayFormat::Category prev = currentCategory();

    QSignalBlocker b(this);
    Q_UNUSED(b);
    clear();
    addItem(tr("General"), static_cast< int >(DATableDisplayFormat::General));  // cn:常规
    if (numeric) {
        addItem(tr("Number"), static_cast< int >(DATableDisplayFormat::Number));             // cn:数值
        addItem(tr("Scientific"), static_cast< int >(DATableDisplayFormat::Scientific));     // cn:科学计数法
        addItem(tr("Percentage"), static_cast< int >(DATableDisplayFormat::Percent));       // cn:百分比
    }
    if (datetime) {
        addItem(tr("Date/Time"), static_cast< int >(DATableDisplayFormat::DateTime));  // cn:日期时间
        addItem(tr("Datetime as Number"),
                static_cast< int >(DATableDisplayFormat::DatetimeAsNumber));  // cn:时间显示为数字
    }
    addItem(tr("Text"), static_cast< int >(DATableDisplayFormat::Text));  // cn:文本

    // 保持当前类别（若仍在列表中），否则回退 General
    int idx = indexOfCategory(prev);
    setCurrentIndex(idx >= 0 ? idx : 0);
}

/**
 * @brief 反向同步当前格式（block signals）
 * @param fmt 当前格式，invalid→选中 General
 */
void DATableDisplayFormatComboBox::setCurrentFormat(const DATableDisplayFormat& fmt)
{
    QSignalBlocker b(this);
    Q_UNUSED(b);
    DATableDisplayFormat::Category c =
        fmt.isValid() ? fmt.category() : DATableDisplayFormat::General;
    int idx = indexOfCategory(c);
    if (idx >= 0) {
        setCurrentIndex(idx);
    }
}

/**
 * @brief 当前选中类别
 * @return 类别
 */
DATableDisplayFormat::Category DATableDisplayFormatComboBox::currentCategory() const
{
    int idx = currentIndex();
    if (idx < 0) {
        return DATableDisplayFormat::General;
    }
    return static_cast< DATableDisplayFormat::Category >(itemData(idx).toInt());
}

int DATableDisplayFormatComboBox::indexOfCategory(DATableDisplayFormat::Category c) const
{
    int target = static_cast< int >(c);
    for (int i = 0; i < count(); ++i) {
        if (itemData(i).toInt() == target) {
            return i;
        }
    }
    return -1;
}

}  // end of namespace DA
