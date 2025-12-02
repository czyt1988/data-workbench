#include "DAPluginManager.h"
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include "DAPluginOption.h"
#include <QLibrary>
#include <QFile>
#include <QTextStream>
namespace DA
{
class DAPluginManager::PrivateData
{
    DA_DECLARE_PUBLIC(DAPluginManager)
public:
    PrivateData(DAPluginManager* p);
    // 判断是否有ignore文件
    bool hasIgnoreFile() const;
    // 更新忽略set
    void updateIgnoreSet();
    // 创建忽略文件，如果已经存在将跳过
    void ensureIgnoreFileExist();
    //
    static QString getIgnoreFilePath();

public:
    QDir mPluginDir;
    QList< DAPluginOption > mPluginOptions;
    bool mIsLoaded { false };               ///< 标记是否加载了，可以只加载一次
    QSet< QString > mIgnorePluginBaseName;  ///< 记录忽略插件的基本名字
};

//===================================================
// DAPluginManagerPrivate
//===================================================

DAPluginManager::PrivateData::PrivateData(DAPluginManager* p) : q_ptr(p)
{
    QString pluginPath = DAPluginManager::getPluginDirPath();
    mPluginDir.mkpath(pluginPath);
    mPluginDir.setPath(pluginPath);
    updateIgnoreSet();
}

bool DAPluginManager::PrivateData::hasIgnoreFile() const
{
    return QFile::exists(getIgnoreFilePath());
}

void DAPluginManager::PrivateData::updateIgnoreSet()
{
    ensureIgnoreFileExist();
    QFile ignoreFile(getIgnoreFilePath());
    if (!ignoreFile.open(QIODevice::ReadOnly)) {
        qWarning() << DAPluginManager::tr(
                          "The file .pluginignore exists, but it failed to read due to the following reason:%1")
                          .arg(ignoreFile.errorString());
        return;
    }
    QTextStream ss(&ignoreFile);
    while (!ss.atEnd()) {
        QString line = ss.readLine();
        line         = line.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (line.at(0) == '#') {
            continue;
        }
        mIgnorePluginBaseName.insert(line.toLower());
    }
    qDebug() << "will ignore plugin:" << mIgnorePluginBaseName;
}

void DAPluginManager::PrivateData::ensureIgnoreFileExist()
{
    if (hasIgnoreFile()) {
        return;
    }
    QFile ignoreFile(getIgnoreFilePath());
    qDebug() << DAPluginManager::tr("No plugins ignore files, a %1 file will be automatically generated")
                    .arg(ignoreFile.fileName());  // cn:缺少插件忽略文件，将自动生成.pluginignore文件
    if (!ignoreFile.exists()) {
        // 不存在，则创建一个
        if (ignoreFile.open(QIODevice::ReadWrite)) {
            QTextStream txt(&ignoreFile);
#if QT_VERSION_MAJOR >= 6
            txt.setEncoding(QStringConverter::Utf8);
#else
            txt.setCodec("utf-8");
#endif
            txt << u8"# pluginignore file,Plugins that you do not want to load are described in this file,only write "
                   u8"the "
                   "file base name, do not need to write suffixes"
#if QT_VERSION_MAJOR >= 6
                << Qt::endl;
#else
                << endl;
#endif
            txt << u8"# 不想加载的插件在此文件描述，写入基本文件名，无需后缀"
#if QT_VERSION_MAJOR >= 6
                << Qt::endl;
#else
                << endl;
#endif
        }
    }
    ignoreFile.close();
}

QString DAPluginManager::PrivateData::getIgnoreFilePath()
{
    return QDir::toNativeSeparators(DAPluginManager::getPluginDirPath() + QDir::separator() + getPluginIgnoreFileName());
}

//===================================================
// DAPluginManager
//===================================================

DAPluginManager::DAPluginManager(QObject* p) : QObject(p), DA_PIMPL_CONSTRUCT
{
}

DAPluginManager::~DAPluginManager()
{
}


/**
 * @brief FCPluginManager::setIgnoreList
 * @param ignorePluginsName
 */
void DAPluginManager::setIgnoreList(const QStringList ignorePluginsName)
{
    d_ptr->mIgnorePluginBaseName = QSet< QString >(ignorePluginsName.begin(), ignorePluginsName.end());
}

/**
 * @brief 加载插件
 */
void DAPluginManager::loadAllPlugins(DACoreInterface* c)
{
    const QFileInfoList fileInfos = d_ptr->mPluginDir.entryInfoList(QDir::Files);

    qInfo().noquote() << tr("plugin dir is:%1").arg(d_ptr->mPluginDir.absolutePath());
    for (const QFileInfo& fi : fileInfos) {
        if (fi.baseName().isEmpty()) {
            //".开头的文件，如.pluginignore"
            continue;
        }

        // 跨平台的动态库后缀检查
        QString suffix = fi.suffix().toLower();
        bool isLibrary = false;
#ifdef Q_OS_WIN
        isLibrary = (suffix == "dll");
#elif defined(Q_OS_MACOS)
        isLibrary = (suffix == "dylib") || (suffix == "so");  // 有时macOS也用.so
#else  // Unix/Linux
        isLibrary = (suffix == "so");
#endif
        if (!isLibrary) {
            continue;
        }
        if (d_ptr->mIgnorePluginBaseName.contains(fi.baseName().toLower())) {
            qInfo().noquote() << tr("ignore plugin %1").arg(fi.baseName());
            continue;
        }
        const QString filepath = fi.absoluteFilePath();
        if (!QLibrary::isLibrary(filepath)) {
            qWarning().noquote() << tr(" ignore invalid file:%1").arg(fi.absoluteFilePath());
            continue;
        }
        DAPluginOption pluginopt;
        emit beginLoadPlugin(fi.absoluteFilePath());
        if (!pluginopt.load(fi.absoluteFilePath(), c)) {
            qDebug() << tr("can not load plugin:%1").arg(fi.absoluteFilePath());
            continue;
        }
        d_ptr->mPluginOptions.append(pluginopt);
    }
    d_ptr->mIsLoaded = true;
}

bool DAPluginManager::isLoaded() const
{
    return (d_ptr->mIsLoaded);
}

/**
 * @brief 设置插件路径，可以多次load，同一个插件（插件名称和类型组成一个key）只会加载一次
 */
void DAPluginManager::setPluginPath(const QString& path)
{
    d_ptr->mPluginDir.setPath(path);
}

/**
 * @brief 获取加载成功插件的数量
 * @return
 */
int DAPluginManager::getPluginCount() const
{
    return (d_ptr->mPluginOptions.size());
}

/**
 * @brief 获取加载成功插件的插件名
 * @return
 */
QList< QString > DAPluginManager::getPluginNames() const
{
    QList< QString > res;

    for (const DAPluginOption& opt : qAsConst(d_ptr->mPluginOptions)) {
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
    return (d_ptr->mPluginOptions);
}

bool DAPluginManager::unloadPlugin(const QString& pluginName)
{
    for (auto it = d_ptr->mPluginOptions.begin(); it != d_ptr->mPluginOptions.end(); ++it) {
        if (it->getPluginName() == pluginName) {
            // 1. 调用插件的 finalize() 进行清理
            DAAbstractPlugin* plugin = it->plugin();
            if (plugin && !plugin->finalize()) {
                qWarning() << tr("Plugin %1 refused to finalize, unload cancelled.").arg(pluginName);  // cn:插件:%1 拒绝完成，卸载已取消
                return false;
            }
            // 2. 卸载插件库
            if (it->unload()) {
                Q_EMIT pluginUnloaded(pluginName);  // 建议新增此信号
                d_ptr->mPluginOptions.erase(it);
                return true;
            } else {
                qWarning() << tr("Failed to unload plugin library for %1.").arg(pluginName);  // cn:无法卸载插件%1
                return false;
            }
        }
    }
    qWarning() << tr("Plugin %1 not found for unloading.").arg(pluginName);  // cn:未能找到插件%1
    return false;
}

bool DAPluginManager::unloadAllPlugins()
{
    bool allSuccess = true;
    // 注意：反向迭代卸载，有时更安全，unloadPlugin执行后mPluginOptions的长度会变少1位，这样正好就顺着往下，不会受mPluginOptions长度变小影响
    for (int i = d_ptr->mPluginOptions.size() - 1; i >= 0; --i) {
        if (!unloadPlugin(d_ptr->mPluginOptions[ i ].getPluginName())) {
            allSuccess = false;
        }
    }
    return allSuccess;
}


/**
 * @brief 获取插件目录的绝对路径
 *
 * @return 返回插件的绝对路径
 */
QString DAPluginManager::getPluginDirPath()
{
    return QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/plugins");
}

/**
 * @brief 忽略文件名
 * @return
 */
QString DAPluginManager::getPluginIgnoreFileName()
{
    return QString(".pluginignore");
}

QDebug operator<<(QDebug debug, const DAPluginManager& fmg)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << DAPluginManager::tr("Plugin Manager Info:is loaded=%1,plugin counts=%2")
                           .arg(fmg.isLoaded())
                           .arg(fmg.getPluginCount())
#if QT_VERSION_MAJOR >= 6
                    << Qt::endl;
#else
                    << endl;
#endif
    QList< DAPluginOption > opts = fmg.getPluginOptions();

    for (const DAPluginOption& opt : qAsConst(opts)) {
        debug.nospace() << opt;
    }
    return (debug);
}
}  // namespace DA
