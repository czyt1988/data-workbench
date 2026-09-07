#include "DAAppPluginManager.h"
#include "DAAbstractNodePlugin.h"
#include "DAAbstractPlugin.h"
#include "DAPluginManager.h"
#include "DAPluginOption.h"
#include "DAPyNodeFactory.h"
#include "DALogCategory.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "DAPyInterpreter.h"
#include "DAPyNode.h"
#include "DAPyWorkFlowManager.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DAPyWorkFlowOperateWidget.h"
#include "DAPyWorkFlowEditWidget.h"
#include "DAAgentInterface.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

// pyplugins的忽略文件路径
static QString getPyPluginIgnoreFilePath()
{
    return QDir::toNativeSeparators(DAAppPluginManager::getPyPluginDirPath() + QDir::separator()
                                    + DAPluginManager::getPluginIgnoreFileName());
}

//===================================================
// DAWorkFlowPluginManager
//===================================================
DAAppPluginManager::DAAppPluginManager(QObject* p) : DAPluginManager(p)
{
}

DAAppPluginManager::~DAAppPluginManager()
{
}

/**
 * @brief 初始化加载所有的插件
 *
 * 加载C++插件节点后，如果Python环境已启用，还会初始化Python节点工厂，
 * 将Python节点元数据合并到节点列表中，实现Python和C++节点共存。
 *
 * @param[in] c 核心接口
 */
void DAAppPluginManager::loadAllPlugins(DACoreInterface* c)
{
    // 加载插件
    if (!isLoaded()) {
        DAPluginManager::loadAllPlugins(c);
    }
    mCore = c;
    // 初始化Python节点工厂
    initPyNodeFactory();
    // 建立插件列表与节点元数据（C++插件 + Python工厂合并去重）
    refreshAfterPluginChange();
}

/**
 * @brief 获取所有的插件
 * @return
 */
QList< DAAbstractPlugin* > DAAppPluginManager::getAllPlugins() const
{
    return mPlugins;
}

/**
 * @brief 获取所有的节点插件
 * @return
 */
QList< DAAbstractNodePlugin* > DAAppPluginManager::getNodePlugins() const
{
    QList< DAAbstractNodePlugin* > res;
    for (DAAbstractPlugin* p : std::as_const(mPlugins)) {
        if (DAAbstractNodePlugin* np = dynamic_cast< DAAbstractNodePlugin* >(p)) {
            res.append(np);
        }
    }
    return res;
}

/**
 * @brief 获取所有的节点工厂
 *
 * 返回C++插件节点工厂和Python节点工厂的合并列表
 *
 * @return 所有节点工厂的共享指针列表
 */
QList< std::shared_ptr< DAPyNodeFactory > > DAAppPluginManager::createNodeFactorys() const
{
    QList< std::shared_ptr< DAPyNodeFactory > > res;
    const QList< DAAbstractNodePlugin* > nodePlugins = getNodePlugins();
    for (DAAbstractNodePlugin* d : std::as_const(nodePlugins)) {
        res.append(std::shared_ptr< DAPyNodeFactory >(d->createNodeFactory()));
    }
    if (mPyNodeFactory) {
        res.append(mPyNodeFactory);
    }
    return (res);
}

/**
 * @brief 获取所有的元数据
 * @return
 */
QList< DAPyNodeMetaData > DAAppPluginManager::getAllNodeMetaDatas() const
{
    return mNodeMetaDatas;
}

/**
 * @brief 获取Python节点工厂
 *
 * 返回Python节点工厂的共享指针，如果Python未启用或初始化失败则返回nullptr。
 * 此工厂可用于创建DAPyNode实例并注入到DAPyWorkFlowScene中。
 *
 * @return Python节点工厂共享指针，未初始化时返回nullptr
 */
std::shared_ptr< DAPyNodeFactory > DAAppPluginManager::getPyNodeFactory() const
{
    return mPyNodeFactory;
}

