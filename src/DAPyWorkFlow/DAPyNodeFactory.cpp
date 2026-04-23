#include "DAPyNodeFactory.h"
#include "DAPybind11InQt.h"
#include "DAPyGILGuard.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyNodeProxy.h"
#include "DAPyInterpreter.h"
#include "DANodeMetaData.h"
#include "DAAbstractNode.h"
#include "DAAbstractNodeGraphicsItem.h"
#include <QDebug>
#include <QJsonObject>

namespace DA
{

//===================================================
// 辅助函数（文件内部使用）
//===================================================

/**
 * @brief 将Python描述符字典转换为DANodeMetaData
 *
 * Python侧的DANodeDescriptor.to_dict()返回字典包含：
 * - name: 节点显示名称 → NodeName
 * - category: 节点分类 → Group
 * - qualified_name: 节点唯一标识 → NodePrototype
 * - icon: 图标标识
 *
 * @param[in] descDict Python描述符字典
 * @return 对应的DANodeMetaData对象
 */
static DANodeMetaData convertDescriptorToMetaData(const pybind11::dict& descDict)
{
    QString prototype;
    QString name;
    QString group;

    // qualified_name → NodePrototype
    if (descDict.contains("qualified_name")) {
        prototype = QString::fromStdString(pybind11::str(descDict["qualified_name"]));
    }

    // name → NodeName
    if (descDict.contains("name")) {
        name = QString::fromStdString(pybind11::str(descDict["name"]));
    }

    // category → Group
    if (descDict.contains("category")) {
        group = QString::fromStdString(pybind11::str(descDict["category"]));
    }

    DANodeMetaData metaData(prototype, name, QIcon(), group);

    // tooltip: 使用name + qualified_name作为tooltip
    QString tooltip = name;
    if (!prototype.isEmpty()) {
        tooltip += " (" + prototype + ")";
    }
    metaData.setNodeTooltip(tooltip);

    return metaData;
}

/**
 * @brief 设置节点的输入输出key
 *
 * 从Python注册表中查找节点描述符，提取inputs和outputs列表中的name字段，
 * 设置到DAAbstractNode的输入输出key中。
 *
 * @param[in] node 节点指针
 * @param[in] meta 节点元数据
 * @param[in] registry Python DANodeRegistry实例
 */
static void setupNodeIOKeys(DAAbstractNode::SharedPointer node,
                             const DANodeMetaData& meta,
                             const pybind11::object& registry)
{
    try {
        std::string qn = meta.getNodePrototype().toStdString();
        pybind11::object descObj = registry.attr("get_descriptor")(qn);

        if (descObj.is_none()) {
            qWarning() << "无法获取节点描述符:" << meta.getNodePrototype();
            return;
        }

        // 获取描述符字典
        pybind11::dict descDict;
        if (pybind11::hasattr(descObj, "to_dict")) {
            pybind11::object toDictMethod = descObj.attr("to_dict");
            pybind11::object descDictObj  = toDictMethod();
            descDict = descDictObj.cast< pybind11::dict >();
        } else {
            return;
        }

        // 设置输入key
        if (descDict.contains("inputs")) {
            pybind11::list inputs = descDict["inputs"].cast< pybind11::list >();
            for (pybind11::handle item : inputs) {
                pybind11::dict inputDict = item.cast< pybind11::dict >();
                if (inputDict.contains("name")) {
                    QString inputKey = QString::fromStdString(pybind11::str(inputDict["name"]));
                    node->addInputKey(inputKey);
                }
            }
        }

        // 设置输出key
        if (descDict.contains("outputs")) {
            pybind11::list outputs = descDict["outputs"].cast< pybind11::list >();
            for (pybind11::handle item : outputs) {
                pybind11::dict outputDict = item.cast< pybind11::dict >();
                if (outputDict.contains("name")) {
                    QString outputKey = QString::fromStdString(pybind11::str(outputDict["name"]));
                    node->addOutputKey(outputKey);
                }
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "设置节点IO key失败:" << e.what();
    }
}

//===================================================
// DAPyNode — Python节点的DAAbstractNode子类
//===================================================

/**
 * @brief Python节点的DAAbstractNode子类
 *
 * 此类是DAPyNodeFactory创建的节点类型，继承DAAbstractNode，
 * 持有DAPyNodeProxy实例，将执行逻辑委托给Python节点。
 */
class DAPyNode : public DAAbstractNode
{
    DA_DECLARE_PRIVATE(DAPyNode)
public:
    using SharedPointer = std::shared_ptr< DAPyNode >;

    DAPyNode();
    ~DAPyNode();

    // 设置Python节点代理
    void setPyNodeProxy(const DAPyNodeProxy& proxy);
    // 获取Python节点代理
    DAPyNodeProxy getPyNodeProxy() const;

    // 执行节点（委托给Python节点的execute）
    virtual bool exec() override;

    // 创建图形项（返回nullptr，由DAPyWorkFlowScene处理渲染）
    virtual DAAbstractNodeGraphicsItem* createGraphicsItem() override;
};

class DAPyNode::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNode)
public:
    PrivateData(DAPyNode* p);
    DAPyNodeProxy mPyNodeProxy;
};

DAPyNode::PrivateData::PrivateData(DAPyNode* p) : q_ptr(p)
{
}

/**
 * @brief DAPyNode构造函数
 */
DAPyNode::DAPyNode() : DAAbstractNode(), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief DAPyNode析构函数
 */
DAPyNode::~DAPyNode()
{
}

/**
 * @brief 设置Python节点代理
 * @param[in] proxy Python节点代理实例
 */
void DAPyNode::setPyNodeProxy(const DAPyNodeProxy& proxy)
{
    d_ptr->mPyNodeProxy = proxy;
}

/**
 * @brief 获取Python节点代理
 * @return 当前持有的Python节点代理实例
 */
DAPyNodeProxy DAPyNode::getPyNodeProxy() const
{
    return d_ptr->mPyNodeProxy;
}

/**
 * @brief 执行节点逻辑
 *
 * 委托给DAPyNodeProxy的execute方法执行Python节点逻辑
 *
 * @return 执行成功返回true，失败返回false
 */
bool DAPyNode::exec()
{
    return d_ptr->mPyNodeProxy.execute();
}

/**
 * @brief 创建图形项
 *
 * Python节点的渲染由DAPyWorkFlowScene处理，此函数返回nullptr
 *
 * @return nullptr
 */
DAAbstractNodeGraphicsItem* DAPyNode::createGraphicsItem()
{
    return nullptr;
}

//===================================================
// DAPyNodeFactory::PrivateData
//===================================================

class DAPyNodeFactory::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNodeFactory)
public:
    PrivateData(DAPyNodeFactory* p);

    // DANodeMetaData到创建函数的映射
    QMap< DANodeMetaData, std::function< DAAbstractNode::SharedPointer() > > mPrototypeToCreator;
    // 缓存的Python DANodeRegistry实例
    pybind11::object mPyNodeRegistry;
    // 已发现的节点元数据列表
    QList< DANodeMetaData > mNodeMetaDataList;
    // 最后的错误信息
    QString mLastErrorString;
};

