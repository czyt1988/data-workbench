#include "DAFormSchemaIO.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QStringList>
#include <QVariant>

namespace DA
{
namespace
{
// 存入 attributes 的数值/枚举扩展键（options 除外）
const QStringList kAttributeKeys = { "min", "max", "step", "decimals", "filter" };

/**
 * @brief 解析字段对象为 DAFormFieldDef
 *
 * 容错策略：识别 v2 的键并填充，未知键跳过。name 为必填项，缺失或非字符串时写入错误信息并返回 false。
 * @param obj 字段对应的 JSON 对象
 * @param field 输出的字段定义
 * @param errorMessage 错误信息输出（可为 nullptr）
 * @return 解析成功返回 true，name 缺失等硬错误返回 false
 */
bool parseField(const QJsonObject& obj, DAFormFieldDef& field, QString* errorMessage)
{
    QJsonValue nameVal = obj.value("name");
    if (!nameVal.isString()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("field missing required 'name'");
        }
        return false;
    }
    field.name = nameVal.toString();

    QJsonValue typeVal = obj.value("type");
    if (typeVal.isString()) {
        field.type = typeVal.toString();
    }
    QJsonValue labelVal = obj.value("label");
    if (labelVal.isString()) {
        field.label = labelVal.toString();
    }
    QJsonValue descVal = obj.value("description");
    if (descVal.isString()) {
        field.description = descVal.toString();
    }
    QJsonValue defaultVal = obj.value("default");
    if (!defaultVal.isUndefined() && !defaultVal.isNull()) {
        field.defaultValue = defaultVal.toVariant();
    }
    QJsonValue placeholderVal = obj.value("placeholder");
    if (placeholderVal.isString()) {
        field.placeholder = placeholderVal.toString();
    }
    QJsonValue readOnlyVal = obj.value("read_only");
    if (readOnlyVal.isBool()) {
        field.readOnly = readOnlyVal.toBool();
    }
    QJsonValue layoutVal = obj.value("layout");
    if (layoutVal.isString()) {
        field.layout = layoutVal.toString();
    }
    QJsonValue heightVal = obj.value("height");
    if (heightVal.isDouble()) {
        field.height = heightVal.toInt();
    }

    // 数值/枚举扩展属性存入 attributes（options 除外）
    for (const QString& k : kAttributeKeys) {
        QJsonValue v = obj.value(k);
        if (!v.isUndefined()) {
            field.attributes.insert(k, v.toVariant());
        }
    }

    // options 单独解析为 DAFormOption 列表
    QJsonValue optionsVal = obj.value("options");
    if (optionsVal.isArray()) {
        const QJsonArray arr = optionsVal.toArray();
        for (const QJsonValue& elem : arr) {
            if (!elem.isObject()) {
                continue;
            }
            const QJsonObject optObj = elem.toObject();
            DAFormOption opt;
            opt.value       = optObj.value("value").toVariant();
            opt.label       = optObj.value("label").toString();
            opt.description = optObj.value("description").toString();
            field.options.append(opt);
        }
    }

