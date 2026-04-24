#ifndef DAPYNODEFACTORY_H
#define DAPYNODEFACTORY_H
#include "DAPyWorkFlowAPI.h"
#include "DAGlobals.h"
#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QIcon>

namespace DA
{
class DAPyNodeProxy;

/**
 * @brief Python节点的元数据结构体，独立于DAWorkFlow模块
 *
 * 替代原DANodeMetaData，用于描述Python节点的注册信息。
 * 由discoverNodes()从Python侧DANodeDescriptor.to_dict()提取填充。
 */
struct DAPYWORKFLOW_API DAPyNodeMetaData
{
    QString name;                // 节点显示名称
    QString prototype;           // 节点唯一原型标识（qualified_name）
    QString group;               // 节点分组/分类
    QString iconPath;            // 节点图标路径
    QString tooltip;             // 节点提示文本
    QList< QString > inputKeys;  // 输入key列表
    QList< QString > outputKeys; // 输出key列表

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
    QString getNodePrototype() const;
    QString getGroup() const;
    QIcon getIcon() const;
    QString getNodeTooltip() const;
};

// QDebug输出
DAPYWORKFLOW_API QDebug operator<<(QDebug dbg, const DAPyNodeMetaData& meta);

// qHash（用于QHash/QSet容器）
DAPYWORKFLOW_API uint qHash(const DAPyNodeMetaData& key, uint seed = 0);

/**
 * @brief Python节点工厂，独立QObject，不继承DAAbstractNodeFactory
 *
 * 通过DAPyModuleWorkflow调用Python侧的DANodeRegistry发现节点，
 * 将Python的DANodeDescriptor转换为C++的DAPyNodeMetaData，
 * 并通过DAPyNodeProxy创建节点实例。
 *
 * @code
 * DA::DAPyNodeFactory factory;
 * factory.discoverNodes(); // 发现Python节点
 * auto metaList = factory.getNodeMetadataList();
 * auto proxy = factory.createNodeProxy("pkg.module.MyNode");
 * @endcode
 * @see DAPyModuleWorkflow DAPyNodeProxy DAPyNodeMetaData
 */
class DAPYWORKFLOW_API DAPyNodeFactory : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyNodeFactory)
public:
    // 构造工厂（可选parent，非单例）
    explicit DAPyNodeFactory(QObject* parent = nullptr);
    ~DAPyNodeFactory();

    // 发现Python节点（调用DANodeRegistry.discover）
    bool discoverNodes(const QStringList& scanPaths = QStringList(), bool useEntryPoints = false);

    // 通过限定名创建DAPyNodeProxy实例
    DAPyNodeProxy* createNodeProxy(const QString& qualifiedName);

    // 获取所有已发现节点的元数据列表
    QList< DAPyNodeMetaData > getNodeMetadataList() const;

    // 获取所有已发现节点的原型标识列表
    QStringList getNodePrototypes() const;

    // 工厂名称
    QString factoryName() const;

    // 工厂描述
    QString factoryDescribe() const;

    // 获取最后的错误信息
    QString getLastErrorString() const;

Q_SIGNALS:
    // 节点发现完成信号，通知UI更新
    void nodeDiscovered(const QList< DA::DAPyNodeMetaData >& metadataList);
};

}  // namespace DA

Q_DECLARE_METATYPE(DA::DAPyNodeMetaData)

#endif  // DAPYNODEFACTORY_H