DAPyNodeFactory::PrivateData::PrivateData(DAPyNodeFactory* p) : q_ptr(p)
{
}

//===================================================
// DAPyNodeFactory
//===================================================

/**
 * @brief DAPyNodeFactory构造函数
 */
DAPyNodeFactory::DAPyNodeFactory() : DAAbstractNodeFactory(), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief DAPyNodeFactory析构函数
 */
DAPyNodeFactory::~DAPyNodeFactory()
{
}

/**
 * @brief 发现Python节点
 *
 * 通过DAPyModuleWorkflow获取DANodeRegistry类，创建实例并调用discover()方法，
 * 将返回的DANodeDescriptor列表转换为C++的DANodeMetaData并注册到工厂中。
 *
 * 发现流程：
 * 1. 将scanPaths添加到Python sys.path（通过DAPyInterpreter::appendSysPath）
 * 2. 获取DAPyModuleWorkflow单例并导入DAWorkFlowPy模块
 * 3. 创建DANodeRegistry Python实例
 * 4. 调用DANodeRegistry.discover(scan_paths, use_entry_points)
 * 5. 遍历返回的DANodeDescriptor列表，转换为DANodeMetaData并注册创建函数
 *
 * @param[in] scanPaths 要扫描的目录路径列表，这些路径会被添加到Python sys.path
 * @param[in] useEntryPoints 是否使用entry_points发现节点
 * @return 发现成功返回true，失败返回false
 */
