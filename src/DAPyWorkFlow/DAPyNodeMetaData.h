#ifndef DAPYNODEMETADATA_H
#define DAPYNODEMETADATA_H
#include "DAPyWorkFlowAPI.h"
#include <QList>
#include <QString>
#include <QStringList>
#include <QIcon>
namespace DA
{

/**
 * @brief Python节点的元数据结构体
 *
 * 替代原DANodeMetaData，用于描述Python节点的注册信息。
 * 由discoverNodes()从Python侧DANodeDescriptor.to_dict()提取填充。
 */
class DAPYWORKFLOW_API DAPyNodeMetaData
{
public:
    DAPyNodeMetaData();
    ~DAPyNodeMetaData();
    // 有效性判断
    bool isValid() const;
    // bool转换运算符（兼容原DANodeMetaData用法）
    explicit operator bool() const;
    // 比较运算符
    bool operator==(const DAPyNodeMetaData& other) const;
    bool operator!=(const DAPyNodeMetaData& other) const;
    bool operator<(const DAPyNodeMetaData& other) const;

    // 兼容原DANodeMetaData的getter方法
    QString getNodeName() const;
    QString getNodeQualifiedName() const;
    QString getGroup() const;
    QIcon getIcon() const;
    QString getNodeTooltip() const;

public:
    QString name;           // 节点显示名称
    QString qualifiedName;  // 节点唯一标识名（Python qualified_name）
    QString group;          // 节点分组/分类
    QString iconPath;  // 节点图标路径,注意这个图标仅仅用于进行节点树的图标展示，不作为场景显示的图标，场景显示的图标可以通过DANodeStyle定义
    QString tooltip;   // 节点提示文本
    QList< QString > inputKeys;   // 输入key列表
    QList< QString > outputKeys;  // 输出key列表
};

// QDebug输出
DAPYWORKFLOW_API QDebug operator<<(QDebug dbg, const DAPyNodeMetaData& meta);

// qHash（用于QHash/QSet容器）
DAPYWORKFLOW_API uint qHash(const DAPyNodeMetaData& key, uint seed = 0);
}  // namespace DA

Q_DECLARE_METATYPE(DA::DAPyNodeMetaData)

#endif  // DAPYNODEMETADATA_H