/**
 * @brief 运行期启用C++插件（热加载）并持久化启用状态
 *
 * 流程：定位插件文件 → 移出忽略列表并回写.pluginignore → 加载并initialize →
 * 重建插件/节点元数据列表 → 节点插件补调afterLoadedNodes → 发射nodeMetaDatasChanged。
 * 加载失败时启用状态保留（体现用户意图），状态栏显示加载失败，可重启后重试
 * @param pluginBaseName 插件文件基本名（不含后缀）
 * @param errorString 失败原因输出，可为nullptr
 * @return 是否启用成功
 */
bool DAAppPluginManager::enablePlugin(const QString& pluginBaseName, QString* errorString)
{
    auto setErr = [ errorString, &pluginBaseName ](const QString& e) {
        if (errorString) {
            *errorString = e;
        }
        qWarning() << "DAAppPluginManager::enablePlugin failed for plugin:" << pluginBaseName;
    };
    if (nullptr == mCore) {
        setErr(tr("Plugin manager is not initialized"));  //cn:插件管理器未初始化
        return false;
    }
    if (findPluginOptionIndexByBaseName(pluginBaseName) >= 0) {
        setErr(tr("Plugin %1 is already loaded").arg(pluginBaseName));  //cn:插件%1已加载
        return false;
    }
    // 定位插件文件
    QString filePath;
    QDir dir(getPluginDirPath());
    const QFileInfoList fileInfos = dir.entryInfoList(QDir::Files);
    for (const QFileInfo& fi : fileInfos) {
        if (fi.baseName().compare(pluginBaseName, Qt::CaseInsensitive) == 0 && isPluginLibrarySuffix(fi.suffix())) {
            filePath = fi.absoluteFilePath();
            break;
        }
    }
    if (filePath.isEmpty()) {
        setErr(tr("Plugin file %1 is not found in the plugin directory").arg(pluginBaseName));  //cn:插件目录中未找到插件文件%1
        return false;
    }
    // 先移出忽略列表（loadPlugin会拒绝命中忽略列表的文件）
    setPluginEnabled(pluginBaseName, true);
    if (!loadPlugin(filePath, mCore)) {
        setErr(tr("Failed to load plugin %1").arg(pluginBaseName));  //cn:加载插件%1失败
        return false;
    }
    refreshAfterPluginChange();
    // 先刷新节点工具箱（信号为直连，同步重建），再补调afterLoadedNodes，与启动时序一致
    // （启动路径由AppMainWindow::initWorkflowNodes在toolbox填充后统一调用）
    Q_EMIT nodeMetaDatasChanged();
    const int idx = findPluginOptionIndexByBaseName(pluginBaseName);
    if (idx >= 0) {
        const QList< DAPluginOption > opts = getPluginOptions();
        if (DAAbstractPlugin* p = opts[ idx ].plugin()) {
            if (DAAbstractNodePlugin* np = dynamic_cast< DAAbstractNodePlugin* >(p)) {
                np->afterLoadedNodes();
            }
        }
    }
    return true;
}

/**
 * @brief 运行期禁用C++插件（热卸载）并持久化禁用状态
 *
 * 流程：在用节点守卫 → 注销宿主侧agent注册表（工具/系统提示词，必须先于插件实例销毁）→
 * finalize并卸载 → 回写.pluginignore → 重建插件/节点元数据列表 → 发射nodeMetaDatasChanged。
 * 库释放失败时插件已finalize并移出加载列表（降级停用，重启后完全释放），仍视为禁用成功
 * @param pluginBaseName 插件文件基本名（不含后缀）
 * @param errorString 失败原因输出，可为nullptr
 * @return 是否禁用成功
 */
