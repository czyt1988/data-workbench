#include "DAPluginOption.h"
#include <QObject>
#include <QLibrary>
#include <memory>
#include <QPluginLoader>
namespace DA
{
class DAPluginOptionPrivate
{
    DA_IMPL_PUBLIC(DAPluginOption)
public:
    DAPluginOptionPrivate(DAPluginOption* p);
    DAAbstractPlugin* _plugin = { nullptr };
    std::shared_ptr< QPluginLoader > _lib;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPluginOptionPrivate
//===================================================

DAPluginOptionPrivate::DAPluginOptionPrivate(DAPluginOption* p) : q_ptr(p)
{
}
//===================================================
// DAPluginOption
//===================================================
DAPluginOption::DAPluginOption() : d_ptr(new DAPluginOptionPrivate(this))
{
    d_ptr->_lib.reset(new QPluginLoader);
}

/**
 * @brief 复制构造函数
 *
 * 这时候会共享QLibrary的内存
 * @param other
 */
DAPluginOption::DAPluginOption(const DAPluginOption& other)
{
    d_ptr.reset(new DAPluginOptionPrivate(this));
    (*d_ptr) = *(other.d_ptr);
}

DAPluginOption::DAPluginOption(DAPluginOption&& other)
{
    d_ptr.reset(other.d_ptr.take());
}

DAPluginOption& DAPluginOption::operator=(const DAPluginOption& other)
{
    if (d_ptr.isNull()) {
        d_ptr.reset(new DAPluginOptionPrivate(this));
    }
    (*d_ptr) = *(other.d_ptr);
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
    return (nullptr != d_ptr->_lib);
}

/**
 * @brief 加载插件
 * @param pluginPath
 * @param c
 * @return
 */
bool DAPluginOption::load(const QString& pluginPath, DACoreInterface* c)
{
    if (d_ptr->_lib == nullptr) {
        d_ptr->_lib = std::make_shared< QPluginLoader >();
    }
    d_ptr->_lib->setFileName(pluginPath);
    if (!(d_ptr->_lib->load())) {
        qWarning() << QObject::tr("Failed to load %1 (Reason: %2)").arg(getFileName(), getErrorString());
        return (false);
    }

    //最后创建一个插件
    QObject* obj = d_ptr->_lib->instance();
    if (nullptr == obj) {
        qWarning() << QObject::tr("Failed to create plugin instance %1 (Reason: %2)").arg(getFileName(), getErrorString());
        return (false);
    }

    //! 这里必须用dynamic_cast，由于Q_DECLARE_INTERFACE只会识别一层继承，DAAbstractNodePlugin继承DAAbstractPlugin
    //! 继承DAAbstractNodePlugin的插件通过qobject_cast无法转换到DAAbstractPlugin，只能通过dynamic_cast转换
    //! 因此d_ptr->_plugin = qobject_cast< DAAbstractPlugin* >(obj);这样写会导致继承DAAbstractNodePlugin的插件无法加载
    d_ptr->_plugin = dynamic_cast< DAAbstractPlugin* >(obj);
    if (nullptr == d_ptr->_plugin) {
        qWarning() << QObject::tr("Failed to cast plugin to DA plugin %1").arg(getFileName());
        return (false);
    }
    //设置core
    d_ptr->_plugin->setCore(c);
    qDebug() << QObject::tr("loaded plugin:%1").arg(pluginPath);
    //设置core后调用初始化
    if (!d_ptr->_plugin->initialize()) {
        //初始化失败，停止加载
        qWarning() << QObject::tr("success load plugin %1,but failed to initialize").arg(getFileName());  // cn:成功加载插件%1,但插件的初始化失败
        d_ptr->_plugin = nullptr;
        d_ptr->_lib.reset();
        return (false);
    }
    return (true);
}

bool DAPluginOption::unload()
{
    if (d_ptr->_lib) {
        if (d_ptr->_lib->unload()) {
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
    return (d_ptr->_lib->errorString());
}

QString DAPluginOption::getFileName() const
{
    return (d_ptr->_lib->fileName());
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
    return (d_ptr->_plugin);
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
    debug.nospace()
    << QObject::tr("plugin file name:%1,iid:%2,name:%3,description:%4,version:%5,error string:%6")
       .arg(po.getFileName(), po.getIid(), po.getPluginName(), po.getPluginDescription(), po.getPluginVersion(), po.getErrorString())
    << endl;
    return (debug);
}
