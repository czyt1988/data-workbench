#include "DAPyNodeFactory.h"
#include "DAPybind11InQt.h"
#include "DAPyGILGuard.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyNodeProxy.h"
#include "DAPyInterpreter.h"
#include "DANodeDescriptor.h"
#include "DAPyJsonCast.h"
#include <QDebug>
#include <QJsonObject>
#include <QHash>

namespace DA
{

//===================================================
// DAPyNodeMetaData
//===================================================

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
QString DAPyNodeMetaData::getGroup() const
{
    return group;
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
                  << ", group=" << meta.group << ", inputs=" << meta.inputKeys.size()
                  << ", outputs=" << meta.outputKeys.size() << ")";
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

//===================================================
// DAPyNodeFactory::PrivateData
//===================================================

class DAPyNodeFactory::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNodeFactory)
public:
    PrivateData(DAPyNodeFactory* p);

    // 统一异常处理
    void dealException(const std::exception& e) const;

    // 已发现的节点元数据列表
    QList< DAPyNodeMetaData > mNodeMetaDataList;
    // 最后的错误信息
    mutable QString mLastErrorString;
};

DAPyNodeFactory::PrivateData::PrivateData(DAPyNodeFactory* p) : q_ptr(p)
{
}

/**
 * @brief 统一异常处理
 *
 * 参考DAPyModulePandas的dealException模式，将异常信息存储到mLastErrorString中。
 * 对于pybind11::error_already_set异常，在GIL作用域内消费。
 *
 * @param[in] e 捕获的异常对象
 */
void DAPyNodeFactory::PrivateData::dealException(const std::exception& e) const
{
    mLastErrorString = e.what();
    qCritical() << "DAPyNodeFactory error:" << mLastErrorString;
}

//===================================================
// DAPyNodeFactory
//===================================================

/**
 * @brief DAPyNodeFactory构造函数
 *
 * 构造独立的Python节点工厂，非单例模式。
 * 可通过parent参数挂载到QObject对象树。
 *
 * @param[in] parent 父QObject对象，默认nullptr
 */
DAPyNodeFactory::DAPyNodeFactory(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
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
 * 将返回的DANodeDescriptor列表转换为C++的DAPyNodeMetaData并缓存到工厂中。
 *
 * 发现流程：
 * 1. 将scanPaths添加到Python sys.path（通过DAPyInterpreter::appendSysPath）
 * 2. 获取DAPyModuleWorkflow单例并导入DAWorkbench.DAWorkFlowPy模块
 * 3. 创建DANodeRegistry Python实例
 * 4. 调用DANodeRegistry.discover(scan_paths, use_entry_points)
 * 5. 遍历返回的DANodeDescriptor列表，转换为DAPyNodeMetaData并缓存
 * 6. 发射nodeDiscovered信号通知UI更新
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
                d->mLastErrorString = "无法导入DAWorkbench.DAWorkFlowPy模块";
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

        pybind11::object registryInstance = registryClass();

        // 4. 构建Python参数并调用discover
        pybind11::list pyScanPaths;
        for (const QString& path : scanPaths) {
            pyScanPaths.append(path.toStdString());
        }

        pybind11::object result = registryInstance.attr("discover")(pyScanPaths, useEntryPoints);

        // 5. 遍历返回的DANodeDescriptor列表
        QList< DAPyNodeMetaData > discoveredList;
        for (pybind11::handle item : result) {
            pybind11::object descObj = pybind11::reinterpret_borrow< pybind11::object >(item);

            DANodeDescriptor descriptor;
            // 新式C++ struct描述符（通过pybind11直接cast）
            descriptor                = descObj.cast< DA::DANodeDescriptor >();
            DAPyNodeMetaData metaData = descriptor.toMetaData();
            if (!metaData.isValid()) {
                qWarning() << "发现无效的节点元数据，跳过";
                continue;
            }

            discoveredList.append(metaData);
        }

        // 更新缓存列表
        d->mNodeMetaDataList = discoveredList;

        qDebug() << "DAPyNodeFactory discovered" << d->mNodeMetaDataList.size() << "Python nodes";

        // 6. 发射信号通知UI
        Q_EMIT nodeDiscovered(d->mNodeMetaDataList);
        return true;

    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = QString("Python异常: %1").arg(e.what());
        d->dealException(e);
        return false;
    } catch (const std::exception& e) {
        d->mLastErrorString = QString("异常: %1").arg(e.what());
        d->dealException(e);
        return false;
    }
}

