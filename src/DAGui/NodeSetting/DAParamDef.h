#ifndef DAPARAMDEF_H
#define DAPARAMDEF_H
#include "DAGuiAPI.h"
#include <QString>
#include <QVariant>
#include <QVariantHash>
#include <QList>
#include <QPair>
#include <QStringList>
#include "DAPyNodeParameter.h"

namespace DA
{

/**
 * @brief Python节点参数定义
 *
 * 轻量级数据结构，用于描述Python节点的参数信息。
 * 替代原DAParameterDescriptor，保留DAGui参数编辑器所需的属性系统。
 * 包含参数名称、类型、描述、默认值及扩展属性（enum/min/max/step/decimals/filter）。
 *
 * 类型识别统一通过 typeEnum() / isXxx() 完成，业务代码避免直接与 type 字符串字面量比较。
 * 类型字符串归一化由 normalizeTypeName() 完成，toParamDef() 在构造时即归一化。
 *
 * @code
 * DAParamDef def = toParamDef(pyParam);
 * switch (def.typeEnum()) {
 * case DAParamDef::TypeInt:    // 处理 int
 * case DAParamDef::TypeFloat:  // 处理 float
 * default: break;
 * }
 * @endcode
 */
struct DAGUI_API DAParamDef
{
    /**
     * @brief 参数类型枚举
     *
     * 与字符串类型一一对应，用于 switch 穷尽性分发，避免业务代码散落字符串比较。
     */
    enum ParamType
    {
        TypeUnknown = 0,  ///< 未知/未识别类型
        TypeInt,          ///< 整数 (int)
        TypeFloat,        ///< 浮点 (float)
        TypeBool,         ///< 布尔 (bool)
        TypeStr,          ///< 字符串 (str)
        TypeEnum,         ///< 枚举
        TypeList,         ///< 列表
        TypeFile,         ///< 文件路径
        TypeFolder,       ///< 文件夹路径
        TypeColor,        ///< 颜色
        TypeFont,         ///< 字体
        TypeCode,         ///< 代码
        TypeDict          ///< 字典
    };

    QString name;           ///< 参数名称
    QString type;           ///< 参数类型字符串（已归一化，可直接与 TypeName_* 常量比较）
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
     * - layout (PropertyName_Layout): QString，"inline"（默认）或 "below"
     * - height (PropertyName_Height): int，编辑器高度（像素），仅 below 模式生效
     */
    QVariantHash propertys;

    // 属性名常量
    static const QString PropertyName_Enum;
    static const QString PropertyName_Min;
    static const QString PropertyName_Max;
    static const QString PropertyName_Step;
    static const QString PropertyName_Decimals;
    static const QString PropertyName_Filter;
    static const QString PropertyName_Layout;
    static const QString PropertyName_Height;

    // 类型名字符串常量（用于注册表 key、序列化等场景，避免散落字面量）
    static const QString TypeName_Int;
    static const QString TypeName_Float;
    static const QString TypeName_Bool;
    static const QString TypeName_Str;
    static const QString TypeName_Enum;
    static const QString TypeName_List;
    static const QString TypeName_File;
    static const QString TypeName_Folder;
    static const QString TypeName_Color;
    static const QString TypeName_Font;
    static const QString TypeName_Code;
    static const QString TypeName_Dict;

    // 默认构造
    DAParamDef();

    // 判断是否有属性
    bool hasProperty(const QString& propName) const;

    // ---- 类型识别 ----
    // 返回归一化后的 ParamType 枚举，TypeUnknown 表示未识别
    ParamType typeEnum() const;
    // 类型判断快捷方法
    bool isInt() const;
    bool isFloat() const;
    bool isBool() const;
    bool isStr() const;
    bool isEnum() const;
    bool isList() const;
    bool isFile() const;
    bool isFolder() const;
    bool isColor() const;
    bool isFont() const;
    bool isCode() const;
    bool isDict() const;
    // 复合判断
    bool isNumeric() const;  // int 或 float
    bool isPath() const;     // file 或 folder

    // ---- 类型字符串归一化与转换（静态工具） ----
    // 将 Python 端别名归一化为内部标准类型字符串：string→str、boolean→bool、double→float、integer→int
    static QString normalizeTypeName(const QString& rawType);
    // 字符串 → ParamType 枚举（先归一化再映射，未知返回 TypeUnknown）
    static ParamType stringToParamType(const QString& typeStr);
    // ParamType 枚举 → 字符串（TypeUnknown 返回空串）
    static QString paramTypeToString(ParamType t);

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

    // ---- Layout属性 ----
    // 返回 "inline"（默认）或 "below"，不区分大小写归一化为小写
    QString getLayoutProperty() const;
    bool isLayoutBelow() const;
    bool hasLayoutProperty() const;

    // ---- Height属性 ----
    // 返回编辑器高度（像素），未设置时 isSuccess=false 并返回 -1
    int getHeightProperty(bool* isSuccess = nullptr) const;
    bool hasHeightProperty() const;

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

DAParamDef toParamDef(const DAPyNodeParameter& param);
}  // namespace DA

#endif  // DAPARAMDEF_H
