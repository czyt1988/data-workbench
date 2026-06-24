#include "DAParamDef.h"

namespace DA
{

const QString DAParamDef::PropertyName_Enum     = QStringLiteral("enum");
const QString DAParamDef::PropertyName_Min      = QStringLiteral("min");
const QString DAParamDef::PropertyName_Max      = QStringLiteral("max");
const QString DAParamDef::PropertyName_Step     = QStringLiteral("step");
const QString DAParamDef::PropertyName_Decimals = QStringLiteral("decimals");
const QString DAParamDef::PropertyName_Filter   = QStringLiteral("filter");
const QString DAParamDef::PropertyName_Layout   = QStringLiteral("layout");
const QString DAParamDef::PropertyName_Height   = QStringLiteral("height");

const QString DAParamDef::TypeName_Int    = QStringLiteral("int");
const QString DAParamDef::TypeName_Float  = QStringLiteral("float");
const QString DAParamDef::TypeName_Bool   = QStringLiteral("bool");
const QString DAParamDef::TypeName_Str    = QStringLiteral("str");
const QString DAParamDef::TypeName_Enum   = QStringLiteral("enum");
const QString DAParamDef::TypeName_List   = QStringLiteral("list");
const QString DAParamDef::TypeName_File   = QStringLiteral("file");
const QString DAParamDef::TypeName_Folder = QStringLiteral("folder");
const QString DAParamDef::TypeName_Color  = QStringLiteral("color");
const QString DAParamDef::TypeName_Font   = QStringLiteral("font");
const QString DAParamDef::TypeName_Code   = QStringLiteral("code");
const QString DAParamDef::TypeName_Dict   = QStringLiteral("dict");

DAParamDef::DAParamDef() : propertyId(0)
{
}

bool DAParamDef::hasProperty(const QString& propName) const
{
    return propertys.contains(propName);
}

/**
 * @brief 将 Python 端类型别名归一化为内部标准类型字符串
 *
 * 处理以下别名（大小写不敏感）：
 * - "string"  → "str"
 * - "boolean" → "bool"
 * - "double"  → "float"
 * - "integer" → "int"
 *
 * 其他类型字符串原样返回（仅做 trim）。
 * 空字符串返回空字符串。
 *
 * @param[in] rawType 原始类型字符串
 * @return 归一化后的类型字符串
 */
QString DAParamDef::normalizeTypeName(const QString& rawType)
{
    QString t = rawType.trimmed().toLower();
    if (t.isEmpty()) {
        return QString();
    }
    if (t == QStringLiteral("string")) {
        return TypeName_Str;
    }
    if (t == QStringLiteral("boolean")) {
        return TypeName_Bool;
    }
    if (t == QStringLiteral("double")) {
        return TypeName_Float;
    }
    if (t == QStringLiteral("integer")) {
        return TypeName_Int;
    }
    return t;
}

/**
 * @brief 字符串类型 → ParamType 枚举
 *
 * 先调用 normalizeTypeName() 归一化，再映射到 ParamType。
 * 未识别的字符串返回 TypeUnknown。
 *
 * @param[in] typeStr 类型字符串
 * @return 对应的 ParamType 枚举
 */
DAParamDef::ParamType DAParamDef::stringToParamType(const QString& typeStr)
{
    const QString t = normalizeTypeName(typeStr);
    if (t == TypeName_Int) {
        return TypeInt;
    }
    if (t == TypeName_Float) {
        return TypeFloat;
    }
    if (t == TypeName_Bool) {
        return TypeBool;
    }
    if (t == TypeName_Str) {
        return TypeStr;
    }
    if (t == TypeName_Enum) {
        return TypeEnum;
    }
    if (t == TypeName_List) {
        return TypeList;
    }
    if (t == TypeName_File) {
        return TypeFile;
    }
    if (t == TypeName_Folder) {
        return TypeFolder;
    }
    if (t == TypeName_Color) {
        return TypeColor;
    }
    if (t == TypeName_Font) {
        return TypeFont;
    }
    if (t == TypeName_Code) {
        return TypeCode;
    }
    if (t == TypeName_Dict) {
        return TypeDict;
    }
    return TypeUnknown;
}

/**
 * @brief ParamType 枚举 → 字符串
 *
 * @param[in] t 参数类型枚举
 * @return 对应的类型字符串，TypeUnknown 返回空串
 */
QString DAParamDef::paramTypeToString(DAParamDef::ParamType t)
{
    switch (t) {
    case TypeInt:
        return TypeName_Int;
    case TypeFloat:
        return TypeName_Float;
    case TypeBool:
        return TypeName_Bool;
    case TypeStr:
        return TypeName_Str;
    case TypeEnum:
        return TypeName_Enum;
    case TypeList:
        return TypeName_List;
    case TypeFile:
        return TypeName_File;
    case TypeFolder:
        return TypeName_Folder;
    case TypeColor:
        return TypeName_Color;
    case TypeFont:
        return TypeName_Font;
    case TypeCode:
        return TypeName_Code;
    case TypeDict:
        return TypeName_Dict;
    case TypeUnknown:
    default:
        return QString();
    }
}

/**
 * @brief 返回当前参数的 ParamType 枚举
 *
 * 内部调用 stringToParamType(type)，对 type 字段做归一化后映射。
 * 业务代码应优先使用此方法而非直接字符串比较。
 *
 * @return 归一化后的 ParamType，未识别返回 TypeUnknown
 */
DAParamDef::ParamType DAParamDef::typeEnum() const
{
    return stringToParamType(type);
}

bool DAParamDef::isInt() const
{
    return typeEnum() == TypeInt;
}

bool DAParamDef::isFloat() const
{
    return typeEnum() == TypeFloat;
}

bool DAParamDef::isBool() const
{
    return typeEnum() == TypeBool;
}

bool DAParamDef::isStr() const
{
    return typeEnum() == TypeStr;
}

bool DAParamDef::isEnum() const
{
    return typeEnum() == TypeEnum;
}

bool DAParamDef::isList() const
{
    return typeEnum() == TypeList;
}

bool DAParamDef::isFile() const
{
    return typeEnum() == TypeFile;
}

bool DAParamDef::isFolder() const
{
    return typeEnum() == TypeFolder;
}

bool DAParamDef::isColor() const
{
    return typeEnum() == TypeColor;
}

bool DAParamDef::isFont() const
{
    return typeEnum() == TypeFont;
}

bool DAParamDef::isCode() const
{
    return typeEnum() == TypeCode;
}

bool DAParamDef::isDict() const
{
    return typeEnum() == TypeDict;
}

bool DAParamDef::isNumeric() const
{
    const ParamType t = typeEnum();
    return t == TypeInt || t == TypeFloat;
}

bool DAParamDef::isPath() const
{
    const ParamType t = typeEnum();
    return t == TypeFile || t == TypeFolder;
}

QStringList DAParamDef::getEnumStringListProperty() const
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

QList< QPair< QString, int > > DAParamDef::getEnumListProperty() const
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

bool DAParamDef::hasEnumProperty() const
{
    return hasProperty(PropertyName_Enum);
}

int DAParamDef::getMinProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Min, -1, isSuccess);
}