/**
 * @brief 通过限定名创建DAPyNodeProxy实例
 *
 * 核心创建流程：
 * 1. 获取GIL保护（DAPyGILGuard RAII）
 * 2. 通过qualified_name导入Python模块并获取节点类
 * 4. 创建Python节点实例
 * 5. 创建DAPyNodeProxy并设置Python节点引用
 *
 * @param[in] qualifiedName Python节点的限定名（如"pkg.module.ClassName"）
 * @return 成功返回DAPyNodeProxy指针，失败返回nullptr
 * @note 返回的DAPyNodeProxy由调用方负责生命周期管理
 */
DAPyNodeProxy* DAPyNodeFactory::createNodeProxy(const QString& qualifiedName)
{
    DA_D(d);
    DAPyGILGuard gil;

    try {
        // 通过qualified_name导入Python模块并获取节点类
        std::string qn = qualifiedName.toStdString();
        size_t dotPos  = qn.rfind('.');
        if (dotPos == std::string::npos) {
            d->mLastErrorString = QString("无效的qualified_name: %1").arg(qualifiedName);
            qWarning() << d->mLastErrorString;
            return nullptr;
        }
        std::string moduleName = qn.substr(0, dotPos);
        std::string className  = qn.substr(dotPos + 1);

        pybind11::module_ pyMod       = pybind11::module_::import(moduleName.c_str());
        pybind11::object nodeClassObj = pyMod.attr(className.c_str());

        // 创建Python节点实例
        pybind11::object pyNodeInstance = nodeClassObj();

        // 创建DAPyNodeProxy并设置Python节点引用
        DAPyNodeProxy* proxy = new DAPyNodeProxy();
        proxy->setQualifiedName(qualifiedName);
        proxy->setPyNodeRef(pyNodeInstance);

        return proxy;

    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = QString("创建Python节点实例失败: %1").arg(e.what());
        d->dealException(e);
        return nullptr;
    } catch (const std::exception& e) {
        d->mLastErrorString = QString("创建Python节点实例失败: %1").arg(e.what());
        d->dealException(e);
        return nullptr;
    }
}

/**
 * @brief 通过节点元数据创建DAPyNodeProxy实例
 *
 * 此方法为便捷接口，从DAPyNodeMetaData中提取qualifiedName，
 * 然委托给createNodeProxy(const QString&)方法完成实际的代理创建。
 * 如果元数据无效（qualifiedName为空），直接返回nullptr。
 *
 * @param[in] metaData 节点元数据对象
 * @return 创建的DAPyNodeProxy实例指针，元数据无效时返回nullptr
 * @note 此方法不存储元数据到代理对象，代理对象仍通过qualifiedName标识
 * @see createNodeProxy(const QString&)
 */
DAPyNodeProxy* DAPyNodeFactory::createNodeProxy(const DAPyNodeMetaData& metaData)
{
    if (!metaData.isValid()) {
        return nullptr;
    }
    return createNodeProxy(metaData.qualifiedName);
}

/**
 * @brief 获取所有已发现节点的元数据列表
 *
 * @return DAPyNodeMetaData列表
 */
QList< DAPyNodeMetaData > DAPyNodeFactory::getNodeMetadataList() const
{
    DA_DC(d);
    return d->mNodeMetaDataList;
}

/**
 * @brief 获取所有已发现节点的原型标识列表
 *
 * @return 原型字符串列表
 */
QStringList DAPyNodeFactory::getNodePrototypes() const
{
    DA_DC(d);
    QStringList res;
    res.reserve(d->mNodeMetaDataList.size());
    for (const DAPyNodeMetaData& meta : d->mNodeMetaDataList) {
        res.append(meta.qualifiedName);
    }
    return res;
}

/**
 * @brief 工厂名称
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
 * @return 工厂描述文本
 */
QString DAPyNodeFactory::factoryDescribe() const
{
    return u8"Python工作流节点工厂，通过DANodeRegistry发现和创建Python定义的节点";
}

/**
 * @brief 获取最后的错误信息
 *
 * 在discoverNodes()或createNodeProxy()执行失败后，
 * 错误信息存储在PrivateData::mLastErrorString中。
 *
 * @return 最后一次错误的描述字符串，若无错误返回空字符串
 */
QString DAPyNodeFactory::getLastErrorString() const
{
    DA_DC(d);
    return d->mLastErrorString;
}

}  // namespace DA

DA_AUTO_REGISTER_META_TYPE(DA::DAPyNodeMetaData)
