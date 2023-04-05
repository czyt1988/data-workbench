#include "DAPluginManager.h"
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include "DAPluginOption.h"
#include <QLibrary>
namespace DA
{
class DAPluginManagerPrivate
{
    DA_IMPL_PUBLIC(DAPluginManager)
public:
    DAPluginManagerPrivate(DAPluginManager* p);

    QDir _pluginDir;
    QList< DAPluginOption > _pluginOptions;
    bool _isLoaded;  ///< 标记是否加载了，可以只加载一次
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPluginManagerPrivate
//===================================================

DAPluginManagerPrivate::DAPluginManagerPrivate(DAPluginManager* p) : q_ptr(p), _isLoaded(false)
{
    _pluginDir.setPath(QCoreApplication::applicationDirPath() + QDir::separator() + "plugins");
}

//===================================================
// DAPluginManager
//===================================================

DAPluginManager::DAPluginManager(QObject* p) : QObject(p), d_ptr(new DAPluginManagerPrivate(this))
{
}

DAPluginManager::~DAPluginManager()
{
}

DAPluginManager& DAPluginManager::instance()
{
    static DAPluginManager s_plugin_mgr;

    return (s_plugin_mgr);
}

/**
 * @brief FCPluginManager::setIgnoreList
 * @param ignorePluginsName
 * @todo 还未实现
 */
void DAPluginManager::setIgnoreList(const QStringList ignorePluginsName)
{
    // TODO 给插件设置忽略名
}

/**
 * @brief 加载插件
 */
void DAPluginManager::load(DACoreInterface* c)
{
    QFileInfoList fileInfos = d_ptr->_pluginDir.entryInfoList(QDir::Files);

    qInfo() << tr("plugin dir is:%1").arg(d_ptr->_pluginDir.absolutePath());
    for (const QFileInfo& fi : qAsConst(fileInfos)) {
        const QString filepath = fi.absoluteFilePath();
        if (!QLibrary::isLibrary(filepath)) {
            qWarning() << tr(" ignore invalid file:%1").arg(fi.absoluteFilePath());
            continue;
        }
        DAPluginOption pluginopt;
        emit beginLoadPlugin(fi.absoluteFilePath());
        if (!pluginopt.load(fi.absoluteFilePath(), c)) {
            qDebug() << tr("can not load plugin:%1").arg(fi.absoluteFilePath());
            continue;
        }
        d_ptr->_pluginOptions.append(pluginopt);
    }
    d_ptr->_isLoaded = true;
}

bool DAPluginManager::isLoaded() const
{
    return (d_ptr->_isLoaded);
}

/**
 * @brief 设置插件路径，可以多次load，同一个插件（插件名称和类型组成一个key）只会加载一次
 */
void DAPluginManager::setPluginPath(const QString& path)
{
    d_ptr->_pluginDir.setPath(path);
}

/**
 * @brief 获取加载成功插件的数量
 * @return
 */
int DAPluginManager::getPluginCount() const
{
    return (d_ptr->_pluginOptions.size());
}

/**
 * @brief 获取加载成功插件的插件名
 * @return
 */
QList< QString > DAPluginManager::getPluginNames() const
{
    QList< QString > res;

    for (const DAPluginOption& opt : qAsConst(d_ptr->_pluginOptions)) {
        res.append(opt.getPluginName());
    }
    return (res);
}

/**
 * @brief 获取所有插件信息
 * @return
 */
QList< DAPluginOption > DAPluginManager::getPluginOptions() const
{
    return (d_ptr->_pluginOptions);
}

QDebug operator<<(QDebug debug, const DAPluginManager& fmg)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << DAPluginManager::tr("Plugin Manager Info:is loaded=%1,plugin counts=%2").arg(fmg.isLoaded()).arg(fmg.getPluginCount())
                    << endl;
    QList< DAPluginOption > opts = fmg.getPluginOptions();

    for (const DAPluginOption& opt : qAsConst(opts)) {
        debug.nospace() << opt;
    }
    return (debug);
}
