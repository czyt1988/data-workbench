#include "DAPluginManager.h"
#include "DALogCategory.h"
#include "DAPluginOption.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>
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
    // 将当前忽略set回写到.pluginignore文件，保留原文件中的注释行
    bool saveIgnoreFile();
    // 根据文件路径查找已加载的插件选项，未找到返回nullptr
    const DAPluginOption* findOptionByFilePath(const QString& filePath) const;
    //
    static QString getIgnoreFilePath();

public:
    QDir mPluginDir;
    QList< DAPluginOption > mPluginOptions;
    bool mIsLoaded { false };               ///< 标记是否加载了，可以只加载一次
    QSet< QString > mIgnorePluginBaseName;  ///< 记录忽略插件的基本名字
    QHash< QString, QString > mFailedPlugins;  ///< 加载失败的插件（小写baseName → 错误信息）
    QSet< QString > mDegradedPlugins;          ///< 降级停用的插件（小写baseName），finalize已执行但库释放失败
};

//===================================================
// DAPluginManagerPrivate
//===================================================

DAPluginManager::PrivateData::PrivateData(DAPluginManager* p) : q_ptr(p)
{
    QString pluginPath = DAPluginManager::getPluginDirPath();
    if (!mPluginDir.mkpath(pluginPath)) {
        daWarning << DAPluginManager::tr("Failed to create plugin directory: %1").arg(pluginPath);  // cn:创建插件目录失败：%1
    }
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
        daWarning << DAPluginManager::tr("The file .pluginignore exists, but failed to "
                                         "read due to the following reason: %1")
                         .arg(ignoreFile.errorString());  // cn:.pluginignore文件存在，但由于以下原因读取失败：%1
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
    daInfo << DAPluginManager::tr("Will ignore plugin:") << mIgnorePluginBaseName;  //cn:将忽略以下插件
}

void DAPluginManager::PrivateData::ensureIgnoreFileExist()
{
    if (hasIgnoreFile()) {
        return;
    }
    QFile ignoreFile(getIgnoreFilePath());
    daInfo << DAPluginManager::tr("No plugins ignore files, a %1 file will be "
                                  "automatically generated")
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
            txt << u8"# pluginignore file,Plugins that you do not want to load are "
                   u8"described in this file,only write "
                   u8"the "
                   "file base name, do not need to write suffixes"
                << Qt::endl;
            txt << u8"# 不想加载的插件在此文件描述，写入基本文件名，无需后缀"
                << Qt::endl;
        }
    }
    ignoreFile.close();
}

QString DAPluginManager::PrivateData::getIgnoreFilePath()
{
    return QDir::toNativeSeparators(DAPluginManager::getPluginDirPath() + QDir::separator() + getPluginIgnoreFileName());
}

bool DAPluginManager::PrivateData::saveIgnoreFile()
{
    const QString filePath = getIgnoreFilePath();
    // 先读出原文件的注释行，保留模板说明
    QStringList commentLines;
    QFile readFile(filePath);
    if (readFile.open(QIODevice::ReadOnly)) {
        QTextStream rss(&readFile);
        while (!rss.atEnd()) {
            QString line = rss.readLine();
            if (line.trimmed().startsWith(QLatin1Char('#'))) {
                commentLines.append(line);
            }
        }
        readFile.close();
    }
    QFile writeFile(filePath);
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        daWarning << DAPluginManager::tr("Failed to write plugin ignore file %1: %2")
                         .arg(filePath, writeFile.errorString());  // cn:写入插件忽略文件%1失败：%2
        return false;
    }
    QTextStream txt(&writeFile);
#if QT_VERSION_MAJOR >= 6
    txt.setEncoding(QStringConverter::Utf8);
#else
    txt.setCodec("utf-8");
#endif
    for (const QString& line : std::as_const(commentLines)) {
        txt << line << Qt::endl;
    }
    QStringList names = mIgnorePluginBaseName.values();
    names.sort(Qt::CaseInsensitive);
    for (const QString& name : std::as_const(names)) {
        txt << name << Qt::endl;
    }
    writeFile.close();
    return true;
}

