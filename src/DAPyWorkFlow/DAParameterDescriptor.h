#ifndef DAPARAMETERDESCRIPTOR_H
#define DAPARAMETERDESCRIPTOR_H
#include <type_traits>
#include <QString>
#include <QVariant>
#include <QList>
#include <QPair>
#include <QHash>
#include "DAPyWorkFlowAPI.h"
namespace DA
{

/**
 * @brief Python 节点参数描述符
 *
 * 轻量级数据结构，用于解析并存储 Python 节点的参数描述信息。
 * 包含参数名称、类型、描述、默认值等基础字段，并有扩展能力。
 *
 * @see DAPyParamTypeHelper
 */
class DAPYWORKFLOW_API DAParameterDescriptor
{
public:
    QString name;           ///< 参数名称
    QString type;           ///< 参数类型 (str/int/float/bool/list/dict)
    QString description;    ///< 参数描述
    QVariant defaultValue;  ///< 默认值（可为无效 QVariant 表示无默认值）
    /**
     * @brief 记录属性，有些参数，例如spinbox，需要min,max属性，则可以通过此属性来设置
     * 当前支持的属性：
     * - enums [PropertyName_Enum]
     *  value 1:QList<QPair<QString,int>>
     *  value 2:QStringList
     *  作用，存储枚举，使用QComboBox按顺序显示，QComboBox的data设置为int
     *
     * - min [PropertyName_Min]:
     *  value:int/double
     *  作用，QSpinBox/QDoubleSpinBox存储最小值
     *
     * - max [PropertyName_Max]:
     *  value:int/double
     *  作用，QSpinBox/QDoubleSpinBox存储最大值
     *
     * - step [PropertyName_Step]:
     *  value:int/double
     *  作用，QSpinBox/QDoubleSpinBox存储步长
     *
     * - decimals [PropertyName_Decimals]:
     *  value:int
     *  作用，QDoubleSpinBox的精度设置
     *
     * - filter [PropertyName_Filter]
     *   value:QString
     *   作用，DAFilePathEditWidget的文件过滤器设置
     */
    QVariantHash propertys;
    int propertyId;  ///< 属性面板中的属性 ID（由面板构建器设置）

    // 属性名常量
    static const QString PropertyName_Enum;
    static const QString PropertyName_Min;
    static const QString PropertyName_Max;
    static const QString PropertyName_Step;
    static const QString PropertyName_Decimals;
    static const QString PropertyName_Filter;

public:
    // 默认构造函数
    DAParameterDescriptor();

    // 判断是否有属性
    bool hasProperty(const QString& propName) const;

    // ---- 枚举属性 ----
    QStringList getEnumStringListProperty() const;
    QList< QPair< QString, int > > getEnumListProperty() const;
    void setEnumStringListProperty(const QStringList& enumList);
    void setEnumStringListProperty(const QList< QPair< QString, int > >& pairList);
    bool hasEnumProperty() const;

    // ---- min属性 ----
    int getMinProperty(bool* isSuccess = nullptr) const;
    double getMinFProperty(bool* isSuccess = nullptr) const;
    void setMinProperty(int min);
    void setMinProperty(double min);

    // ---- max属性 ----
    int getMaxProperty(bool* isSuccess = nullptr) const;
    double getMaxFProperty(bool* isSuccess = nullptr) const;
    void setMaxProperty(int max);
    void setMaxProperty(double max);

    // ---- step属性 ----
    int getStepProperty(bool* isSuccess = nullptr) const;
    double getStepFProperty(bool* isSuccess = nullptr) const;
    void setStepProperty(int step);
    void setStepProperty(double step);

    // ---- decimals属性 ----
    int getDecimalsProperty(bool* isSuccess = nullptr) const;
    void setDecimalsProperty(int decimals);

    // ---- Filter属性 ----
    QString getFilterProperty() const;
    void setFilterProperty(const QString& filters);

    // ---- 原始描述符属性（rawDescriptor） ----
    void setRawDescriptor(const QVariantHash& props);
    QVariantHash getRawDescriptor() const;

    // ---- 默认值快捷方法 ----
    bool hasDefaultValue() const;
    QString defaultValueToString() const;
    int defaultValueToInt(bool* isSuccess = nullptr) const;
    double defaultValueToDouble(bool* isSuccess = nullptr) const;
    bool defaultValueToBool() const;
};

namespace Detail
{

template< typename T >
T getNumericProperty(const QVariantHash& propertys, const QString& propName, T fallback, bool* isSuccess)
{
    if (isSuccess) {
        *isSuccess = false;
    }
    QVariant val = propertys.value(propName);
    if (!val.isValid()) {
        return fallback;
    }
    if constexpr (std::is_same_v< T, int >) {
        return val.toInt(isSuccess);
    } else if constexpr (std::is_same_v< T, double >) {
        return val.toDouble(isSuccess);
    } else if constexpr (std::is_same_v< T, QString >) {
        return val.toString();
    }
    return val.value< T >();
}

}  // namespace Detail
}  // namespace DA

#endif  // DAPARAMETERDESCRIPTOR_H
