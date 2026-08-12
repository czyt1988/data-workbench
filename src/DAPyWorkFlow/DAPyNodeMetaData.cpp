#include "DAPyNodeMetaData.h"
#include <QDebug>
namespace DA
{
/**
 * @brief 默认构造函数
 */
DAPyNodeMetaData::DAPyNodeMetaData()
{
}

/**
 * @brief 析构函数
 */
DAPyNodeMetaData::~DAPyNodeMetaData()
{
}

/**
 * @brief 判断元数据是否有效
 *
 * 有效条件：qualifiedName字段非空，qualifiedName是节点的唯一标识，
 * 是创建节点和查找注册信息的必要字段。
 *
 * @return true表示元数据有效，false表示无效
 */
bool DAPyNodeMetaData::isValid() const
{
    return !qualifiedName.isEmpty();
}

/**
 * @brief 相等比较运算符
 *
 * 以qualifiedName作为唯一标识判断两个元数据是否相等，
 * qualifiedName对应Python侧的qualified_name，保证唯一性。
 *
 * @param[in] other 要比较的另一个元数据对象
 * @return true表示qualifiedName相同，false表示不同
 */
bool DAPyNodeMetaData::operator==(const DAPyNodeMetaData& other) const
{
    return qualifiedName == other.qualifiedName;
}

/**
 * @brief 不等比较运算符
 *
 * @param[in] other 要比较的另一个元数据对象
 * @return true表示prototype不同，false表示相同
 */
bool DAPyNodeMetaData::operator!=(const DAPyNodeMetaData& other) const
{
    return !(*this == other);
}

/**
 * @brief bool转换运算符
 *
 * 兼容原DANodeMetaData用法，等效于isValid()。
 * 在条件判断中使用，如：if (md) { ... }
 *
 * @return true表示元数据有效，false表示无效
 */
DAPyNodeMetaData::operator bool() const
{
    return isValid();
}

/**
 * @brief 小于运算符
 *
 * 以qualifiedName作为比较基准，用于QMap等有序容器。
 * qualifiedName是节点的唯一标识，保证排序一致性。
 *
 * @param[in] other 要比较的另一个元数据对象
 * @return true表示当前对象的qualifiedName小于other的qualifiedName
 */
bool DAPyNodeMetaData::operator<(const DAPyNodeMetaData& other) const
{
    return qualifiedName < other.qualifiedName;
}

/**
 * @brief 获取节点显示名称
 *
 * 兼容原DANodeMetaData的getNodeName()方法，
 * 直接返回name字段。
 *
 * @return 节点显示名称字符串
 */
QString DAPyNodeMetaData::getNodeName() const
{
    return name;
}

/**
 * @brief 获取节点唯一标识名
 *
 * 兼容原DANodeMetaData的getNodePrototype()方法，
 * 直接返回qualifiedName字段。
 *
 * @return 节点唯一标识名字符串
 */
QString DAPyNodeMetaData::getNodeQualifiedName() const
{
    return qualifiedName;
}

/**
 * @brief 获取节点分组
 *
 * 兼容原DANodeMetaData的getGroup()方法，
 * 直接返回group字段。
 *
 * @return 节点分组字符串
 */
QString DAPyNodeMetaData::getCategory() const
{
    return category;
}

/**
 * @brief 获取节点图标
 *
 * 兼容原DANodeMetaData的getIcon()方法，
 * 从iconPath字段加载QIcon。若iconPath为空则返回空QIcon。
 *
 * @return 节点QIcon对象
 */
QIcon DAPyNodeMetaData::getIcon() const
{
    if (iconPath.isEmpty()) {
        return QIcon();
    }
    return QIcon(iconPath);
}

/**
 * @brief 获取节点提示文本
 *
 * 兼容原DANodeMetaData的getNodeTooltip()方法，
 * 直接返回tooltip字段。
 *
 * @return 节点提示文本字符串
 */
QString DAPyNodeMetaData::getNodeTooltip() const
{
    return tooltip;
}

/**
 * @brief QDebug输出运算符
 *
 * 格式化输出DAPyNodeMetaData的主要字段，便于调试和日志追踪。
 * 输出格式：DAPyNodeMetaData(name=xxx, qualifiedName=xxx, group=xxx, inputs=N, outputs=N)
 *
 * @param[in] dbg QDebug流对象
 * @param[in] meta 要输出的元数据对象
 * @return QDebug流对象引用
 */
QDebug operator<<(QDebug dbg, const DAPyNodeMetaData& meta)
{
    dbg.nospace() << "DAPyNodeMetaData(name=" << meta.name << ", qualifiedName=" << meta.qualifiedName
                  << ", group=" << meta.category << ")";
    return dbg.space();
}

/**
 * @brief qHash函数
 *
 * 以qualifiedName作为hash基准，用于QHash/QSet容器。
 *
 * @param[in] key 元数据对象
 * @param[in] seed hash种子
 * @return hash值
 */
uint qHash(const DAPyNodeMetaData& key, uint seed)
{
    return qHash(key.qualifiedName, seed);
}

}