bool DAAppPluginManager::disablePlugin(const QString& pluginBaseName, QString* errorString)
{
    auto setErr = [ errorString, &pluginBaseName ](const QString& e) {
        if (errorString) {
            *errorString = e;
        }
        qWarning() << "DAAppPluginManager::disablePlugin failed for plugin:" << pluginBaseName;
    };
    const int idx = findPluginOptionIndexByBaseName(pluginBaseName);
    if (idx < 0) {
        setErr(tr("Plugin %1 is not loaded").arg(pluginBaseName));  //cn:插件%1未加载
        return false;
    }
    const QList< DAPluginOption > opts = getPluginOptions();
    DAAbstractPlugin* plugin           = opts[ idx ].plugin();
    if (nullptr == plugin) {
        setErr(tr("Plugin %1 is not loaded").arg(pluginBaseName));  //cn:插件%1未加载
        return false;
    }
    // 守卫：插件节点正被打开的工作流使用时拒绝卸载
    if (DAAbstractNodePlugin* np = dynamic_cast< DAAbstractNodePlugin* >(plugin)) {
        const QStringList inUse = pluginNodesInUse(np);
        if (!inUse.isEmpty()) {
            setErr(tr("Nodes of plugin %1 are in use by open workflows (%2), please close them first")
                       .arg(plugin->getName(), inUse.join(QStringLiteral(", "))));  //cn:插件%1的节点正被打开的工作流使用（%2），请先关闭这些工作流
            return false;
        }
    }
    // 注销宿主侧agent注册表，必须先于插件实例销毁，否则留下悬空指针。
    // DAAbstractPlugin不是QObject，插件类多继承QObject+DAAbstractPlugin，
    // 需dynamic_cast横转到QObject面（与注册时记录的provider一致）
    if (mCore) {
        if (DAAgentInterface* agent = mCore->getAgentInterface()) {
            if (QObject* pluginObj = dynamic_cast< QObject* >(plugin)) {
                agent->unregisterToolsByProvider(pluginObj);
                agent->unregisterSystemPromptsByProvider(pluginObj);
            } else {
                qWarning() << "DAAppPluginManager::disablePlugin: plugin is not a QObject, cannot unregister agent resources by provider:"
                           << pluginBaseName;
            }
        }
    }
    // 注意：unloadPlugin内部会销毁插件实例，pluginName必须先取出
    const QString pluginName = plugin->getName();
    const UnloadResult res   = DAPluginManager::unloadPlugin(pluginName);
    switch (res) {
    case UnloadRefused:
        setErr(tr("Plugin %1 refused to finalize, disabling cancelled").arg(pluginName));  //cn:插件%1拒绝清理，禁用已取消
        return false;
    case UnloadNotFound:
        setErr(tr("Plugin %1 is not loaded").arg(pluginBaseName));  //cn:插件%1未加载
        return false;
    case UnloadSucceed:
    case UnloadDegraded:
        break;
    }
    // 持久化禁用状态
    setPluginEnabled(pluginBaseName, false);
    refreshAfterPluginChange();
    Q_EMIT nodeMetaDatasChanged();
    return true;
}

/**
 * @brief 获取pyplugins目录下全部Python节点包信息（含禁用项）
 * @return Python节点包信息列表
 */
QList< DAPyPluginPackageInfo > DAAppPluginManager::getPyPluginPackageInfos() const
{
    QList< DAPyPluginPackageInfo > res;
    QDir dir(getPyPluginDirPath());
    if (!dir.exists()) {
        return res;
    }
    const QSet< QString > ignoreSet = readPyPluginIgnoreSet();
    const QStringList subDirs       = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subDirName : subDirs) {
        const QString pkgPath = QDir::toNativeSeparators(dir.absoluteFilePath(subDirName));
        if (!QFile::exists(QDir(pkgPath).filePath("__init__.py"))) {
            continue;
        }
        DAPyPluginPackageInfo info;
        info.packageName = subDirName;
        info.dirPath     = pkgPath;
        info.enabled     = !ignoreSet.contains(subDirName.toLower());
        res.append(info);
    }
    return res;
}

