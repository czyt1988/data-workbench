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

}  // namespace DA
