#ifndef DAPARAMDEF_H
#define DAPARAMDEF_H
#include "DAGuiAPI.h"
#include <QString>
#include <QVariant>
#include <QVariantHash>
#include <QList>
#include <QPair>
#include <QStringList>

namespace DA
{

/**
 * @brief Python节点参数定义
 *
 * 轻量级数据结构，用于描述Python节点的参数信息。
 * 替代原DAParameterDescriptor，保留DAGui参数编辑器所需的属性系统。
 * 包含参数名称、类型、描述、默认值及扩展属性（enum/min/max/step/decimals/filter）。
 */
struct DAGUI_API DAParamDef
{
    QString name;           ///< 参数名称
    QString type;           ///< 参数类型 (str/int/float/bool/list/dict)
    QString description;    ///< 参数描述
    QVariant defaultValue;  ///< 默认值（可为无效QVariant表示无默认值）
    int propertyId;         ///< 属性面板中的属性ID（由面板构建器设置）

    /**
     * @brief 记录扩展属性，用于编辑器配置
     *
     * 支持的属性键：
     * - enum (PropertyName_Enum): QList<QPair<QString,int>> 或 QStringList
     * - min (PropertyName_Min): int/double
     * - max (PropertyName_Max): int/double
     * - step (PropertyName_Step): int/double
     * - decimals (PropertyName_Decimals): int
     * - filter (PropertyName_Filter): QString
     */
    QVariantHash propertys;

    // 属性名常量
    static const QString PropertyName_Enum;
    static const QString PropertyName_Min;
    static const QString PropertyName_Max;
    static const QString PropertyName_Step;
    static const QString PropertyName_Decimals;
    static const QString PropertyName_Filter;

    // 默认构造
    DAParamDef();

    // 判断是否有属性
    bool hasProperty(const QString& propName) const;

    // ---- 枚举属性 ----
    QStringList getEnumStringListProperty() const;
    QList< QPair< QString, int > > getEnumListProperty() const;
    bool hasEnumProperty() const;

    // ---- min属性 ----
    int getMinProperty(bool* isSuccess = nullptr) const;
    double getMinFProperty(bool* isSuccess = nullptr) const;

    // ---- max属性 ----
    int getMaxProperty(bool* isSuccess = nullptr) const;
    double getMaxFProperty(bool* isSuccess = nullptr) const;

    // ---- step属性 ----
    int getStepProperty(bool* isSuccess = nullptr) const;
    double getStepFProperty(bool* isSuccess = nullptr) const;

    // ---- decimals属性 ----
    int getDecimalsProperty(bool* isSuccess = nullptr) const;

    // ---- Filter属性 ----
    QString getFilterProperty() const;

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

#endif  // DAPARAMDEF_H