    QJsonValue visibleVal = obj.value("visible_when");
    if (visibleVal.isString()) {
        field.visibleWhen = visibleVal.toString();
    }
    QJsonValue enabledVal = obj.value("enabled_when");
    if (enabledVal.isString()) {
        field.enabledWhen = enabledVal.toString();
    }
    QJsonValue requiredVal = obj.value("required_when");
    if (requiredVal.isString()) {
        field.requiredWhen = requiredVal.toString();
    }
    return true;
}

// 前置声明，用于 parseGroup 与 parseItemsArray 之间的递归引用
bool parseItemsArray(const QJsonValue& val, QList< DAFormItemDef >& items, QString* errorMessage);

/**
 * @brief 解析分组对象为 DAFormGroupDef
 *
 * @param obj 分组对应的 JSON 对象
 * @param group 输出的分组定义（shared_ptr）
 * @param errorMessage 错误信息输出（可为 nullptr）
 * @return 解析成功返回 true，结构性错误返回 false
 */
bool parseGroup(const QJsonObject& obj, std::shared_ptr< DAFormGroupDef >& group, QString* errorMessage)
{
    group = std::make_shared< DAFormGroupDef >();
    group->name        = obj.value("name").toString();
    group->label       = obj.value("label").toString();
    group->description = obj.value("description").toString();

    QJsonValue itemsVal = obj.value("items");
    if (!itemsVal.isUndefined()) {
        if (!parseItemsArray(itemsVal, group->items, errorMessage)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 解析条目对象，按 kind 分发到字段或分组
 *
 * kind 缺省时按 "field" 处理。
 * @param obj 条目对应的 JSON 对象
 * @param item 输出的条目定义
 * @param errorMessage 错误信息输出（可为 nullptr）
 * @return 解析成功返回 true，硬错误返回 false
 */
bool parseItem(const QJsonObject& obj, DAFormItemDef& item, QString* errorMessage)
{
    QJsonValue kindVal = obj.value("kind");
    QString kindStr = kindVal.isString() ? kindVal.toString() : QStringLiteral("field");
    if (kindStr == "group") {
        item.kind = DAFormItemDef::Group;
        return parseGroup(obj, item.group, errorMessage);
    }
    item.kind = DAFormItemDef::Field;
    return parseField(obj, item.field, errorMessage);
}

/**
 * @brief 解析 items 数组到条目列表
 *
 * 当 items 键存在但非数组时视为硬错误。
 * @param val items 对应的 JSON 值
 * @param items 输出的条目列表
 * @param errorMessage 错误信息输出（可为 nullptr）
 * @return 解析成功返回 true，结构性错误返回 false
 */
bool parseItemsArray(const QJsonValue& val, QList< DAFormItemDef >& items, QString* errorMessage)
{
    if (!val.isArray()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("'items' must be an array");
        }
        return false;
    }
    const QJsonArray arr = val.toArray();
    for (const QJsonValue& elem : arr) {
        if (!elem.isObject()) {
            // 容错：跳过非对象元素
            continue;
        }
        DAFormItemDef item;
        if (!parseItem(elem.toObject(), item, errorMessage)) {
            return false;
        }
        items.append(item);
    }
    return true;
}

/**
 * @brief 将字段定义序列化为 JSON 对象
 *
 * 仅在 options 非空时输出 options；attributes 中的键扁平化回顶层输出；
 * 空 visible_when/enabled_when/required_when 予以省略。
 * @param field 字段定义
 * @return 字段对应的 JSON 对象
 */
QJsonObject fieldToJsonObject(const DAFormFieldDef& field)
{
    QJsonObject obj;
    obj[ "name" ] = field.name;
    if (!field.type.isEmpty()) {
        obj[ "type" ] = field.type;
    }
    if (!field.label.isEmpty()) {
        obj[ "label" ] = field.label;
    }
    if (!field.description.isEmpty()) {
        obj[ "description" ] = field.description;
    }
    if (field.defaultValue.isValid()) {
        obj[ "default" ] = QJsonValue::fromVariant(field.defaultValue);
    }
    if (!field.placeholder.isEmpty()) {
        obj[ "placeholder" ] = field.placeholder;
    }
    if (field.readOnly) {
        obj[ "read_only" ] = true;
    }
    if (!field.layout.isEmpty()) {
        obj[ "layout" ] = field.layout;
    }
    if (field.height >= 0) {
        obj[ "height" ] = field.height;
    }
    // attributes 扁平化回顶层键
    for (auto it = field.attributes.constBegin(); it != field.attributes.constEnd(); ++it) {
        obj[ it.key() ] = QJsonValue::fromVariant(it.value());
    }
    // 仅在非空时输出 options
    if (!field.options.isEmpty()) {
        QJsonArray arr;
        for (const DAFormOption& opt : field.options) {
            QJsonObject optObj;
            optObj[ "value" ] = QJsonValue::fromVariant(opt.value);
            optObj[ "label" ] = opt.label;
            if (!opt.description.isEmpty()) {
                optObj[ "description" ] = opt.description;
            }
            arr.append(optObj);
        }
        obj[ "options" ] = arr;
    }
    // 省略空的联动规则
    if (!field.visibleWhen.isEmpty()) {
        obj[ "visible_when" ] = field.visibleWhen;
    }
    if (!field.enabledWhen.isEmpty()) {
        obj[ "enabled_when" ] = field.enabledWhen;
    }
    if (!field.requiredWhen.isEmpty()) {
        obj[ "required_when" ] = field.requiredWhen;
    }
    return obj;
}

// 前置声明，用于 itemToJsonObject 与 groupToJsonObject 之间的递归引用
QJsonObject groupToJsonObject(const DAFormGroupDef& group);

/**
 * @brief 将条目定义序列化为 JSON 对象
 *
 * 按 kind 分发：Group 时输出 kind="group" 并合并分组字段；Field 时输出 kind="field" 并合并字段定义。
 * @param item 条目定义
 * @return 条目对应的 JSON 对象
 */
QJsonObject itemToJsonObject(const DAFormItemDef& item)
{
    QJsonObject obj;
    if (item.kind == DAFormItemDef::Group) {
        obj[ "kind" ] = QStringLiteral("group");
        if (item.group) {
            QJsonObject groupObj = groupToJsonObject(*item.group);
            for (auto it = groupObj.constBegin(); it != groupObj.constEnd(); ++it) {
                obj[ it.key() ] = it.value();
            }
        }
    } else {
        obj[ "kind" ] = QStringLiteral("field");
        QJsonObject fieldObj = fieldToJsonObject(item.field);
        for (auto it = fieldObj.constBegin(); it != fieldObj.constEnd(); ++it) {
            obj[ it.key() ] = it.value();
        }
    }
    return obj;
}

/**
 * @brief 将分组定义序列化为 JSON 对象
 * @param group 分组定义
 * @return 分组对应的 JSON 对象
 */
QJsonObject groupToJsonObject(const DAFormGroupDef& group)
{
    QJsonObject obj;
    obj[ "name" ] = group.name;
    if (!group.label.isEmpty()) {
        obj[ "label" ] = group.label;
    }
    if (!group.description.isEmpty()) {
        obj[ "description" ] = group.description;
    }
    if (!group.items.isEmpty()) {
        QJsonArray arr;
        for (const DAFormItemDef& item : group.items) {
            // group 为 const 引用，group.items 为 const 容器，range-for 安全
            arr.append(itemToJsonObject(item));
        }
        obj[ "items" ] = arr;
    }
    return obj;
}
}  // namespace

/**
 * @brief 从 JSON 对象解析表单规格
 *
 * 解析 v2 schema：version 缺省为 2，title 为字符串，items 为数组。
 * 对未知键保持容错，但对 items 非数组、字段缺少 name 等硬错误写入 errorMessage 并返回 false。
 * @param obj 顶层 JSON 对象
 * @param spec 输出的表单规格
 * @param errorMessage 错误信息输出（可为 nullptr）
 * @return 解析成功返回 true，硬错误返回 false
 */
bool DAFormSchemaIO::fromJsonObject(const QJsonObject& obj, DAFormSpec& spec, QString* errorMessage)
{
    spec = DAFormSpec();  // 重置为默认状态

    QJsonValue versionVal = obj.value("version");
    if (versionVal.isDouble()) {
        spec.version = versionVal.toInt();
    } else {
        spec.version = 2;
    }

    QJsonValue titleVal = obj.value("title");
    if (titleVal.isString()) {
        spec.title = titleVal.toString();
    }

    QJsonValue itemsVal = obj.value("items");
    if (!itemsVal.isUndefined()) {
        if (!parseItemsArray(itemsVal, spec.items, errorMessage)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 将表单规格序列化为 JSON 对象
 *
 * 固定输出 version: 2，省略空的可选字符串字段，仅输出 attributes 中存在的键，
 * 仅在 options 非空时输出 options，省略空的 visible_when/enabled_when/required_when。
 * @param spec 表单规格
 * @return 表单规格对应的 JSON 对象
 */
QJsonObject DAFormSchemaIO::toJsonObject(const DAFormSpec& spec)
{
    QJsonObject obj;
    obj[ "version" ] = 2;  // 固定输出 v2
    if (!spec.title.isEmpty()) {
        obj[ "title" ] = spec.title;
    }
    if (!spec.items.isEmpty()) {
        QJsonArray arr;
        for (const DAFormItemDef& item : spec.items) {
            // spec 为 const 引用，spec.items 为 const 容器，range-for 安全
            arr.append(itemToJsonObject(item));
        }
        obj[ "items" ] = arr;
    }
    return obj;
}

}  // namespace DA
