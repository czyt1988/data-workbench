#include "DAPyNodeFactory.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyModuleWorkflow.h"
#include "DAPyNode.h"
#include "DAPyInterpreter.h"
#include <QDebug>
#include <QHash>
#include <QObject>
namespace DA
{

//===================================================
// DAPyNodeFactory
//===================================================

/**
 * @brief DAPyNodeFactory构造函数
 *
 * 构造Python节点工厂代理，继承DAPyObjectWrapper。
 * Python DANodeFactory实例在discoverNodes()中创建并缓存到DAPyObjectWrapper的_object中。
 */
DAPyNodeFactory::DAPyNodeFactory() : DAPyObjectWrapper()
{
    DAPyModuleWorkflow pyModule = DAPyModuleWorkflow();
    if (!pyModule.isImport()) {
        if (!pyModule.import()) {
            qWarning() << "DAPyNodeFactory: cannot import DAWorkbench.DAWorkFlowPy";
            return;
        }
    }
    pybind11::object factoryClass = pyModule.getNodeFactoryObject();
    object()                      = factoryClass();
}

DAPyNodeFactory::DAPyNodeFactory(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
}

DAPyNodeFactory::DAPyNodeFactory(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
}

DAPyNodeFactory::DAPyNodeFactory(const DAPyObjectWrapper& obj) : DAPyObjectWrapper(obj)
{
}

DAPyNodeFactory::DAPyNodeFactory(const DAPyNode& obj) : DAPyObjectWrapper(obj)
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
 * 通过DAPyModuleWorkflow获取Python侧的DANodeFactory类，创建实例并调用discover()方法，
 * 将返回的节点类列表转换为C++的DAPyNodeMetaData并缓存到工厂中。
 * Python DANodeFactory实例缓存到DAPyObjectWrapper的_object中，供后续createNodeProxy()使用。
 *
 * 发现流程：
 * 1. 将scanPaths添加到Python sys.path（通过DAPyInterpreter::appendSysPath）
 * 2. 获取DAPyModuleWorkflow单例并导入DAWorkbench.DAWorkFlowPy模块
 * 3. 获取DANodeFactory类并创建Python实例，缓存到DAPyObjectWrapper的_object中
 * 4. 调用DANodeFactory.discover(scan_paths, use_entry_points)
 * 5. 遍历返回的节点类列表，从类属性读取元数据并转换为DAPyNodeMetaData
 *
 * @param[in] scanPaths 要扫描的目录路径列表，这些路径会被添加到Python sys.path
 * @param[in] useEntryPoints 是否使用entry_points发现节点
 * @return 发现成功返回true，失败返回false
 */
bool DAPyNodeFactory::discoverNodes(const QStringList& scanPaths, bool useEntryPoints)
{

    try {
        pybind11::list pyScanPaths = DA::PY::toPyObject(scanPaths);

        pybind11::object result = attr("discover")(pybind11::arg("scan_paths")       = pyScanPaths,
                                                   pybind11::arg("use_entry_points") = useEntryPoints);

        // 5. 遍历返回的节点类列表，直接从类属性读取元数据
        QList< DAPyNodeMetaData > discoveredList;
        for (pybind11::handle item : result) {
            pybind11::object nodeClassObj = pybind11::reinterpret_borrow< pybind11::object >(item);

            DAPyNodeMetaData metaData;

            // 从 Python 类属性直接读取
            if (pybind11::hasattr(nodeClassObj, "qualified_name")) {
                metaData.qualifiedName = nodeClassObj.attr("qualified_name").cast< QString >();
            }
            if (pybind11::hasattr(nodeClassObj, "name")) {
                metaData.name = nodeClassObj.attr("name").cast< QString >();
            }
            if (pybind11::hasattr(nodeClassObj, "category")) {
                metaData.category = nodeClassObj.attr("category").cast< QString >();
            }
            if (pybind11::hasattr(nodeClassObj, "icon")) {
                metaData.iconPath = nodeClassObj.attr("icon").cast< QString >();
            }
            if (pybind11::hasattr(nodeClassObj, "input_keys")) {
                pybind11::list pyKeys = nodeClassObj.attr("input_keys").cast< pybind11::list >();
                for (auto item : pyKeys) {
                    metaData.inputKeys.append(pybind11::cast< QString >(item));
                }
            }
            if (pybind11::hasattr(nodeClassObj, "output_keys")) {
                pybind11::list pyKeys = nodeClassObj.attr("output_keys").cast< pybind11::list >();
                for (auto item : pyKeys) {
                    metaData.outputKeys.append(pybind11::cast< QString >(item));
                }
            }

            if (!metaData.isValid()) {
                qWarning() << "发现无效的节点元数据，跳过";
                continue;
            }

            discoveredList.append(metaData);
        }

        // 更新缓存列表
        mNodeMetaDataList = discoveredList;

        qDebug() << "DAPyNodeFactory discovered" << mNodeMetaDataList.size() << "Python nodes";
        return true;

    } catch (const pybind11::error_already_set& e) {
        dealException(e);
        return false;
    } catch (const std::exception& e) {
        dealException(e);
        return false;
    }
}

/**
 * @brief 通过限定名创建DAPyNode实例
 *
 * 核心创建流程：
 * 1. 获取GIL保护（DAPyGILGuard RAII）
 * 2. 检查Python DANodeFactory实例（DAPyObjectWrapper的_object）是否有效
 * 3. 调用DANodeFactory.create_node(qualified_name)获取Python节点实例
 * 4. 创建DAPyNode并设置Python节点引用
 *
 * 节点实例化由Python侧DANodeFactory完成（通过DANodeRegistry.get_descriptor获取类并实例化），
 * C++ 侧不再自行解析qualified_name进行module_::import和类名查找。
 *
 * @param[in] qualifiedName Python节点的限定名（如"pkg.module.ClassName"）
 * @return 成功返回DAPyNode值类型实例，失败返回默认构造的DAPyNode（isNone()为true）
 * @note 必须先调用discoverNodes()创建Python factory实例，否则返回无效的DAPyNode
 */
DAPyNode DAPyNodeFactory::createNode(const QString& qualifiedName)
{
    try {
        // 调用Python DANodeFactory.create_node()获取节点实例
        pybind11::object pyNodeInstance = attr("create_node")(qualifiedName.toStdString());
        if (pyNodeInstance.is_none()) {
            return DAPyNode();
        }
        // 创建DAPyNode并设置Python节点引用
        return DAPyNode(pyNodeInstance);
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
        return DAPyNode();
    } catch (const std::exception& e) {
        dealException(e);
        return DAPyNode();
    }
}

/**
 * @brief 通过节点元数据创建DAPyNode实例
 *
 * 此方法为便捷接口，从DAPyNodeMetaData中提取qualifiedName，
 * 然后委托给createNode(const QString&)方法完成实际的代理创建。
 * 如果元数据无效（qualifiedName为空），直接返回默认构造的DAPyNode。
 *
 * @param[in] metaData 节点元数据对象
 * @return 创建的DAPyNode值类型实例，元数据无效时返回默认构造的DAPyNode（isNone()为true）
 * @see createNode(const QString&)
 */
DAPyNode DAPyNodeFactory::createNode(const DAPyNodeMetaData& metaData)
{
    if (!metaData.isValid()) {
        return DAPyNode();
    }
    return createNode(metaData.qualifiedName);
}

/**
 * @brief 获取所有已发现节点的元数据列表
 *
 * @return DAPyNodeMetaData列表
 */
QList< DAPyNodeMetaData > DAPyNodeFactory::getNodeMetadataList() const
{
    return mNodeMetaDataList;
}

/**
 * @brief 获取所有已发现节点的原型标识列表
 *
 * @return 原型字符串列表
 */
QStringList DAPyNodeFactory::getNodePrototypes() const
{
    QStringList res;
    res.reserve(mNodeMetaDataList.size());
    for (const DAPyNodeMetaData& meta : mNodeMetaDataList) {
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
    return QObject::tr("DA Python Node Factory");
}

/**
 * @brief 工厂描述
 *
 * @return 工厂描述文本
 */
QString DAPyNodeFactory::factoryDescribe() const
{
    return QObject::tr("Python工作流节点工厂，通过DANodeRegistry发现和创建Python定义的节点");
}

}  // namespace DA

DA_AUTO_REGISTER_META_TYPE(DA::DAPyNodeMetaData)
