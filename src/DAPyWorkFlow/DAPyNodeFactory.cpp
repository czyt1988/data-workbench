#include "DAPyNodeFactory.h"
#include "DAPybind11InQt.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyNodeProxy.h"
#include "DAPyInterpreter.h"
#include <QDebug>
#include <QHash>

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
    DAPyGILGuard gil;

    try {
        // 1. 将扫描路径添加到Python sys.path
        for (const QString& path : scanPaths) {
            DAPyInterpreter::appendSysPath(path);
        }

        // 2. 获取DAPyModuleWorkflow单例并导入模块
        DAPyModuleWorkflow& pyModule = DAPyModuleWorkflow::getInstance();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                mLastErrorString = "无法导入DAWorkbench.DAWorkFlowPy模块";
                qCritical() << mLastErrorString;
                return false;
            }
        }

        // 3. 获取DANodeFactory类并创建实例，缓存到DAPyObjectWrapper的_object中
        pybind11::object factoryClass = pyModule.getNodeFactoryClass();
        if (factoryClass.is_none()) {
            mLastErrorString = "无法获取DANodeFactory类引用";
            qCritical() << mLastErrorString;
            return false;
        }

        _object = factoryClass();

        // 4. 构建Python参数并调用discover
        pybind11::list pyScanPaths;
        for (const QString& path : scanPaths) {
            pyScanPaths.append(path.toStdString());
        }

        pybind11::object result = _object.attr("discover")(pyScanPaths, useEntryPoints);

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
                metaData.group = nodeClassObj.attr("category").cast< QString >();
            } else if (pybind11::hasattr(nodeClassObj, "group")) {
                metaData.group = nodeClassObj.attr("group").cast< QString >();
            }
            if (pybind11::hasattr(nodeClassObj, "icon")) {
                metaData.iconPath = nodeClassObj.attr("icon").cast< QString >();
            }

            // input_keys/output_keys
            if (pybind11::hasattr(nodeClassObj, "input_keys")) {
                pybind11::list inKeys = nodeClassObj.attr("input_keys").cast< pybind11::list >();
                for (auto k : inKeys) {
                    metaData.inputKeys.append(pybind11::cast< QString >(k));
                }
            }
            if (pybind11::hasattr(nodeClassObj, "output_keys")) {
                pybind11::list outKeys = nodeClassObj.attr("output_keys").cast< pybind11::list >();
                for (auto k : outKeys) {
                    metaData.outputKeys.append(pybind11::cast< QString >(k));
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
        mLastErrorString = QString("Python异常: %1").arg(e.what());
        dealException(e);
        return false;
    } catch (const std::exception& e) {
        mLastErrorString = QString("异常: %1").arg(e.what());
        dealException(e);
        return false;
    }
}

/**
 * @brief 通过限定名创建DAPyNodeProxy实例
 *
 * 核心创建流程：
 * 1. 获取GIL保护（DAPyGILGuard RAII）
 * 2. 检查Python DANodeFactory实例（DAPyObjectWrapper的_object）是否有效
 * 3. 调用DANodeFactory.create_node(qualified_name)获取Python节点实例
 * 4. 创建DAPyNodeProxy并设置Python节点引用
 *
 * 节点实例化由Python侧DANodeFactory完成（通过DANodeRegistry.get_descriptor获取类并实例化），
 * C++ 侧不再自行解析qualified_name进行module_::import和类名查找。
 *
 * @param[in] qualifiedName Python节点的限定名（如"pkg.module.ClassName"）
 * @return 成功返回DAPyNodeProxy指针，失败返回nullptr
 * @note 返回的DAPyNodeProxy由调用方负责生命周期管理
 * @note 必须先调用discoverNodes()创建Python factory实例，否则返回nullptr
 */
DAPyNodeProxy* DAPyNodeFactory::createNodeProxy(const QString& qualifiedName)
{
    DAPyGILGuard gil;

    try {
        // 检查Python factory实例是否有效（DAPyObjectWrapper的_object即为Python DANodeFactory实例）
        if (isNone() || !(*this)) {
            mLastErrorString = "Python DANodeFactory实例未初始化，请先调用discoverNodes()";
            qWarning() << mLastErrorString;
            return nullptr;
        }

        // 调用Python DANodeFactory.create_node()获取节点实例
        pybind11::object pyNodeInstance = _object.attr("create_node")(qualifiedName.toStdString());

        if (pyNodeInstance.is_none()) {
            mLastErrorString = QString("Python DANodeFactory.create_node(%1)返回None").arg(qualifiedName);
            qWarning() << mLastErrorString;
            return nullptr;
        }

        // 创建DAPyNodeProxy并设置Python节点引用
        DAPyNodeProxy* proxy = new DAPyNodeProxy(pyNodeInstance);

        return proxy;

    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = QString("创建Python节点实例失败: %1").arg(e.what());
        dealException(e);
        return nullptr;
    } catch (const std::exception& e) {
        mLastErrorString = QString("创建Python节点实例失败: %1").arg(e.what());
        dealException(e);
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
 * 错误信息存储在mLastErrorString中。
 *
 * @return 最后一次错误的描述字符串，若无错误返回空字符串
 */
QString DAPyNodeFactory::getLastErrorString() const
{
    return mLastErrorString;
}

}  // namespace DA

DA_AUTO_REGISTER_META_TYPE(DA::DAPyNodeMetaData)