/**
 * @brief 设置Python节点包启用状态
 *
 * 持久化到pyplugins/.pluginignore，Python包无法在运行期安全卸载，下次启动生效
 * @param packageName 包目录名
 * @param enable 是否启用
 * @return 是否成功写入配置
 */
bool DAAppPluginManager::setPyPluginEnabled(const QString& packageName, bool enable)
{
    const QString key = packageName.trimmed().toLower();
    if (key.isEmpty()) {
        return false;
    }
    QSet< QString > ignoreSet = readPyPluginIgnoreSet();
    if (enable) {
        ignoreSet.remove(key);
    } else {
        ignoreSet.insert(key);
    }
    return writePyPluginIgnoreSet(ignoreSet);
}

/**
 * @brief 获取pyplugins目录绝对路径
 * @return
 */
QString DAAppPluginManager::getPyPluginDirPath()
{
    return QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/pyplugins");
}

/**
 * @brief 扫描pyplugins目录，收集有效的Python插件路径
 *
 * 遍历pyplugins目录下的一级子目录，检查每个子目录是否包含__init__.py。
 * 满足条件的包路径将被收集并返回。
 *
 * @param pyPluginsDir pyplugins目录的绝对路径
 * @return 有效的Python插件目录路径列表
 */
static QStringList scanPyPluginsDir(const QString& pyPluginsDir)
{
    QStringList result;
    QDir dir(pyPluginsDir);
    if (!dir.exists()) {
        qDebug() << "pyplugins dir not found:" << pyPluginsDir << ", skip Python plugin scan";
        return result;
    }

    const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subDirName : subDirs) {
        QString pyScriptsPath = QDir::toNativeSeparators(pyPluginsDir + "/" + subDirName);
        QDir pyScriptsDir(pyScriptsPath);
        QString initFilePath = pyScriptsDir.filePath("__init__.py");
        bool hasPyPackage    = QFile::exists(initFilePath);

        if (hasPyPackage) {
            result.append(pyScriptsPath);
        } else {
            qDebug() << "pyplugins/" << subDirName << "/PyScripts has no valid Python package, skip";
        }
    }

    return result;
}

/**
 * @brief 初始化Python节点工厂
 *
 * 创建DAPyNodeFactory实例，调用discoverNodes()发现Python节点。
 * 元数据合并统一在refreshAfterPluginChange中处理。
 *
 * 发现流程：
 * 1. 检查Python解释器是否已初始化
 * 2. 将PyScripts目录添加到Python sys.path
 * 3. 扫描pyplugins目录并按pyplugins/.pluginignore过滤禁用包
 * 4. 创建DAPyNodeFactory并调用discoverNodes()
 *
 * @note 如果Python未初始化或发现失败，不会影响C++节点的正常加载
 */
void DAAppPluginManager::initPyNodeFactory()
{
    if (!DAPyInterpreter::isPythonInitialized()) {
        daWarning << tr("Python interpreter not initialized, skip Python node discovery");  // cn:Python解释器未初始化，跳过Python节点发现
        return;
    }
    try {
        // 将内置PyScripts目录添加到sys.path，确保DAWorkbench.DAWorkFlowPy可被导入

        DAPyInterpreter::appendSysPath(QDir::toNativeSeparators(QApplication::applicationDirPath() + "/PyScripts"));

        // 扫描pyplugins目录，收集有效的Python插件路径，并过滤禁用包
        QString pyPluginsDir  = getPyPluginDirPath();
        QStringList scanPaths = scanPyPluginsDir(pyPluginsDir);
        const QSet< QString > ignoreSet = readPyPluginIgnoreSet();
        if (!ignoreSet.isEmpty()) {
            QStringList enabledPaths;
            for (const QString& p : std::as_const(scanPaths)) {
                const QString pkgName = QDir(p).dirName();
                if (ignoreSet.contains(pkgName.toLower())) {
                    qDebug() << "pyplugin package disabled by .pluginignore, skip:" << pkgName;
                    continue;
                }
                enabledPaths.append(p);
            }
            scanPaths = enabledPaths;
        }
        qDebug() << QString("Pyplugins scan completed, found %1 valid Python plugin paths").arg(scanPaths.size());  // cn:Python插件扫描完成，发现%1个有效的Python插件路径

        // 创建Python节点工厂，传入扫描路径并启用entry_points模式
        mPyNodeFactory = std::make_shared< DAPyNodeFactory >();
        if (!mPyNodeFactory->discoverNodes(scanPaths, true)) {
            daWarning << tr("Python node discovery failed");  // cn:Python节点发现失败
            mPyNodeFactory.reset();
            return;
        }
        qDebug() << QString("Python node factory initialized, discovered %1 nodes").arg(mPyNodeFactory->getNodeMetadataList().size());  // cn:Python节点工厂初始化完成，发现%1个节点
    } catch (const std::exception& e) {
        daCritical << tr("Python node factory initialization failed: %1").arg(e.what());  // cn:Python节点工厂初始化失败:%1
        mPyNodeFactory.reset();
    }
}