const DAPluginOption* DAPluginManager::PrivateData::findOptionByFilePath(const QString& filePath) const
{
    const QString nativePath = QDir::toNativeSeparators(filePath);
    for (const DAPluginOption& opt : mPluginOptions) {
        if (QDir::toNativeSeparators(opt.getFileName()).compare(nativePath, Qt::CaseInsensitive) == 0) {
            return &opt;
        }
    }
    return nullptr;
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
void DAPluginManager::setIgnoreList(const QStringList& ignorePluginsName)
{
    d_ptr->mIgnorePluginBaseName.clear();
    for (const QString& name : ignorePluginsName) {
        d_ptr->mIgnorePluginBaseName.insert(name.toLower());
    }
}

/**
 * @brief 获取当前忽略（禁用）列表
 * @return 插件文件基本名列表（统一小写）
 */
QStringList DAPluginManager::ignoreList() const
{
    QStringList res = d_ptr->mIgnorePluginBaseName.values();
    res.sort(Qt::CaseInsensitive);
    return res;
}

/**
 * @brief 设置插件启用状态并持久化到.pluginignore文件
 *
 * enable为false时将插件加入忽略列表（下次启动不加载），为true时移出忽略列表。
 * 此函数只负责状态持久化，不会触发运行期的加载或卸载
 * @param pluginBaseName 插件文件基本名（不含后缀）
 * @param enable 是否启用
 * @return 是否成功写入配置文件
 */
bool DAPluginManager::setPluginEnabled(const QString& pluginBaseName, bool enable)
{
    const QString key = pluginBaseName.trimmed().toLower();
    if (key.isEmpty()) {
        return false;
    }
    if (enable) {
        d_ptr->mIgnorePluginBaseName.remove(key);
    } else {
        d_ptr->mIgnorePluginBaseName.insert(key);
    }
    return d_ptr->saveIgnoreFile();
}

/**
 * @brief 加载插件
 */
void DAPluginManager::loadAllPlugins(DACoreInterface* c)
{
    if (d_ptr->mIsLoaded) {
        daWarning << tr("Plugins have already been loaded, skipping duplicate load.");  // cn:插件已加载，跳过重复加载
        return;
    }

    if (!d_ptr->mPluginDir.exists()) {
        daWarning << tr("Plugin directory does not exist: %1").arg(d_ptr->mPluginDir.absolutePath());  // cn:插件目录不存在：%1
        d_ptr->mIsLoaded = true;
        return;
    }

    const QFileInfoList fileInfos = d_ptr->mPluginDir.entryInfoList(QDir::Files);
    if (fileInfos.isEmpty()) {
        daInfo << tr("No plugin files found in: %1").arg(d_ptr->mPluginDir.absolutePath());  // cn:插件目录中未找到插件文件：%1
    }

    daInfo.noquote() << tr("plugin directory is: %1").arg(d_ptr->mPluginDir.absolutePath());  // cn:插件目录为：%1
    for (const QFileInfo& fi : fileInfos) {
        if (fi.baseName().isEmpty()) {
            //".开头的文件，如.pluginignore"
            continue;
        }

        // 跨平台的动态库后缀检查
        if (!isPluginLibrarySuffix(fi.suffix())) {
            continue;
        }
        if (d_ptr->mIgnorePluginBaseName.contains(fi.baseName().toLower())) {
            daInfo.noquote() << tr("ignoring plugin %1").arg(fi.baseName());  // cn:忽略插件 %1
            continue;
        }
        const QString filepath = fi.absoluteFilePath();
        if (!QLibrary::isLibrary(filepath)) {
            daWarning.noquote() << tr("ignoring invalid file: %1").arg(fi.absoluteFilePath());  // cn:忽略无效文件：%1
            continue;
        }
        DAPluginOption pluginopt;
        Q_EMIT beginLoadPlugin(fi.absoluteFilePath());
        if (!pluginopt.load(fi.absoluteFilePath(), c)) {
            daWarning << tr("cannot load plugin: %1").arg(fi.absoluteFilePath());  // cn:无法加载插件：%1
            d_ptr->mFailedPlugins.insert(fi.baseName().toLower(), pluginopt.getErrorString());
            continue;
        }
        d_ptr->mFailedPlugins.remove(fi.baseName().toLower());
        d_ptr->mPluginOptions.append(pluginopt);
    }
    d_ptr->mIsLoaded = true;
}

/**
 * @brief 运行期加载单个插件（热加载）
 *
 * 与 loadAllPlugins 不同，此函数不受 mIsLoaded 标记限制，可在程序运行中随时调用。
 * 若插件文件命中忽略列表、已加载或加载失败，将拒绝加载并返回false
 * @param pluginFilePath 插件文件绝对路径
 * @param c 核心接口
 * @return 是否加载成功
 */
bool DAPluginManager::loadPlugin(const QString& pluginFilePath, DACoreInterface* c)
{
    QFileInfo fi(pluginFilePath);
    if (!fi.exists() || fi.baseName().isEmpty() || !isPluginLibrarySuffix(fi.suffix())) {
        daWarning << tr("%1 is not a valid plugin library file").arg(pluginFilePath);  // cn:%1不是有效的插件库文件
        return false;
    }
    const QString key = fi.baseName().toLower();
    if (d_ptr->findOptionByFilePath(fi.absoluteFilePath())) {
        daWarning << tr("plugin %1 is already loaded").arg(fi.baseName());  // cn:插件%1已加载
        return false;
    }
    if (d_ptr->mIgnorePluginBaseName.contains(key)) {
        daWarning << tr("plugin %1 is disabled, remove it from the ignore list first").arg(fi.baseName());  // cn:插件%1已被禁用，请先将其移出忽略列表
        return false;
    }
    if (!QLibrary::isLibrary(fi.absoluteFilePath())) {
        daWarning << tr("ignoring invalid file: %1").arg(fi.absoluteFilePath());  // cn:忽略无效文件：%1
        return false;
    }
    DAPluginOption pluginopt;
    Q_EMIT beginLoadPlugin(fi.absoluteFilePath());
    if (!pluginopt.load(fi.absoluteFilePath(), c)) {
        daWarning << tr("cannot load plugin: %1").arg(fi.absoluteFilePath());  // cn:无法加载插件：%1
        d_ptr->mFailedPlugins.insert(key, pluginopt.getErrorString());
        return false;
    }
    d_ptr->mFailedPlugins.remove(key);
    d_ptr->mDegradedPlugins.remove(key);
    d_ptr->mPluginOptions.append(pluginopt);
    daInfo << tr("plugin %1 loaded at runtime").arg(fi.baseName());  // cn:插件%1已在运行期加载
    Q_EMIT pluginLoaded(fi.absoluteFilePath());
    return true;
}

bool DAPluginManager::isLoaded() const
{
    return (d_ptr->mIsLoaded);
}

/**
 * @brief
 * 设置插件路径，可以多次load，同一个插件（插件名称和类型组成一个key）只会加载一次
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

    for (const DAPluginOption& opt : std::as_const(d_ptr->mPluginOptions)) {
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

/**
 * @brief 扫描插件目录，返回全部插件文件及其状态
 *
 * 合并已加载插件的运行时信息（名称/版本/描述）与目录中未加载的文件
 * （禁用项、加载失败项、降级停用项、新发现项），供插件管理界面展示
 * @return 插件文件信息列表
 */
QList< DAPluginFileInfo > DAPluginManager::scanPluginFiles() const
{
    QList< DAPluginFileInfo > res;
    if (!d_ptr->mPluginDir.exists()) {
        return res;
    }
    const QFileInfoList fileInfos = d_ptr->mPluginDir.entryInfoList(QDir::Files);
    for (const QFileInfo& fi : fileInfos) {
        if (fi.baseName().isEmpty()) {
            continue;
        }
        if (!isPluginLibrarySuffix(fi.suffix())) {
            continue;
        }
        DAPluginFileInfo info;
        info.baseName = fi.baseName();
        info.filePath = fi.absoluteFilePath();
        info.name     = fi.baseName();
        const QString key = fi.baseName().toLower();
        const DAPluginOption* opt = d_ptr->findOptionByFilePath(fi.absoluteFilePath());
        if (opt && opt->isValid()) {
            info.state       = DAPluginFileInfo::Loaded;
            info.name        = opt->getPluginName();
            info.version     = opt->getPluginVersion();
            info.description = opt->getPluginDescription();
        } else if (d_ptr->mDegradedPlugins.contains(key)) {
            // 降级停用优先于禁用判断：降级插件同时也在忽略列表中，但需展示"重启后释放库"状态
            info.state = DAPluginFileInfo::InactivePendingRestart;
        } else if (d_ptr->mIgnorePluginBaseName.contains(key)) {
            info.state = DAPluginFileInfo::Disabled;
        } else if (d_ptr->mFailedPlugins.contains(key)) {
            info.state       = DAPluginFileInfo::LoadFailed;
            info.errorString = d_ptr->mFailedPlugins.value(key);
        } else {
            info.state = DAPluginFileInfo::NotLoaded;
        }
        res.append(info);
    }
    return res;
}

/**
 * @brief 卸载指定插件（热卸载）
 *
 * 流程：先调用插件finalize()让其清理自建资源（返回false则取消卸载），
 * 再销毁插件实例并尝试释放动态库。调用方必须在调用本函数之前完成宿主侧
 * 注册表的注销（如agent工具），否则插件实例销毁后会留下悬空指针。
 * 若库释放失败（Windows下库被其他模块引用时常见），插件实例已被销毁、
 * 功能已停止，转入降级停用状态，重启后完全释放
 * @param pluginName 插件名（DAAbstractPlugin::getName）
 * @return 卸载结果
 */
DAPluginManager::UnloadResult DAPluginManager::unloadPlugin(const QString& pluginName)
{
    for (auto it = d_ptr->mPluginOptions.begin(); it != d_ptr->mPluginOptions.end(); ++it) {
        if (it->getPluginName() == pluginName) {
            // 1. 调用插件的 finalize() 进行清理
            DAAbstractPlugin* plugin = it->plugin();
            if (plugin && !plugin->finalize()) {
                daWarning << tr("Plugin %1 refused to finalize, unload cancelled.").arg(pluginName);  // cn:插件 %1 拒绝完成清理，卸载已取消
                return UnloadRefused;
            }
            // 2. 卸载插件库（无论库是否释放成功，插件实例都已被销毁，必须从列表移除）
            const QString baseKey = it->getBaseName().toLower();
            const bool fullUnload = it->unload();
            d_ptr->mPluginOptions.erase(it);
            if (fullUnload) {
                d_ptr->mDegradedPlugins.remove(baseKey);
                Q_EMIT pluginUnloaded(pluginName, true);
                return UnloadSucceed;
            }
            // 库释放失败，转入降级停用状态
            d_ptr->mDegradedPlugins.insert(baseKey);
            daWarning << tr("Plugin %1 has been finalized and deactivated, but its library could not be released; "
                            "it will be fully released after restart")
                             .arg(pluginName);  // cn:插件%1已清理并停用，但其库无法释放，将在重启后完全释放
            Q_EMIT pluginUnloaded(pluginName, false);
            return UnloadDegraded;
        }
    }
    daWarning << tr("Plugin %1 not found for unloading.").arg(pluginName);  // cn:未找到要卸载的插件 %1
    return UnloadNotFound;
}

bool DAPluginManager::unloadAllPlugins()
{
    bool allSuccess = true;
    // 使用 takeLast 逐个取出并处理，避免二次查找，同时确保列表被清空
    while (!d_ptr->mPluginOptions.isEmpty()) {
        DAPluginOption opt = d_ptr->mPluginOptions.takeLast();
        DAAbstractPlugin* plugin = opt.plugin();
        QString name = opt.getPluginName();
        // 1. 调用插件的 finalize() 进行清理
        if (plugin && !plugin->finalize()) {
            daWarning << tr("Plugin %1 refused to finalize, skip unload.").arg(name);  // cn:插件 %1 拒绝清理，跳过卸载
            allSuccess = false;
            continue;
        }
        // 2. 卸载插件库
        if (opt.unload()) {
            Q_EMIT pluginUnloaded(name, true);
        } else {
            daWarning << tr("Failed to unload plugin library for %1.").arg(name);  // cn:无法卸载插件 %1 的库
            allSuccess = false;
            Q_EMIT pluginUnloaded(name, false);
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

/**
 * @brief 判断文件后缀是否为当前平台的动态库后缀
 * @param suffix 文件后缀（不区分大小写）
 * @return
 */
bool DAPluginManager::isPluginLibrarySuffix(const QString& suffix)
{
    const QString s = suffix.toLower();
#ifdef Q_OS_WIN
    return (s == QLatin1String("dll"));
#elif defined(Q_OS_MACOS)
    return (s == QLatin1String("dylib")) || (s == QLatin1String("so"));  // 有时macOS也用.so
#else  // Unix/Linux
    return (s == QLatin1String("so"));
#endif
}

QDebug operator<<(QDebug debug, const DAPluginManager& fmg)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << DAPluginManager::tr("Plugin Manager Info: is loaded=%1, plugin counts=%2")
                           .arg(fmg.isLoaded())
                           .arg(fmg.getPluginCount())  // cn:插件管理器信息：已加载=%1，插件数量=%2
                    << Qt::endl;
    QList< DAPluginOption > opts = fmg.getPluginOptions();

    for (const DAPluginOption& opt : std::as_const(opts)) {
        debug.nospace() << opt;
    }
    return (debug);
}
}  // namespace DA
