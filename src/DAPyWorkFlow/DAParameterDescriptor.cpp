#include "DAParameterDescriptor.h"

namespace DA
{

const QString DAParameterDescriptor::PropertyName_Enum     = QStringLiteral("enum");
const QString DAParameterDescriptor::PropertyName_Min      = QStringLiteral("min");
const QString DAParameterDescriptor::PropertyName_Max      = QStringLiteral("max");
const QString DAParameterDescriptor::PropertyName_Step     = QStringLiteral("step");
const QString DAParameterDescriptor::PropertyName_Decimals = QStringLiteral("decimals");
const QString DAParameterDescriptor::PropertyName_Filter   = QStringLiteral("filter");
//===============================================================
// 内部模板辅助
//===============================================================

//===============================================================
// 构造与基础方法
//===============================================================

/**
 * @brief 默认构造函数
 *
 * 初始化 propertyId 为 0，其余字段为空。
 */
DAParameterDescriptor::DAParameterDescriptor() : propertyId(0)
{
}

/**
 * @brief 判断是否有属性
 *
 * @param[in] propName 属性名
 * @return 存在该属性返回 true
 */
bool DAParameterDescriptor::hasProperty(const QString& propName) const
{
    return propertys.contains(propName);
}

//===============================================================
// 枚举属性
//===============================================================

/**
 * @brief 获取枚举属性的字符串列表
 *
 * 支持两种存储模式：QStringList 和 QList<QPair<QString, int>>。
 * QList<QPair<QString, int>> 模式下提取每个 pair 的 first（名称）。
 *
 * @param[out] isSuccess 转换是否成功
 * @return 枚举名称的字符串列表
 */
QStringList DAParameterDescriptor::getEnumStringListProperty() const
{
    QStringList res;

    if (hasProperty(PropertyName_Enum)) {
        QVariant val = propertys.value(PropertyName_Enum);
        if (val.canConvert< QStringList >()) {
            res = val.value< QStringList >();
        } else if (val.canConvert< QList< QPair< QString, int > > >()) {
            QList< QPair< QString, int > > pairList = val.value< QList< QPair< QString, int > > >();
            for (const auto& pair : pairList) {
                res.append(pair.first);
            }
        }
    }

    return res;
}

/**
 * @brief 获取枚举属性的名称+值列表
 *
 * 无论原始存储为 QStringList 还是 QList<QPair<QString, int>>，
 * 均转换为 QList<QPair<QString, int>> 返回。
 * QStringList 模式下，int 值按序号从 0 开始递增。
 *
 * @param[out] isSuccess 转换是否成功
 * @return 名称与值配对的枚举列表
 */
QList< QPair< QString, int > > DAParameterDescriptor::getEnumListProperty() const
{
    QList< QPair< QString, int > > res;

    if (hasProperty(PropertyName_Enum)) {
        QVariant val = propertys.value(PropertyName_Enum);
        if (val.canConvert< QList< QPair< QString, int > > >()) {
            res = val.value< QList< QPair< QString, int > > >();
        } else if (val.canConvert< QStringList >()) {
            QStringList strList = val.value< QStringList >();
            for (int i = 0; i < strList.size(); ++i) {
                res.append({ strList[ i ], i });
            }
        }
    }

    return res;
}

/**
 * @brief 设置枚举属性（字符串列表模式）
 *
 * @param[in] enumList 枚举名称列表
 */
void DAParameterDescriptor::setEnumStringListProperty(const QStringList& enumList)
{
    propertys[ PropertyName_Enum ] = QVariant::fromValue(enumList);
}

/**
 * @brief 设置枚举属性（名称+值模式）
 *
 * @param[in] pairList 名称与值配对的枚举列表
 */
void DAParameterDescriptor::setEnumStringListProperty(const QList< QPair< QString, int > >& pairList)
{
    propertys[ PropertyName_Enum ] = QVariant::fromValue(pairList);
}

/**
 * @brief 判断是否含有枚举属性
 *
 * @return 含有枚举属性返回 true
 */
bool DAParameterDescriptor::hasEnumProperty() const
{
    return hasProperty(PropertyName_Enum);
}

//===============================================================
// min属性
//===============================================================

/**
 * @brief 获取 min 属性的整数值
 *
 * @param[out] isSuccess 获取是否成功
 * @return min 值，无效时返回 -1
 */
int DAParameterDescriptor::getMinProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Min, -1, isSuccess);
}

/**
 * @brief 获取 min 属性的浮点值
 *
 * @param[out] isSuccess 获取是否成功
 * @return min 值，无效时返回 -1.0
 */
double DAParameterDescriptor::getMinFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Min, -1.0, isSuccess);
}

/**
 * @brief 设置 min 属性（整数）
 *
 * @param[in] min 最小值
 */
