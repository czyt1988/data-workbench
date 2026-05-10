#include "DANodeDescriptor.h"

namespace DA
{

// ============================================================
// DANodeDescriptor — 默认构造函数
// ============================================================

/**
 * @brief 默认构造函数，初始化节点描述符
 *
 * 初始化字段为以下默认值：
 * - name: 空字符串
 * - qualifiedName: 空字符串
 * - category: 空字符串
 * - icon: 空字符串
 * - inputs: 空 QVector
 * - outputs: 空 QVector
 * - parameters: 空 QVector
 * - renderTemplate: RenderTemplate::NodeStyleTemplate
 * - style: 默认 DANodeStyle（调用 setDefaults）
 */
DANodeDescriptor::DANodeDescriptor() : renderTemplate(RenderTemplate::NodeStyleTemplate), style(DANodeStyle())
{
}

// ============================================================
// DANodeDescriptor::isValid — 有效性判断
// ============================================================

/**
 * @brief 判断节点描述符是否有效
 *
 * 仅当 qualifiedName 非空时返回 true，
 * 空字符串表示节点尚未绑定到具体 Python 类，视为无效。
 *
 * @return true 表示 qualifiedName 非空，false 表示无效
 */
bool DANodeDescriptor::isValid() const
{
    return !qualifiedName.isEmpty();
}

// ============================================================
// DANodeDescriptor::toMetaData — 转换为 DAPyNodeMetaData
// ============================================================

/**
 * @brief 从节点描述符提取 DAPyNodeMetaData
 *
 * 将描述符字段映射到工厂注册所需的元数据：
 * - name → metaData.name
 * - qualifiedName → metaData.qualifiedName
 * - category → metaData.group
 * - icon → metaData.iconPath
 * - tooltip = name + " (" + qualifiedName + ")"（qualifiedName 非空时）
 * - inputs 各项的 .name → metaData.inputKeys
 * - outputs 各项的 .name → metaData.outputKeys
 *
 * @return 包含注册信息的 DAPyNodeMetaData 实例
 * @see DAPyNodeFactory::convertDescriptorToMetaData（Python dict 版本）
 */
DAPyNodeMetaData DANodeDescriptor::toMetaData() const
{
    DAPyNodeMetaData metaData;

    metaData.name          = name;
    metaData.qualifiedName = qualifiedName;
    metaData.group         = category;
    metaData.iconPath      = icon;

    // tooltip: name + " (" + qualifiedName + ")"
    metaData.tooltip = name;
    if (!qualifiedName.isEmpty()) {
        metaData.tooltip += " (" + qualifiedName + ")";
    }

    // inputs → inputKeys（提取每项的 .name）
    for (const DAPortDescriptor& port : inputs) {
        metaData.inputKeys.append(port.name);
    }

    // outputs → outputKeys（提取每项的 .name）
    for (const DAPortDescriptor& port : outputs) {
        metaData.outputKeys.append(port.name);
    }

    return metaData;
}

// ============================================================
// DANodeDescriptor::toJson — 序列化为 JSON
// ============================================================

/**
 * @brief 将节点描述符序列化为 QJsonObject
 *
 * JSON 键名使用 snake_case 以匹配旧 Python dict 格式：
 * - "name": 节点显示名称
 * - "qualified_name": 节点唯一标识名
 * - "category": 节点分类
 * - "icon": 图标路径（空时省略）
 * - "inputs": 输入端口数组（空数组时省略）
 * - "outputs": 输出端口数组（空数组时省略）
 * - "parameters": 参数数组（空数组时省略）
 * - "render_template": 渲染模板字符串（默认值 NodeStyleTemplate 时省略）
 * - "style": 样式 JSON（DANodeStyleToJson 稀疏输出，空时省略）
 *
 * 采用稀疏策略：仅写入非空/非默认的字段。
 *
 * @return QJsonObject 序列化结果
 */
QJsonObject DANodeDescriptor::toJson() const
{
    QJsonObject obj;

    obj[ QStringLiteral("name") ]           = name;
    obj[ QStringLiteral("qualified_name") ] = qualifiedName;

    if (!category.isEmpty()) {
        obj[ QStringLiteral("category") ] = category;
    }

    if (!icon.isEmpty()) {
        obj[ QStringLiteral("icon") ] = icon;
    }

    // inputs 数组（非空时才写入）
    if (!inputs.isEmpty()) {
        QJsonArray inputsArr;
        for (const DAPortDescriptor& port : inputs) {
            inputsArr.append(port.toJson());
        }
        obj[ QStringLiteral("inputs") ] = inputsArr;
    }

    // outputs 数组（非空时才写入）
    if (!outputs.isEmpty()) {
        QJsonArray outputsArr;
        for (const DAPortDescriptor& port : outputs) {
            outputsArr.append(port.toJson());
        }
        obj[ QStringLiteral("outputs") ] = outputsArr;
    }

    // parameters 数组（非空时才写入）
    if (!parameters.isEmpty()) {
        QJsonArray paramsArr;
        for (const DAParameterDescriptor& param : parameters) {
            QJsonObject paramObj;
            paramObj[ QStringLiteral("name") ] = param.name;
            paramObj[ QStringLiteral("type") ] = param.type;
            if (!param.description.isEmpty()) {
                paramObj[ QStringLiteral("description") ] = param.description;
            }
            if (!param.defaultValue.isNull()) {
                paramObj[ QStringLiteral("default") ] = QJsonValue::fromVariant(param.defaultValue);
            }
            paramsArr.append(paramObj);
        }
        obj[ QStringLiteral("parameters") ] = paramsArr;
    }

    // render_template（非默认值时才写入）
    if (renderTemplate != RenderTemplate::NodeStyleTemplate) {
        obj[ QStringLiteral("render_template") ] = DA::enumToString(renderTemplate);
    }

    // style（DANodeStyleToJson 稀疏输出，非空时才写入）
    QJsonObject styleJson = DANodeStyleToJson(style);
    if (!styleJson.isEmpty()) {
        obj[ QStringLiteral("style") ] = styleJson;
    }

    return obj;
}

// ============================================================
// DANodeDescriptor::fromJson — 从 JSON 反序列化
// ============================================================

/**
 * @brief 从 QJsonObject 反序列化为 DANodeDescriptor
 *
 * 解析 snake_case 键名的 JSON，缺失可选字段使用默认值回退：
 * - "name" → name（缺失时为空字符串）
 * - "qualified_name" → qualifiedName（缺失时为空字符串）
 * - "category" → category（缺失时为空字符串）
 * - "icon" → icon（缺失时为空字符串）
 * - "inputs" → inputs（缺失时为空 QVector，各项由 DAPortDescriptor::fromJson 解析）
 * - "outputs" → outputs（同上）
 * - "parameters" → parameters（缺失时为空 QVector，由 ParameterDescriptor::fromJson 解析）
 * - "render_template" → renderTemplate（缺失时默认 NodeStyleTemplate）
 * - "style" → style（缺失时默认 DANodeStyle，由 DANodeStyleFromJson 解析）
 *
 * @param[in] obj 包含节点描述信息的 JSON 对象
 * @return DANodeDescriptor 实例
 */
DANodeDescriptor DANodeDescriptor::fromJson(const QJsonObject& obj)
{
    DANodeDescriptor desc;

    desc.name          = obj.value(QStringLiteral("name")).toString();
    desc.qualifiedName = obj.value(QStringLiteral("qualified_name")).toString();
    desc.category      = obj.value(QStringLiteral("category")).toString();
    desc.icon          = obj.value(QStringLiteral("icon")).toString();

    // inputs 数组
    if (obj.contains(QStringLiteral("inputs"))) {
        QJsonArray inputsArr = obj.value(QStringLiteral("inputs")).toArray();
        for (const QJsonValue& val : inputsArr) {
            desc.inputs.append(DAPortDescriptor::fromJson(val.toObject()));
        }
    }

    // outputs 数组
    if (obj.contains(QStringLiteral("outputs"))) {
        QJsonArray outputsArr = obj.value(QStringLiteral("outputs")).toArray();
        for (const QJsonValue& val : outputsArr) {
            desc.outputs.append(DAPortDescriptor::fromJson(val.toObject()));
        }
    }

    // parameters 数组
    if (obj.contains(QStringLiteral("parameters"))) {
        QJsonArray paramsArr = obj.value(QStringLiteral("parameters")).toArray();
        desc.parameters      = DAParameterDescriptor::fromJsonArray(paramsArr);
    }

    // render_template（缺失时默认 NodeStyleTemplate）
    if (obj.contains(QStringLiteral("render_template"))) {
        desc.renderTemplate =
            DA::stringToEnum< DA::RenderTemplate >(obj.value(QStringLiteral("render_template")).toString());
    }

    // style（缺失时默认 DANodeStyle）
    if (obj.contains(QStringLiteral("style"))) {
        desc.style = DANodeStyleFromJson(obj.value(QStringLiteral("style")).toObject());
    }

    return desc;
}

}  // namespace DA