bool DAPyNodeFactory::discoverNodes(const QStringList& scanPaths, bool useEntryPoints)
{
    DA_D(d);
    DAPyGILGuard gil;

    try {
        // 1. 将扫描路径添加到Python sys.path
        for (const QString& path : scanPaths) {
            DAPyInterpreter::appendSysPath(path);
        }

        // 2. 获取DAPyModuleWorkflow单例
        DAPyModuleWorkflow& pyModule = DAPyModuleWorkflow::getInstance();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                d->mLastErrorString = "无法导入DAWorkFlowPy模块";
                qCritical() << d->mLastErrorString;
                return false;
            }
        }

        // 3. 获取DANodeRegistry类并创建实例
        pybind11::object registryClass = pyModule.getNodeRegistryClass();
        if (registryClass.is_none()) {
            d->mLastErrorString = "无法获取DANodeRegistry类引用";
            qCritical() << d->mLastErrorString;
            return false;
        }

        d->mPyNodeRegistry = registryClass();

        // 4. 构建Python参数并调用discover
        pybind11::list pyScanPaths;
        for (const QString& path : scanPaths) {
            pyScanPaths.append(path.toStdString());
        }

        pybind11::object result = d->mPyNodeRegistry.attr("discover")(pyScanPaths, useEntryPoints);

        // 5. 遍历返回的DANodeDescriptor列表
        for (pybind11::handle item : result) {
            pybind11::object descriptorObj = pybind11::reinterpret_borrow< pybind11::object >(item);

            // 获取描述符字典
            pybind11::dict descDict;
            if (pybind11::hasattr(descriptorObj, "to_dict")) {
                pybind11::object toDictMethod = descriptorObj.attr("to_dict");
                pybind11::object descDictObj  = toDictMethod();
                descDict = descDictObj.cast< pybind11::dict >();
            } else if (pybind11::isinstance< pybind11::dict >(descriptorObj)) {
                descDict = descriptorObj.cast< pybind11::dict >();
            } else {
                qWarning() << "发现无法解析的描述符对象，跳过";
                continue;
            }

            // 从字典中提取元数据
            DANodeMetaData metaData = convertDescriptorToMetaData(descDict);
            if (!metaData.isValid()) {
                qWarning() << "发现无效的节点元数据，跳过";
                continue;
            }

            // 提取qualified_name用于创建节点
            std::string qualifiedName;
            if (descDict.contains("qualified_name")) {
                qualifiedName = pybind11::str(descDict["qualified_name"]);
            }

            // 注册创建函数：通过qualified_name定位Python类，创建DAPyNode实例
            auto fp = [qualifiedName, registry = d->mPyNodeRegistry]() -> DAAbstractNode::SharedPointer {
                DAPyGILGuard gil;
                try {
                    // 通过qualified_name导入Python模块并获取节点类
                    std::string qn(qualifiedName);
                    size_t dotPos = qn.rfind('.');
                    if (dotPos == std::string::npos) {
                        qWarning() << "无效的qualified_name:" << QString::fromStdString(qn);
                        return nullptr;
                    }
                    std::string moduleName = qn.substr(0, dotPos);
                    std::string className  = qn.substr(dotPos + 1);

                    pybind11::module_ pyMod = pybind11::module_::import(moduleName.c_str());
                    pybind11::object nodeClassObj = pyMod.attr(className.c_str());

                    // 创建Python节点实例
                    pybind11::object pyNodeInstance = nodeClassObj();

                    // 创建DAPyNode并设置元数据
                    DAAbstractNode::SharedPointer node(new DAPyNode());
                    // 设置DAPyNodeProxy（通过Python实例创建代理）
                    DAPyNodeProxy proxy;
                    // TODO: DAPyNodeProxy完整实现后，将pyNodeInstance赋给proxy
                    static_cast< DAPyNode* >(node.get())->setPyNodeProxy(proxy);

                    // 设置节点元数据
                    DANodeMetaData meta(QString::fromStdString(qn), QString::fromStdString(className), QIcon(), "");
                    node->setMetaData(meta);

                    // 设置输入输出key
                    setupNodeIOKeys(node, meta, registry);

                    return node;
                } catch (const std::exception& e) {
                    qCritical() << "创建Python节点实例失败:" << e.what();
                    return nullptr;
                }
            };

            d->mPrototypeToCreator[ metaData ] = fp;
            d->mNodeMetaDataList.append(metaData);
        }

        qDebug() << "DAPyNodeFactory discovered" << d->mNodeMetaDataList.size() << "Python nodes";
        return true;

    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = QString("Python异常: %1").arg(e.what());
        qCritical() << d->mLastErrorString;
        return false;
    } catch (const std::exception& e) {
        d->mLastErrorString = QString("异常: %1").arg(e.what());
        qCritical() << d->mLastErrorString;
        return false;
    }
}