void DAParameterDescriptor::setMinProperty(int min)
{
    propertys[ PropertyName_Min ] = QVariant::fromValue(min);
}

/**
 * @brief 设置 min 属性（浮点）
 *
 * @param[in] min 最小值
 */
void DAParameterDescriptor::setMinProperty(double min)
{
    propertys[ PropertyName_Min ] = QVariant::fromValue(min);
}

//===============================================================
// max属性
//===============================================================

/**
 * @brief 获取 max 属性的整数值
 *
 * @param[out] isSuccess 获取是否成功
 * @return max 值，无效时返回 -1
 */
int DAParameterDescriptor::getMaxProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Max, -1, isSuccess);
}

/**
 * @brief 获取 max 属性的浮点值
 *
 * @param[out] isSuccess 获取是否成功
 * @return max 值，无效时返回 -1.0
 */
double DAParameterDescriptor::getMaxFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Max, -1.0, isSuccess);
}

/**
 * @brief 设置 max 属性（整数）
 *
 * @param[in] max 最大值
 */
void DAParameterDescriptor::setMaxProperty(int max)
{
    propertys[ PropertyName_Max ] = QVariant::fromValue(max);
}

/**
 * @brief 设置 max 属性（浮点）
 *
 * @param[in] max 最大值
 */
void DAParameterDescriptor::setMaxProperty(double max)
{
    propertys[ PropertyName_Max ] = QVariant::fromValue(max);
}

//===============================================================
// step属性
//===============================================================

/**
 * @brief 获取 step 属性的整数值
 *
 * @param[out] isSuccess 获取是否成功
 * @return step 值，无效时返回 1
 */
int DAParameterDescriptor::getStepProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Step, 1, isSuccess);
}

/**
 * @brief 获取 step 属性的浮点值
 *
 * @param[out] isSuccess 获取是否成功
 * @return step 值，无效时返回 1.0
 */
double DAParameterDescriptor::getStepFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Step, 1.0, isSuccess);
}

/**
 * @brief 设置 step 属性（整数）
 *
 * @param[in] step 步长
 */
void DAParameterDescriptor::setStepProperty(int step)
{
    propertys[ PropertyName_Step ] = QVariant::fromValue(step);
}

/**
 * @brief 设置 step 属性（浮点）
 *
 * @param[in] step 步长
 */
void DAParameterDescriptor::setStepProperty(double step)
{
    propertys[ PropertyName_Step ] = QVariant::fromValue(step);
}

//===============================================================
// decimals属性
//===============================================================

/**
 * @brief 获取 decimals 属性
 *
 * @param[out] isSuccess 获取是否成功
 * @return decimals 值，无效时返回 2
 */
int DAParameterDescriptor::getDecimalsProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Decimals, 2, isSuccess);
}

/**
 * @brief 设置 decimals 属性
 *
 * @param[in] decimals 小数位数
 */
void DAParameterDescriptor::setDecimalsProperty(int decimals)
{
    propertys[ PropertyName_Decimals ] = QVariant::fromValue(decimals);
}

QString DAParameterDescriptor::getFilterProperty() const
{
    return Detail::getNumericProperty< QString >(propertys, PropertyName_Filter, QString(), nullptr);
}

void DAParameterDescriptor::setFilterProperty(const QString& filters)
{
    propertys[ PropertyName_Filter ] = QVariant::fromValue(filters);
}

//===============================================================
// 默认值快捷方法
//===============================================================

/**
 * @brief 判断默认值是否有效
 *
 * @return defaultValue 有效时返回 true
 */
bool DAParameterDescriptor::hasDefaultValue() const
{
    return defaultValue.isValid();
}

/**
 * @brief 将默认值转换为字符串
 *
 * @return defaultValue 的字符串表示，无效时返回空串
 */
QString DAParameterDescriptor::defaultValueToString() const
{
    return defaultValue.toString();
}

/**
 * @brief 将默认值转换为整数
 *
 * @param[out] isSuccess 转换是否成功
 * @return 转换后的整数值
 */
int DAParameterDescriptor::defaultValueToInt(bool* isSuccess) const
{
    return defaultValue.toInt(isSuccess);
}

/**
 * @brief 将默认值转换为双精度浮点数
 *
 * @param[out] isSuccess 转换是否成功
 * @return 转换后的浮点数值
 */
double DAParameterDescriptor::defaultValueToDouble(bool* isSuccess) const
{
    return defaultValue.toDouble(isSuccess);
}

/**
 * @brief 将默认值转换为布尔值
 *
 * @return 转换后的布尔值，无效 QVariant 时返回 false
 */
bool DAParameterDescriptor::defaultValueToBool() const
{
    return defaultValue.toBool();
}

}  // namespace DA
