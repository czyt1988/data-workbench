#include "DAPluginOption.h"
#include <QObject>
#include <memory>
#include <QPluginLoader>
#include "DALogCategory.h"

namespace DA
{
class DAPluginOption::PrivateData
{
    DA_DECLARE_PUBLIC(DAPluginOption)
public:
    PrivateData(DAPluginOption* p);
    DAAbstractPlugin* mPlugin { nullptr };
    std::shared_ptr< QPluginLoader > mLib;
};

//===================================================
// DAPluginOptionPrivate
//===================================================

DAPluginOption::PrivateData::PrivateData(DAPluginOption* p) : q_ptr(p)
{
}
//===================================================
// DAPluginOption
//===================================================
DAPluginOption::DAPluginOption() : DA_PIMPL_CONSTRUCT
{
    d_ptr->mLib.reset(new QPluginLoader);
}

/**
 * @brief 复制构造函数
 *
 * 这时候会共享QLibrary的内存
 * @param other
 */
DAPluginOption::DAPluginOption(const DAPluginOption& other)
{
    d_ptr.reset(new DAPluginOption::PrivateData(this));
    (*d_ptr) = *(other.d_ptr);
}

DAPluginOption::DAPluginOption(DAPluginOption&& other)
{
    d_ptr = std::move(other.d_ptr);
}

DAPluginOption& DAPluginOption::operator=(const DAPluginOption& other)
{
    if (nullptr == d_ptr) {
        d_ptr = std::make_unique< DAPluginOption::PrivateData >(this);
    }
    (*d_ptr) = *(other.d_ptr);
    return (*this);
}

DAPluginOption& DAPluginOption::operator=(DAPluginOption&& other)
{
    if (this != &other) {
        d_ptr = std::move(other.d_ptr);
    }
    return (*this);
}

DAPluginOption::~DAPluginOption()
{
}

/**
 * @brief 判断是否是有效的
 * @return
 */
bool DAPluginOption::isValid() const
{
    return (d_ptr->mLib != nullptr && d_ptr->mPlugin != nullptr);
}

/**
 * @brief 加载插件
 * @param pluginPath
 * @param c
 * @return
 */
bool DAPluginOption::load(const QString& pluginPath, DACoreInterface* c)
{
    if (d_ptr->mLib == nullptr) {
        d_ptr->mLib = std::make_shared< QPluginLoader >();
    }
    d_ptr->mLib->setFileName(pluginPath);
    if (!(d_ptr->mLib->load())) {
        // QPluginLoader::errorString() already provides the accurate failure reason
        daWarning << QObject::tr("Failed to load %1 (Reason: %2)").arg(getFileName(), getErrorString());  // cn:加载 %1 失败（原因：%2）
        return (false);
    }

    // 最后创建一个插件
    QObject* obj = d_ptr->mLib->instance();
    if (nullptr == obj) {
        daWarning << QObject::tr("Failed to create plugin instance from %1. Error: %2")
                         .arg(getFileName(), getErrorString());  // cn:无法从 %1 创建插件实例。错误：%2
        d_ptr->mLib->unload();
        return (false);
    }

    //! 这里必须用dynamic_cast，由于Q_DECLARE_INTERFACE只会识别一层继承，DAAbstractNodePlugin继承DAAbstractPlugin
    //! 继承DAAbstractNodePlugin的插件通过qobject_cast无法转换到DAAbstractPlugin，只能通过dynamic_cast转换
    //! 因此d_ptr->_plugin = qobject_cast< DAAbstractPlugin* >(obj);这样写会导致继承DAAbstractNodePlugin的插件无法加载
    d_ptr->mPlugin = dynamic_cast< DAAbstractPlugin* >(obj);
    if (nullptr == d_ptr->mPlugin) {
        daWarning << QObject::tr("Failed to cast to DA plugin interface: %1").arg(getFileName());  // cn:无法转换到 DA 插件接口：%1
        d_ptr->mLib->unload();
        return (false);
    }
    // 设置core
    d_ptr->mPlugin->setCore(c);
    daInfo << "loaded plugin:" << pluginPath;  // cn:已加载插件
    // 设置core后调用初始化
    if (!d_ptr->mPlugin->initialize()) {
        // 初始化失败，停止加载
        daWarning << QObject::tr("successfully loaded plugin %1, but failed to initialize")
                         .arg(getFileName());  // cn:成功加载插件 %1，但插件初始化失败
        d_ptr->mPlugin = nullptr;
        d_ptr->mLib->unload();
        d_ptr->mLib.reset();
        return (false);
    }
    return (true);
}

bool DAPluginOption::unload()
{
    if (d_ptr->mLib) {
        if (d_ptr->mLib->unload()) {
            d_ptr->mPlugin = nullptr;
            return true;
        }
    }
    return false;
}

/**
 * @brief 错误信息
 * @return
 */
QString DAPluginOption::getErrorString() const
{
    if (d_ptr->mLib) {
        return (d_ptr->mLib->errorString());
    }
    return (QString());
}

QString DAPluginOption::getFileName() const
{
    if (d_ptr->mLib) {
        return (d_ptr->mLib->fileName());
    }
    return (QString());
}

/**
 * @brief 获取iid
 * @return
 */
QString DAPluginOption::getIid() const
{
    if (plugin()) {
        return (plugin()->getIID());
    }
    return (QString());
}

/**
 * @brief 获取插件
 * @return
 */
DAAbstractPlugin* DAPluginOption::plugin() const
{
    return (d_ptr->mPlugin);
}

/**
 * @brief 获取插件的名称
 * @return
 */
QString DAPluginOption::getPluginName() const
{
    if (plugin()) {
        return (plugin()->getName());
    }
    return (QString());
}

/**
 * @brief 获取插件描述
 * @return
 */
QString DAPluginOption::getPluginDescription() const
{
    if (plugin()) {
        return (plugin()->getDescription());
    }
    return (QString());
}

/**
 * @brief 获取插件版本
 * @return
 */
QString DAPluginOption::getPluginVersion() const
{
    if (plugin()) {
        return (plugin()->getVersion());
    }
    return (QString());
}

/**
 * @brief operator <<
 * @param debug
 * @param po
 * @return
 */
QDebug operator<<(QDebug debug, const DAPluginOption& po)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << QObject::tr("plugin file name:%1,iid:%2,name:%3,description:%4,version:%5,error string:%6")
                           .arg(po.getFileName(),
                                po.getIid(),
                                po.getPluginName(),
                                po.getPluginDescription(),
                                po.getPluginVersion(),
                                po.getErrorString())  // cn:插件文件名：%1，iid：%2，名称：%3，描述：%4，版本：%5，错误信息：%6
#if QT_VERSION_MAJOR >= 6
                    << Qt::endl;
#else
                    << endl;
#endif
    return (debug);
}
}  // namespace DA