/**
 * @brief 工厂唯一标识
 *
 * 每个工厂需要保证有唯一的标识，工作流将通过标识查找工厂
 *
 * @return "DA.PyNode"
 */
QString DAPyNodeFactory::factoryPrototypes() const
{
    return "DA.PyNode";
}

/**
 * @brief 工厂名称
 *
 * 工厂名称可进行翻译
 *
 * @return "DA Python Node Factory"
 */
QString DAPyNodeFactory::factoryName() const
{
    return u8"DA Python Node Factory";
}

/**
 * @brief 工厂描述
 *
 * 工厂描述可进行翻译
 *
 * @return 工厂描述文本
 */
QString DAPyNodeFactory::factoryDescribe() const
{
    return u8"Python工作流节点工厂，通过DANodeRegistry发现和创建Python定义的节点";
}

/**
 * @brief 创建节点实例
 *
 * 根据元数据中的NodePrototype查找创建函数，
 * 创建DAPyNode实例并设置输入输出key，然后初始化节点。
 *
 * @param[in] meta 节点元数据
 * @return 节点智能指针，如果元数据未注册则返回nullptr
 * @note 此函数会在DAWorkFlow::createNode中调用，用户不要直接调用此函数
 */
DAAbstractNode::SharedPointer DAPyNodeFactory::create(const DANodeMetaData& meta)
{
    DA_D(d);
    auto fp = d->mPrototypeToCreator.value(meta, nullptr);
    if (fp) {
        DAAbstractNode::SharedPointer node = fp();
        if (node) {
            // 初始化节点（注册工厂指针）
            initializNode(node);
            return node;
        }
    }
    qWarning() << "DAPyNodeFactory::create: 未注册的节点原型" << meta.getNodePrototype();
    return nullptr;
}

/**
 * @brief 获取所有节点原型标识
 *
 * @return 原型字符串列表
 */
QStringList DAPyNodeFactory::getPrototypes() const
{
    DA_DC(d);
    QStringList res;
    res.reserve(d->mPrototypeToCreator.size());
    for (auto i = d->mPrototypeToCreator.begin(); i != d->mPrototypeToCreator.end(); ++i) {
        res.append(i.key().getNodePrototype());
    }
    return res;
}

/**
 * @brief 获取所有节点元数据
 *
 * @return DANodeMetaData列表
 */
QList< DANodeMetaData > DAPyNodeFactory::getNodesMetaData() const
{
    DA_DC(d);
    return d->mNodeMetaDataList;
}

}  // namespace DA