double DAParamDef::getMinFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Min, -1.0, isSuccess);
}

int DAParamDef::getMaxProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Max, -1, isSuccess);
}

double DAParamDef::getMaxFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Max, -1.0, isSuccess);
}

int DAParamDef::getStepProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Step, 1, isSuccess);
}

double DAParamDef::getStepFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Step, 1.0, isSuccess);
}

int DAParamDef::getDecimalsProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Decimals, 2, isSuccess);
}

QString DAParamDef::getFilterProperty() const
{
    return Detail::getNumericProperty< QString >(propertys, PropertyName_Filter, QString(), nullptr);
}

bool DAParamDef::hasLayoutProperty() const
{
    return hasProperty(PropertyName_Layout);
}

QString DAParamDef::getLayoutProperty() const
{
    if (!hasLayoutProperty()) {
        return QStringLiteral("inline");
    }
    QString val = propertys.value(PropertyName_Layout).toString().toLower();
    return val.isEmpty() ? QStringLiteral("inline") : val;
}

bool DAParamDef::isLayoutBelow() const
{
    return getLayoutProperty() == QStringLiteral("below");
}

bool DAParamDef::hasHeightProperty() const
{
    return hasProperty(PropertyName_Height);
}

int DAParamDef::getHeightProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Height, -1, isSuccess);
}

bool DAParamDef::hasDefaultValue() const
{
    return defaultValue.isValid();
}

QString DAParamDef::defaultValueToString() const
{
    return defaultValue.toString();
}

int DAParamDef::defaultValueToInt(bool* isSuccess) const
{
    return defaultValue.toInt(isSuccess);
}

double DAParamDef::defaultValueToDouble(bool* isSuccess) const
{
    return defaultValue.toDouble(isSuccess);
}

bool DAParamDef::defaultValueToBool() const
{
    return defaultValue.toBool();
}

/**
 * @brief 将DAPyNodeParameter转换为DAParamDef
 *
 * 从参数代理读取元数据，构建等价的DAParamDef结构体，
 * 供编辑器创建函数复用DAParamDef的helper方法。
 * type 字段经过 normalizeTypeName() 归一化，确保后续 typeEnum()/isXxx() 稳定识别。
 *
 * @param[in] param 参数代理
 * @return 等价的DAParamDef
 */
DAParamDef toParamDef(const DAPyNodeParameter& param)
{
    DAParamDef def;
    def.name         = param.name();
    def.type         = DAParamDef::normalizeTypeName(param.typeLabel());
    def.description  = param.description();
    def.defaultValue = param.defaultValue();
    def.propertys    = param.properties();
    return def;
}

}  // namespace DA