/**
 * @brief 按当前已加载插件重建mPlugins/mNodeMetaDatas
 *
 * C++节点插件的元数据经临时工厂获取，Python工厂元数据一并合并，
 * 最后按QMap计数去重且保持原有顺序。插件热插拔后调用
 */
void DAAppPluginManager::refreshAfterPluginChange()
{
    mPlugins.clear();
    mNodeMetaDatas.clear();
    const QList< DAPluginOption > plugins = getPluginOptions();
    for (const DAPluginOption& opt : plugins) {
        if (!opt.isValid()) {
            continue;
        }
        DAAbstractPlugin* p = opt.plugin();
        if (nullptr == p) {
            continue;
        }
        mPlugins.append(p);
        // 开始通过dynamic_cast判断插件的具体类型
        if (DAAbstractNodePlugin* np = dynamic_cast< DAAbstractNodePlugin* >(p)) {
            // 说明是节点插件
            // 这里工厂仅仅为了获取节点的meta数据
            std::unique_ptr< DAPyNodeFactory > fac(np->createNodeFactory());
            if (nullptr == fac) {
                // 创建工厂失败,是没有工作流的界面
                continue;
            }
            // 此操作是为了获取所有节点metadata
            mNodeMetaDatas += fac->getNodeMetadataList();
            qDebug() << QString("Collected node metadata from plugin %1").arg(np->getName());  // cn:已收集插件%1的节点元数据
        }
    }
    // 合并Python节点工厂元数据
    if (mPyNodeFactory) {
        mNodeMetaDatas += mPyNodeFactory->getNodeMetadataList();
    }
    // 最后对mNodeMetaDatas去重，此去重要保证原来的顺序
    QMap< DAPyNodeMetaData, int > mapcnt;
    // 说明有重复项，需要去除
    for (auto i = mNodeMetaDatas.begin(); i != mNodeMetaDatas.end();) {
        if (!mapcnt.contains(*i)) {
            mapcnt.insert(*i, 1);
            ++i;
        } else {
            // 说明找到了重复项目
            i = mNodeMetaDatas.erase(i);
        }
    }
}

/**
 * @brief 检查节点插件的节点是否被打开的工作流使用
 *
 * 经core→uiInterface→dockingArea→workFlowOperateWidget链枚举全部打开的工作流，
 * 比对各工作流管理器中节点的qualifiedName与插件工厂提供的节点元数据
 * @param np 节点插件
 * @return 在用的节点qualifiedName列表，空列表表示无使用
 */
QStringList DAAppPluginManager::pluginNodesInUse(DAAbstractNodePlugin* np) const
{
    QStringList inUse;
    if (nullptr == np || nullptr == mCore) {
        return inUse;
    }
    // 收集插件提供的节点qualifiedName集合
    QSet< QString > pluginNodeNames;
    {
        std::unique_ptr< DAPyNodeFactory > fac(np->createNodeFactory());
        if (nullptr == fac) {
            return inUse;
        }
        const QList< DAPyNodeMetaData > mds = fac->getNodeMetadataList();
        for (const DAPyNodeMetaData& md : mds) {
            if (!md.qualifiedName.isEmpty()) {
                pluginNodeNames.insert(md.qualifiedName);
            }
        }
    }
    if (pluginNodeNames.isEmpty()) {
        return inUse;
    }
    DAUIInterface* ui = mCore->getUiInterface();
    if (nullptr == ui) {
        return inUse;
    }
    DADockingAreaInterface* dockArea = ui->getDockingArea();
    if (nullptr == dockArea) {
        return inUse;
    }
    DAPyWorkFlowOperateWidget* wfOperate = dockArea->getWorkFlowOperateWidget();
    if (nullptr == wfOperate) {
        return inUse;
    }
    const QList< DAPyWorkFlowEditWidget* > wfWidgets = wfOperate->getAllWorkFlowWidgets();
    for (DAPyWorkFlowEditWidget* w : wfWidgets) {
        if (nullptr == w) {
            continue;
        }
        DAPyWorkFlowManager* mgr = w->getManager();
        if (nullptr == mgr) {
            continue;
        }
        const QList< DAPyNode > nodes = mgr->workflowNodes();
        for (const DAPyNode& n : nodes) {
            const QString qn = n.getQualifiedName();
            if (pluginNodeNames.contains(qn) && !inUse.contains(qn)) {
                inUse.append(qn);
            }
        }
    }
    return inUse;
}

/**
 * @brief 按文件基本名查找已加载的插件选项下标
 * @param pluginBaseName 插件文件基本名（不含后缀，不区分大小写）
 * @return 下标，未找到返回-1
 */
int DAAppPluginManager::findPluginOptionIndexByBaseName(const QString& pluginBaseName) const
{
    const QList< DAPluginOption > opts = getPluginOptions();
    for (int i = 0; i < opts.size(); ++i) {
        if (opts[ i ].getBaseName().compare(pluginBaseName, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 读取pyplugins的忽略列表
 * @return 小写包名集合，文件不存在时返回空集合
 */
QSet< QString > DAAppPluginManager::readPyPluginIgnoreSet()
{
    QSet< QString > res;
    QFile f(getPyPluginIgnoreFilePath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return res;
    }
    QTextStream ss(&f);
    while (!ss.atEnd()) {
        QString line = ss.readLine().trimmed();
        if (line.isEmpty() || line.at(0) == QLatin1Char('#')) {
            continue;
        }
        res.insert(line.toLower());
    }
    return res;
}

/**
 * @brief 将包名集合写入pyplugins/.pluginignore
 *
 * 保留原文件中的注释行，重写禁用列表；文件不存在时生成带说明注释的新文件
 * @param ignoreSet 小写包名集合
 * @return 是否写入成功
 */
bool DAAppPluginManager::writePyPluginIgnoreSet(const QSet< QString >& ignoreSet)
{
    const QString filePath = getPyPluginIgnoreFilePath();
    QDir dir(getPyPluginDirPath());
    if (!dir.exists() && !dir.mkpath(".")) {
        qWarning() << "DAAppPluginManager::writePyPluginIgnoreSet: failed to create pyplugins dir:" << dir.absolutePath();
        return false;
    }
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
    } else {
        commentLines << u8"# python plugin ignore file, packages listed here will not be loaded on next startup"
                     << u8"# 不想加载的Python节点包在此文件描述，写入包目录名，下次启动生效";
    }
    QFile writeFile(filePath);
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAppPluginManager::writePyPluginIgnoreSet: failed to write" << filePath << ":" << writeFile.errorString();
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
    QStringList names = ignoreSet.values();
    names.sort(Qt::CaseInsensitive);
    for (const QString& name : std::as_const(names)) {
        txt << name << Qt::endl;
    }
    writeFile.close();
    return true;
}

}
