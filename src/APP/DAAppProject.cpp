#include "DAAppProject.h"
#include <memory>
// Qt
#include <QBuffer>
#include <QDomDocument>
#include <QFile>
#include <QScopedPointer>
#include <QVariant>
#include <QPen>
#include <QElapsedTimer>
#include <QSet>
#include <QSysInfo>
#include <QFileDialog>
#include <QEventLoop>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QDir>
// DA
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DAStatusBarInterface.h"

#include "DAPyWorkFlowOperateWidget.h"
#include "DAXmlHelper.h"
#include "DAQtContainerUtil.hpp"
#include "DAStringUtil.h"
#include "DADataManagerInterface.h"
#include "DAAgentInterface.h"  // plan-05：经 DAAgentInterface 多态收集/导入会话（不依赖 DAAgentModule）
#include "DAAbstractArchiveTask.h"
#include "DAZipArchive.h"
#include "DAZipArchiveThreadWrapper.h"
#include "DADockingAreaInterface.h"
#include "DAZipArchiveTask_ByteArray.h"
#include "DAZipArchiveTask_Xml.h"
#include "DAZipArchiveTask_ArchiveFile.h"
#include "DAZipArchiveTask_ChartItem.h"
#include "DADataOperateWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include "DATableStyleManager.h"
#include "DATableStyleRegistry.h"
#include "DADataEnumStringUtils.h"
#include "DAWaitCursorScoped.h"
#include "Chart/DAChartItemsManager.h"
#include "Chart/DAChartOperateWidget.h"
#include "DAAppPluginManager.h"
#include "DALogCategory.h"
// python
#include "DAPyInterpreter.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPyWorkFlowSerializer.h"
#include "DAPyWorkFlowManager.h"
#include "DAPyWorkFlow.h"
#include "DAPyWorkFlowEditWidget.h"
#include "DAPyNodeFactory.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPybind11InQt.h"
#include "DAPyGILGuard.h"
#include "DAPybind11QtCaster.hpp"
#include "DAZipArchiveTask_ByteArray.h"
const QString c_workflowxml_save_filename  = QStringLiteral("workflow.xml");
const QString c_workflowdata_save_filename = QStringLiteral("workflow-data.xml");
const QString c_chartsxml_save_filename    = QStringLiteral("charts.xml");
const QString c_chartitem_save_folder      = QStringLiteral("chart-data");
const QString c_tablestylesxml_save_filename = QStringLiteral("table-styles.xml");
const QString c_dataoperatexml_save_filename = QStringLiteral("data-operate.xml");

#ifndef DAAPPPROJECT_TASK_LOAD_ID_BEGIN
#define DAAPPPROJECT_TASK_LOAD_ID_BEGIN 0x234
#endif
/**
 *@def 加载任务id - 工作流的ui
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW
#define DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 1)
#endif

/**
 *@def 加载任务id - datamanager
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_DATAMANAGER
#define DAAPPPROJECT_TASK_LOAD_ID_DATAMANAGER (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 2)
#endif

/**
 *@def 加载任务id - chartItemManager
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_CHARTITEMMANAGER
#define DAAPPPROJECT_TASK_LOAD_ID_CHARTITEMMANAGER (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 3)
#endif

/**
 *@def 加载任务id - charts_xml
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_CHARTS_INFO
#define DAAPPPROJECT_TASK_LOAD_ID_CHARTS_INFO (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 4)
#endif

/**
 *@def 加载任务id - 工作流Python逻辑数据
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW_DATA
#define DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW_DATA (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 5)
#endif

/**
 *@def 加载任务id - 表格样式
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_TABLE_STYLES
#define DAAPPPROJECT_TASK_LOAD_ID_TABLE_STYLES (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 6)
#endif

/**
 *@def 加载任务id - Agent会话（plan-05：解压 agent_sessions/*.jsonl 后回调导入）
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_AGENT_SESSIONS
#define DAAPPPROJECT_TASK_LOAD_ID_AGENT_SESSIONS (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 7)
#endif

/**
 *@def 加载任务id - 数据操作窗口嵌套停靠区布局（在 datamanager 之后，重建已打开数据页 + 恢复 dock 布局）
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_DATA_OPERATE_LAYOUT
#define DAAPPPROJECT_TASK_LOAD_ID_DATA_OPERATE_LAYOUT (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 8)
#endif
namespace DA
{

struct DAArchiveRunResult
{
    bool started { false };
    bool success { false };
};

static DAArchiveRunResult waitArchiveSave(DAZipArchiveThreadWrapper* archive, const QString& filePath)
{
    DAArchiveRunResult res;
    if (nullptr == archive) {
        return res;
    }

    QEventLoop loop;
    QMetaObject::Connection c = QObject::connect(archive, &DAZipArchiveThreadWrapper::saved, &loop, [ & ](bool success) {
        res.success = success;
        loop.quit();
    });
    res.started = archive->save(filePath);
    if (res.started) {
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
    QObject::disconnect(c);
    return res;
}

static DAArchiveRunResult waitArchiveLoad(DAZipArchiveThreadWrapper* archive, const QString& filePath)
{
    DAArchiveRunResult res;
    if (nullptr == archive) {
        return res;
    }

    QEventLoop loop;
    QMetaObject::Connection c = QObject::connect(archive, &DAZipArchiveThreadWrapper::loaded, &loop, [ & ](bool success) {
        res.success = success;
        loop.quit();
    });
    res.started = archive->load(filePath);
    if (res.started) {
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
    QObject::disconnect(c);
    return res;
}

class DAZipArchiveTask_LoadDataManager : public DAAbstractArchiveTask
{
public:
    DAZipArchiveTask_LoadDataManager() : DAAbstractArchiveTask()
    {
    }
    ~DAZipArchiveTask_LoadDataManager()
    {
    }

public:
    /**
     * @brief 获取zip文件路径对应的本地临时文件的路径
     *
     * 此函数必须是执行完任务之后调用，否则没有内容
     * @param zipPath
     * @return
     */
    QString getLocalTempFilePath(const QString& zipPath) const
    {
        return mZipPathToTempFilePath.value(zipPath);
    }

    /**
     * @brief 获取datamanager的xml文档
     * @return
     */
    QDomDocument getDataManagerDomDocument() const
    {
        return mDataManagerDomDocument;
    }

    /**
     * @brief exec 注意此函数是在其它线程中执行
     * @param archive
     * @param mode
     * @return
     */
    virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override
    {
        if (!archive) {
            return false;
        }
        DAZipArchive* zip = static_cast< DAZipArchive* >(archive);
        if (mode != DAAbstractArchiveTask::ReadMode) {
            // 只支持读模式
            return false;
        }  // 读取数据模式
        if (!zip->isOpened()) {
            if (!zip->open()) {
                qDebug() << QString("open archive error:%1").arg(zip->getBaseFilePath());
                return false;
            }
        }
        // 首先读取data-manager.xml
        QByteArray dataMgrXmlByte = zip->read(QStringLiteral("data-manager.xml"));
        if (dataMgrXmlByte.isEmpty()) {
            qDebug() << QString("archive loss data-manager.xml file");
            return false;
        }
        // 读取完成后解析
        QString errorString;
        if (!mDataManagerDomDocument.setContent(dataMgrXmlByte, &errorString)) {
            qDebug() << QString("parse data-manager.xml file error:%1").arg(errorString);
            return false;
        }
        // 准备解压临时数据
        // 所有数据都在zip的datas目录下
        mZipPathToTempFilePath = extractDatasFolder(zip, QStringLiteral("datas"), mTempDir);
        return true;
    }

    QHash< QString, QString > extractDatasFolder(DAZipArchive* zip, const QString& zipFolderPath, const QTemporaryDir& tempDir)
    {
        QHash< QString, QString > res;
        // 获取压缩包内所有文件信息
        const QStringList allFiles = zip->getFolderFileNameList(zipFolderPath);
        for (const QString& zipfilePath : allFiles) {
            // 创建目标路径
            QString fileName = zipfilePath.mid(zipFolderPath.length() + 1);

            QString tempPath = tempDir.filePath(fileName);
            // 创建文件夹，如果zipFolderPath不是在顶层下面，就应该执行下面这2句
            //  QFileInfo tempFileInfo(tempPath);
            //  QDir().mkpath(tempFileInfo.absolutePath());
            if (zip->readToFile(zipfilePath, tempPath)) {
                res[ zipfilePath ] = tempPath;
            } else {
                qDebug() << QString("extract file %1 to %2 occur error").arg(zipfilePath, tempPath);
                continue;
            }
        }
        return res;
    }

private:
    QHash< QString, QString > mZipPathToTempFilePath;  ///< 记录zip的相对位置和解压的临时文件的相对位置的关系
    QDomDocument mDataManagerDomDocument;
    QTemporaryDir mTempDir;
};

/**
 * @brief 加载 agent_sessions/ 文件夹下所有 .jsonl 会话文件的任务（plan-05）
 *
 * 仿 DAZipArchiveTask_LoadDataManager，在 worker 线程内 exec()：
 * zip->getFolderFileNameList("agent_sessions") 列出 .jsonl → 对每个 zip->read(relPath)
 * 读字节 → 组装 QHash<QString,QByteArray>（id → bytes）存成员。提供 takeSessions()
 * 移交所有权，供主线程 setLoadedCallBack 回调内 loadSessionsFromProject 使用。
 * @note 此任务在子线程执行，不操作 UI；回调由 DAZipArchiveThreadWrapper::onTaskProgress
 * 在主线程触发（见 setLoadedCallBack 模式）。
 */
class DAZipArchiveTask_LoadAgentSessions : public DAAbstractArchiveTask
{
public:
    DAZipArchiveTask_LoadAgentSessions() : DAAbstractArchiveTask()
    {
    }
    ~DAZipArchiveTask_LoadAgentSessions()
    {
    }

public:
    /**
     * @brief exec 在 worker 线程执行：读取 agent_sessions/ 下所有 .jsonl 字节
     * @param archive 归档基类
     * @param mode 必须为 ReadMode
     * @return 读取成功返回 true；agent_sessions/ 不存在或为空也返回 true（空工程合法）
     */
    virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override
    {
        if (!archive) {
            return false;
        }
        DAZipArchive* zip = static_cast< DAZipArchive* >(archive);
        if (mode != DAAbstractArchiveTask::ReadMode) {
            return false;  // 只支持读模式
        }
        if (!zip->isOpened()) {
            if (!zip->open()) {
                qDebug() << QString("open archive error:%1").arg(zip->getBaseFilePath());
                return false;
            }
        }
        // 列出 agent_sessions/ 下所有文件（不含子目录）
        const QStringList allFiles = zip->getFolderFileNameList(QStringLiteral("agent_sessions"));
        for (const QString& zipPath : allFiles) {
            if (!zipPath.endsWith(QStringLiteral(".jsonl"), Qt::CaseInsensitive)) {
                continue;  // 跳过非会话文件
            }
            // 从 "agent_sessions/<id>.jsonl" 提取 <id>
            QString id = zipPath.mid(QStringLiteral("agent_sessions/").length());
            if (id.endsWith(QStringLiteral(".jsonl"), Qt::CaseInsensitive)) {
                id.chop(QStringLiteral(".jsonl").length());
            }
            if (id.isEmpty()) {
                continue;
            }
            QByteArray bytes = zip->read(zipPath);
            // 空字节也保留（空会话文件合法，导入侧 importSessionFiles 自行处理）
            mSessions[ id ] = bytes;
        }
        return true;
    }

    /**
     * @brief 移交会话字节所有权（id → jsonl bytes）
     * @note exec 完成后由主线程回调调用一次
     */
    QHash< QString, QByteArray > takeSessions()
    {
        return std::move(mSessions);
    }

private:
    QHash< QString, QByteArray > mSessions;  ///< agent_sessions/<id>.jsonl → 字节
};

////////////////////////////////////////////////////

DAAppProject::DAAppProject(DACoreInterface* c, QObject* p) : DAProjectInterface(c, p)
{
    // qRegisterMetaType< std::shared_ptr< DA::DAAbstractArchiveTask > >();
    mXml.setLoadedVersionNumber(DAProjectInterface::getProjectVersion());
    mArchive = new DAZipArchiveThreadWrapper(this);
    connect(mArchive, &DAZipArchiveThreadWrapper::beginSave, this, &DAAppProject::onBeginSave);
    connect(mArchive, &DAZipArchiveThreadWrapper::beginLoad, this, &DAAppProject::onBeginLoad);
    // 信号转发
    connect(mArchive, &DAZipArchiveThreadWrapper::beginSave, this, &DAProjectInterface::projectBeginSave);
    connect(mArchive, &DAZipArchiveThreadWrapper::beginLoad, this, &DAProjectInterface::projectBeginLoad);
    connect(mArchive, &DAZipArchiveThreadWrapper::taskProgress, this, &DAAppProject::onTaskProgress);
    connect(mArchive, &DAZipArchiveThreadWrapper::saved, this, &DAAppProject::onSaveFinish);
    connect(mArchive, &DAZipArchiveThreadWrapper::loaded, this, &DAAppProject::onLoadFinish);
}

DAAppProject::~DAAppProject()
{
}

DAPyWorkFlowOperateWidget* DAAppProject::getWorkFlowOperateWidget() const
{
    return getDockingAreaInterface()->getWorkFlowOperateWidget();
}

/**
 * @brief 数据操作窗口
 * @return
 */
DADataOperateWidget* DAAppProject::getDataOperateWidget() const
{
    return getDockingAreaInterface()->getDataOperateWidget();
}

/**
 * @brief 绘图窗口
 * @return
 */
DAChartOperateWidget* DAAppProject::getChartOperateWidget() const
{
    return getDockingAreaInterface()->getChartOperateWidget();
}

bool DAAppProject::appendWorkflowInProject(const QDomDocument& doc, bool skipIndex)
{
    // 加载之前先清空
    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    int oldProjectHaveWorkflow = wfo->count();  // 已有的工作流数量
    bool isok                  = true;
    QDomElement docElem        = doc.documentElement();                 // root
    QDomElement proEle         = docElem.firstChildElement("project");  // project
    // 获取版本
    QString verString = proEle.attribute("version");
    if (!verString.isEmpty()) {
        QVersionNumber version = QVersionNumber::fromString(verString);
        if (!version.isNull()) {
            // 针对工程版本的操作！！
        }
    }
    QDomElement workflowsEle  = proEle.firstChildElement("workflows");  // workflows
    QString workflowVerString = workflowsEle.attribute("ver");
    if (!workflowVerString.isEmpty()) {
        QVersionNumber workflowVersion = QVersionNumber::fromString(workflowVerString);
        if (!workflowVersion.isNull()) {
            mXml.setLoadedVersionNumber(workflowVersion);
        }
    } else {
        // 说明是较低版本，设置为v1.1
        mXml.setLoadedVersionNumber(QVersionNumber(1, 1, 0));
    }
    QDomNodeList wfListNodes = workflowsEle.childNodes();
    QSet< QString > names    = qlist_to_qset(wfo->getAllWorkflowNames());
    for (int i = 0; i < wfListNodes.size(); ++i) {
        QDomElement workflowEle = wfListNodes.at(i).toElement();
        if (workflowEle.tagName() != "workflow") {
            continue;
        }
        QString name = workflowEle.attribute("name");
        // 生成一个唯一名字
        name = DA::makeUniqueString(names, name);
        names.insert(name);
        // 建立工作流窗口
        DAPyWorkFlowEditWidget* wfe = wfo->appendWorkflow(name);
        isok &= mXml.loadElement(wfe, &workflowEle);
    }
    if (skipIndex) {
        int index = workflowsEle.attribute("currentIndex").toInt();
        index += oldProjectHaveWorkflow;
        wfo->setCurrentWorkflow(index);
    }
    setModified(isok);
    return isok;
}

/**
 * @brief 把一个工程追加到当前工程中
 * @param path
 * @param skipIndex 是否跳转到保存的tab索引
 */
bool DAAppProject::appendWorkflowInProject(const QByteArray& data, bool skipIndex)
{
    QDomDocument doc;
    QString error;
    if (!doc.setContent(data, &error)) {
        qCritical() << "load setContent error:" << error;
        return false;
    }
    return appendWorkflowInProject(doc, skipIndex);
}

/**
 * @brief 把绘图信息添加到工程
 * @param doc
 * @param chartmanager
 * @return
 */
bool DAAppProject::appendChartsInProject(const QDomDocument& doc, DAChartItemsManager* chartmanager)
{
    DAChartOperateWidget* chartOpt = getChartOperateWidget();
    Q_CHECK_PTR(chartOpt);
    QDomElement docElem  = doc.documentElement();                 // root
    QDomElement proEle   = docElem.firstChildElement("project");  // project
    QDomElement chartEle = proEle.firstChildElement("charts");
    if (chartEle.isNull()) {
        return false;
    }
    return mXml.loadElement(chartOpt, &chartEle, chartmanager);
}

/**
 * @brief 繁忙状态判断
 * @return
 */
bool DAAppProject::isBusy() const
{
    return mArchive->isBusy();
}

/**
 * @brief 根据数据文件名字，创建这个数据文件在本地的临时文件位置
 * @param dataName
 * @return
 */
QString DAAppProject::makeDataTemporaryFilePath(const QString& dataName)
{
    if (!mTempDir) {
        mTempDir = std::make_unique< QTemporaryDir >();
    }
    return mTempDir->filePath(dataName);
}

/**
 * @brief 根据数据文件名字，创建这个数据文件在zip文件的位置
 * @param dataName
 * @return
 */
QString DAAppProject::makeDataArchiveFilePath(const QString& dataName)
{
    return QString("datas/%1").arg(dataName);
}

/**
 * @brief 设置插件管理器
 * @param plugin
 */
void DAAppProject::setPluginMgr(DAAppPluginManager* plugin)
{
    mPluginMgr = plugin;
}

/**
 * @brief 清除工程
 */
void DAAppProject::clear()
{
    // 清除工作流
    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    wfo->clear();
    //! 清除数据
    DADataOperateWidget* dow = getDataOperateWidget();
    Q_CHECK_PTR(dow);
    dow->clear();
    //! 清除绘图
    DAChartOperateWidget* cow = getChartOperateWidget();
    Q_CHECK_PTR(cow);
    cow->clear();
    DAProjectInterface::clear();
}

/**
 * @brief 保存工程
 *
 * 保存过程先会生成一个临时的archive，命名规则为.~${工程文件名}
 *
 * @note 工作流保存过程如下：
 * -# 保存工作流扩展信息
 * -# 保存节点信息
 * -# 保存链接信息
 * -# 保存特殊item（非工作流的item）
 * -# 保存工厂扩展信息
 * -# 保存scene信息
 * @param path
 * @return
 */
bool DAAppProject::save(const QString& path)
{
    if (isBusy()) {
        daInfo << tr("The current project is busy");  // cn:当前工程正繁忙
        return false;
    }
    const QString oldProjectFilePath = getProjectFilePath();
    setStatusBarInBusy(tr("Saving project"));  // cn:正在保存工程
    setProjectPath(path);
    DA_WAIT_CURSOR_SCOPED();
    bool started = false;
    if (!executeSave(mArchive, path, &started)) {
        if (!started) {
            setStatusBarNotBusy(tr("Failed to save project"));  // cn:无法保存工程
        }
        setProjectPath(oldProjectFilePath);
        return false;
    }
    return true;
}

/**
 * @brief 加载
 *
 * @note 工作流加载过程如下：
 * -# 加载工作流扩展信息
 * -# 加载节点信息
 * -# 加载链接信息
 * -# 加载特殊item（非工作流的item）
 * -# 加载工厂扩展信息
 * -# 加载scene信息
 * @param path
 * @return
 */
bool DAAppProject::load(const QString& path)
{
    if (isBusy()) {
        daWarning << tr("The current project is busy");  // cn:当前工程正繁忙
        return false;
    }
    // 先确认是否是符合要求的工程
    if (!DAZipArchive::isCorrectFile(path)) {
        daCritical << tr("The file %1 is not a valid project file").arg(path);  // cn:文件%1不是正确的工程文件
        return false;
    }
    const QString oldProjectFilePath = getProjectFilePath();
    const bool oldDirty              = isDirty();
    const bool needSnapshot          = oldDirty || !isEmpty();
    QString snapshotPath;

    if (needSnapshot) {
        setStatusBarInBusy(tr("Creating project snapshot"));  // cn:正在创建工程快照
        if (!createProjectSnapshot(&snapshotPath)) {
            setStatusBarNotBusy(tr("Failed to backup current project"));  // cn:无法备份当前工程
            daCritical << tr("Failed to back up the current project before loading %1").arg(path);  // cn:加载%1前备份当前工程失败
            return false;
        }
    }
    setStatusBarInBusy(tr("Loading project"));  // cn:正在加载工程

    // 加载之前先清空
    clear();

    setProjectPath(path);
    bool started = false;
    if (!executeLoad(mArchive, path, &started)) {
        if (!started) {
            setStatusBarNotBusy(tr("Failed to load project"));  // cn:无法加载工程
        }
        if (!snapshotPath.isEmpty()) {
            setStatusBarInBusy(tr("Restoring previous project"));  // cn:正在恢复之前的工程
            if (restoreProjectSnapshot(snapshotPath, oldProjectFilePath, oldDirty)) {
                setStatusBarNotBusy(tr("Failed to load project, restored previous project"));  // cn:工程加载失败，已恢复之前的工程
            } else {
                setStatusBarNotBusy(tr("Failed to load project and failed to restore previous project"));  // cn:工程加载失败，且恢复之前的工程失败
                daCritical << tr("Failed to restore previous project from snapshot");  // cn:从快照恢复之前的工程失败
                setProjectPath(QString());
                setModified(false);
            }
            QFile::remove(snapshotPath);
        } else if (!started) {
            setProjectPath(QString());
            setModified(false);
        }
        return false;
    }
    if (!snapshotPath.isEmpty()) {
        QFile::remove(snapshotPath);
    }
    return true;
}
bool DAAppProject::requestSave()
{
    QString projectFilePath = getProjectFilePath();
    if (projectFilePath.isEmpty()) {
        QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        QFileDialog dialog(nullptr,
                           tr("Save Project"),  // cn:保存工程
                           desktop,
                           tr("Project Files") + QString(" (*.%1)").arg(DAAppProject::getProjectFileSuffix())  // cn:工程文件
        );
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setDefaultSuffix(DAAppProject::getProjectFileSuffix());
        if (QDialog::Accepted != dialog.exec()) {
            return false;
        }
        QStringList files = dialog.selectedFiles();
        if (files.isEmpty()) {
            return false;
        }
        projectFilePath = files.first();
    }
    bool saveRet = save(projectFilePath);
    if (!saveRet) {
        daCritical << tr("Failed to save project! Path: %1").arg(projectFilePath);  // cn:工程保存失败！路径为:%1
    }
    return saveRet;
}

bool DAAppProject::executeSave(DAZipArchiveThreadWrapper* archive, const QString& path, bool* started)
{
    if (started) {
        *started = false;
    }
    if (nullptr == archive) {
        return false;
    }

    // 保存系统信息，仅仅保存不读取
    makeSaveSystemInfoTask(archive);

    // 保存Python工作流逻辑数据（节点拓扑+参数值+连接关系）
    makeSaveWorkflowDataTask(archive);

    // 先把涉及ui的内容保存下来,ui是无法在其它线程操作，因此需要先保存下来
    makeSaveWorkFlowTask(archive);

    // datamanager
    makeSaveDataManagerTask(archive);

    // 表格样式
    makeSaveTableStyleTask(archive);

    // 数据操作窗口嵌套停靠区布局（已打开数据页列表 + dock 布局）
    makeSaveDataOperateLayoutTask(archive);

    // 绘图
    makeSaveChartTask(archive);

    // Agent会话（主线程收集活跃会话字节→子线程写 agent_sessions/<id>.jsonl）
    makeSaveAgentSessionsTask(archive);

    // 插件
    if (mPluginMgr) {
        const QList< DAAbstractPlugin* > plugins = mPluginMgr->getAllPlugins();
        for (DAAbstractPlugin* plugin : plugins) {
            auto task = plugin->createArchiveTask(true);
            if (task) {
                archive->appendTask(task);
            }
        }
    }

    const DAArchiveRunResult result = waitArchiveSave(archive, path);
    if (started) {
        *started = result.started;
    }
    return result.success;
}

bool DAAppProject::executeLoad(DAZipArchiveThreadWrapper* archive, const QString& path, bool* started, const QString& agentProjectPath)
{
    if (started) {
        *started = false;
    }
    if (nullptr == archive) {
        return false;
    }
    // agentProjectPath：供会话加载回调标记导入会话工程路径。空则用 path
    // （正常 load 路径 path 即工程路径；快照回滚路径 restoreProjectSnapshot 显式传真实工程路径，
    //  避免把会话标记成快照临时路径——CRITICAL-2）
    const QString effAgentPath = agentProjectPath.isEmpty() ? path : agentProjectPath;

    // 创建archive任务队列 - Python工作流逻辑数据（先注册，FIFO保证先执行）
    auto taskData = archive->appendByteLoadTask(c_workflowdata_save_filename, DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW_DATA);
    if (!taskData) {
        return false;
    }
    taskData->setLoadedCallBack([ this ](std::shared_ptr< DAAbstractArchiveTask > t) { loadedWorkflowData(t); });

    // 创建archive任务队列 - 工作流UI（后注册，FIFO保证后执行）
    auto task = archive->appendXmlLoadTask(c_workflowxml_save_filename, DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW);
    if (!task) {
        return false;
    }
    task->setLoadedCallBack([ this ](std::shared_ptr< DAAbstractArchiveTask > t) { loadedWorkflowInfo(t); });

    // 创建datamanager任务
    std::shared_ptr< DAZipArchiveTask_LoadDataManager > loadDataTask =
        std::make_shared< DAZipArchiveTask_LoadDataManager >();
    loadDataTask->setCode(DAAPPPROJECT_TASK_LOAD_ID_DATAMANAGER);
    loadDataTask->setLoadedCallBack([ this ](std::shared_ptr< DAAbstractArchiveTask > t) { loadedDataManager(t); });
    if (!archive->appendTask(loadDataTask)) {
        return false;
    }

    // ChartItemLoadTask必须在chart info 的XmlLoadTask之前
    auto taskChartItem =
        archive->appendChartItemLoadTask(c_chartitem_save_folder, DAAPPPROJECT_TASK_LOAD_ID_CHARTITEMMANAGER);

    // 表格样式加载（在 datamanager 之后，此时 DAData 及其持久化 id 已恢复，
    // 样式按 data-id 回填到会话级 DATableStyleRegistry，无需依赖 widget 是否打开）
    auto taskTableStyles = archive->appendXmlLoadTask(c_tablestylesxml_save_filename,
                                                       DAAPPPROJECT_TASK_LOAD_ID_TABLE_STYLES);
    if (taskTableStyles) {
        taskTableStyles->setLoadedCallBack(
            [ this ](std::shared_ptr< DAAbstractArchiveTask > t) { loadedTableStyles(t); });
    }

    // 数据操作窗口布局加载（在 datamanager 与 table-styles 之后：此时 DAData 及 id 已恢复，
    // 样式注册表已回填，showData 重建数据页可立即借用到会话级样式 manager）
    auto taskDataOperateLayout = archive->appendXmlLoadTask(c_dataoperatexml_save_filename,
                                                             DAAPPPROJECT_TASK_LOAD_ID_DATA_OPERATE_LAYOUT);
    if (taskDataOperateLayout) {
        taskDataOperateLayout->setLoadedCallBack(
            [ this ](std::shared_ptr< DAAbstractArchiveTask > t) { loadedDataOperateLayout(t); });
    }
    if (!taskChartItem) {
        return false;
    }
    taskChartItem->setLoadedCallBack([ this ](std::shared_ptr< DAAbstractArchiveTask > t) {
        const std::shared_ptr< DAZipArchiveTask_ChartItem > chartMgrArchive =
            std::static_pointer_cast< DAZipArchiveTask_ChartItem >(t);
        // 获取chartmanager
        mChartItemManager = chartMgrArchive->getChartItemsManager();
    });

    auto taskCharts = archive->appendXmlLoadTask(c_chartsxml_save_filename, DAAPPPROJECT_TASK_LOAD_ID_CHARTS_INFO);
    if (!taskCharts) {
        return false;
    }
    taskCharts->setLoadedCallBack([ this ](std::shared_ptr< DAAbstractArchiveTask > t) { loadedChartsInfo(t); });

    // Agent会话加载任务（plan-05）：worker 线程解压 agent_sessions/*.jsonl，
    // 回调（主线程，由 DAZipArchiveThreadWrapper::onTaskProgress 触发）先 setCurrentProjectPath
    // 再 loadSessionsFromProject 导入会话。注意：回调内**不**调 restoreLastActiveSession——
    // 初始化统一在 DAAppController::onProjectLoaded 做（MAJOR-3 统一初始化点）。
    // 时序（MAJOR-5）：本回调先于 projectLoaded/onProjectLoaded（waitArchiveLoad 嵌套
    // QEventLoop::exec(ExcludeUserInputEvents) + onLoadFinish emit projectLoaded）。
    {
        auto agentTask = std::make_shared< DAZipArchiveTask_LoadAgentSessions >();
        agentTask->setCode(DAAPPPROJECT_TASK_LOAD_ID_AGENT_SESSIONS);
        agentTask->setLoadedCallBack([ this, effAgentPath ](std::shared_ptr< DAAbstractArchiveTask > t) {
            auto loaded = std::static_pointer_cast< DAZipArchiveTask_LoadAgentSessions >(t);
            if (auto* agent = core()->getAgentInterface()) {
                // 契约4：先设置工程路径，使后续 setLastActive/过滤命中
                agent->setCurrentProjectPath(effAgentPath);
                // 契约4：带 projectPath 导入（importSessionFiles 用 effAgentPath 标记导入会话）
                agent->loadSessionsFromProject(loaded->takeSessions(), effAgentPath);
            }
        });
        if (!archive->appendTask(agentTask)) {
            return false;
        }
    }

    // 插件
    if (mPluginMgr) {
        const QList< DAAbstractPlugin* > plugins = mPluginMgr->getAllPlugins();
        for (DAAbstractPlugin* plugin : std::as_const(plugins)) {
            auto taskPlugin = plugin->createArchiveTask(false);
            if (taskPlugin) {
                archive->appendTask(taskPlugin);
            }
        }
    }

    const DAArchiveRunResult result = waitArchiveLoad(archive, path);
    if (started) {
        *started = result.started;
    }
    return result.success;
}

bool DAAppProject::createProjectSnapshot(QString* snapshotPath)
{
    if (nullptr == snapshotPath) {
        return false;
    }

    QTemporaryFile tempFile(QDir::tempPath() + QDir::separator()
                            + QStringLiteral("daworkbench-project-XXXXXX.%1").arg(getProjectFileSuffix()));
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        return false;
    }
    *snapshotPath = tempFile.fileName();
    tempFile.close();
    QFile::remove(*snapshotPath);

    DAZipArchiveThreadWrapper archive;
    bool started = false;
    if (!executeSave(&archive, *snapshotPath, &started) || !started) {
        QFile::remove(*snapshotPath);
        snapshotPath->clear();
        return false;
    }
    return true;
}

bool DAAppProject::restoreProjectSnapshot(const QString& snapshotPath, const QString& projectFilePath, bool isDirty)
{
    if (snapshotPath.isEmpty() || !QFileInfo::exists(snapshotPath)) {
        return false;
    }

    clear();
    DAZipArchiveThreadWrapper archive;
    bool started = false;
    // 传 projectFilePath 作为 agentProjectPath：使会话加载回调用真实工程路径标记导入会话，
    // 而非快照临时路径（CRITICAL-2：避免会话 projectPath 字段被临时路径污染）。
    if (!executeLoad(&archive, snapshotPath, &started, projectFilePath) || !started) {
        return false;
    }

    setProjectPath(projectFilePath);
    setModified(isDirty);
    // 回滚路径用局部 archive 不发 projectLoaded（成员 mArchive 才连 onLoadFinish），
    // 故 onProjectLoaded/restoreLastActiveSession 不自动触发——此处显式 emit projectLoaded，
    // 由 DAAppController::onProjectLoaded 统一接线初始化会话 UI（全新对话 + 填充下拉，
    // 亦符合 MAJOR-3 统一初始化点：restoreLastActiveSession 只在 onProjectLoaded 调一次）。
    Q_EMIT projectLoaded(projectFilePath);
    return true;
}

/**
 * @brief 保存系统信息
 * @param archive
 */
void DAAppProject::makeSaveSystemInfoTask(DAZipArchiveThreadWrapper* archive)
{
    QDomDocument doc;
    QDomProcessingInstruction processInstruction =
        doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(processInstruction);
    QDomElement root = doc.createElement("root");
    root.setAttribute("type", "system-info");
    doc.appendChild(root);
    QDomElement sysEle = DAXMLFileInterface::makeSysInfoElement(QStringLiteral("system"), &doc);
    root.appendChild(sysEle);
    // 创建archive任务队列
    auto t = archive->appendXmlSaveTask(QStringLiteral("system.xml"), doc);
    t->setName(tr("Save System Info"));             // cn:保存系统信息
    t->setDescribe(tr("Save system information"));  // cn:保存系统信息
}

/**
 * @brief 创建保存Python工作流逻辑数据的任务
 *
 * 遍历所有工作流标签页，通过DAPyWorkFlowSerializer调用Python端
 * 的to_xml()方法，将节点拓扑、参数值和连接关系序列化为XML。
 * Python XML通过CDATA嵌入到外层XML中，避免QDomDocument解析开销。
 * 使用appendByteSaveTask直接保存字节数据。
 *
 * @param archive ZIP归档线程包装器
 */
void DAAppProject::makeSaveWorkflowDataTask(DAZipArchiveThreadWrapper* archive)
{
    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    DAPyWorkFlowSerializer serializer;

    // 纯字符串拼接，不经过QDomDocument解析Python XML
    QString xml;
    xml += QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    xml += QStringLiteral("<root type=\"workflow-data\">\n<workflows>\n");

    DAPyGILGuard gilGuard;  // GIL保护：serializer.toXml() 调用 Python
    const int cnt = wfo->count();
    for (int i = 0; i < cnt; ++i) {
        DAPyWorkFlowEditWidget* wfe = wfo->getWorkFlowWidget(i);
        QString tabName             = wfo->getWorkFlowWidgetName(i);
        DAPyWorkFlowManager* mgr    = wfe->getManager();
        if (!mgr || !mgr->isWorkflowValid()) {
            continue;
        }
        DAPyWorkFlow wf = mgr->getWorkflow();
        QString pyXml   = serializer.toXml(wf);
        if (pyXml.isEmpty()) {
            daWarning << tr("Failed to serialize workflow '%1' to XML").arg(tabName);  // cn:序列化工作流'%1'到XML失败
            continue;
        }
        // CDATA注入防护：转义 ]]>
        pyXml.replace("]]>", "]]]]><![CDATA[>");
        xml += "<workflow name=\"" + tabName.toHtmlEscaped() + "\"><![CDATA[";
        xml += pyXml;
        xml += "]]></workflow>\n";
    }

    xml += QStringLiteral("</workflows>\n</root>");

    auto t = archive->appendByteSaveTask(c_workflowdata_save_filename, xml.toUtf8());
    t->setName(tr("Save workflow data"));  // cn:保存工作流数据
    t->setDescribe(tr("Save Python workflow logic data (nodes, parameters, connections)"));  // cn:保存Python工作流逻辑数据（节点、参数、连接关系）
}

/**
 * @brief 创建保存工作流相关的保存任务
 * @param archive
 */
void DAAppProject::makeSaveWorkFlowTask(DAZipArchiveThreadWrapper* archive)
{
    //! 先把涉及ui的内容保存下来,ui是无法在其它线程操作，因此需要先保存下来
    QDomDocument workflowXml = createWorkflowUIDomDocument();
    // 创建archive任务队列
    auto t = archive->appendXmlSaveTask(c_workflowxml_save_filename, workflowXml);
    t->setName(tr("Save workflow information"));  // cn:保存工作流信息
    t->setDescribe(tr("Save workflow information, including the hierarchical relationships and rendering effects of "
                      "workflow graphics elements"));  // cn:保存工作流信息，包括工作流图元的层级关系渲染效果
}

/**
 * @brief 保存数据的任务
 * @param archive
 */
void DAAppProject::makeSaveDataManagerTask(DAZipArchiveThreadWrapper* archive)
{
    DADataManagerInterface* dataMgr = getDataManagerInterface();
    QDomDocument doc;
    QDomProcessingInstruction processInstruction =
        doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(processInstruction);
    QDomElement root = doc.createElement(QStringLiteral("root"));
    root.setAttribute("type", "data manager");
    doc.appendChild(root);
    // 保存DAData基本信息
    QDomElement dataListEle = doc.createElement(QStringLiteral("datas"));
    const int datacnt       = dataMgr->getDataCount();
    for (int i = 0; i < datacnt; ++i) {
        // 逐个遍历DAData，并生成datamanager.xml和把数据文件进行持久化
        DAData data                   = dataMgr->getData(i);
        DAAbstractData::DataType type = data.getDataType();
        QString name                  = data.getName();
        QString tempFilePath          = makeDataTemporaryFilePath(name);
        QString dataZipPath           = makeDataArchiveFilePath(name);
        switch (type) {
        case DAAbstractData::TypePythonDataFrame: {
            // 写文件，对于大文件，这里可能比较耗时，但python的gli机制，无法在线程里面写
            if (!DAData::writeToFile(data, tempFilePath)) {
                daCritical << tr("An exception occurred while serializing the dataframe named %1 to %2")
                                  .arg(name, tempFilePath);  // cn:把名称为%1的dataframe序列化到%2时出现异常
                continue;
            }
            // 创建archive任务队列
            archive->appendFileSaveTask(dataZipPath, tempFilePath);
        } break;
        default:
            break;
        }
        // 创建ele
        QDomElement dataEle = doc.createElement(QStringLiteral("d"));

        dataEle.setAttribute(QStringLiteral("name"), name);
        dataEle.setAttribute(QStringLiteral("type"), enumToString(type));
        // 持久化 id 作为表格样式跨会话匹配键（table-styles.xml 以 data-id 引用）
        dataEle.setAttribute(QStringLiteral("id"), QString::number(data.id()));

        QDomElement valueEle = doc.createElement(QStringLiteral("v"));
        valueEle.appendChild(doc.createTextNode(dataZipPath));

        QDomElement describeEle = doc.createElement(QStringLiteral("describe"));
        describeEle.appendChild(doc.createTextNode(data.getDescribe()));

        dataEle.appendChild(valueEle);
        dataListEle.appendChild(dataEle);
    }
    root.appendChild(dataListEle);
    // 创建archive任务队列
    auto t = archive->appendXmlSaveTask(QStringLiteral("data-manager.xml"), doc);
    t->setName(tr("Save data information"));                                                         // cn:保存数据信息
    t->setDescribe(tr("Save data information, including data names and data organization formats"));  // cn:保存数据信息，包括数据的名称数据的组织形式
}

/**
 * @brief 添加保存绘图任务
 *
 * 绘图包含了数据和界面，
 * @param archive
 */
void DAAppProject::makeSaveChartTask(DAZipArchiveThreadWrapper* archive)
{
    //! 先把涉及ui的内容保存下来,ui是无法在其它线程操作，因此需要先保存下来
    DAChartItemsManager chartItemMgr;
    QDomDocument chartXml = createChartsUIDomDocument(chartItemMgr);
    // 创建archive任务队列,先保存xml
    auto t1 = archive->appendXmlSaveTask(c_chartsxml_save_filename, chartXml);
    t1->setName(tr("Save charts information"));  // cn:保存绘图的基本信息
    t1->setDescribe(tr("Save charts information, including chart name and chart organization formats"));  // cn:保存绘图信息，包括绘图的名称绘图的组织形式
    // 创建chartitem保存任务
    auto t2 = archive->appendChartItemSaveTask(c_chartitem_save_folder, chartItemMgr);
    t2->setName(tr("Save chart items information"));      // cn:保存绘图元素的基本信息
    t2->setDescribe(tr("Save chart items information"));  // cn:保存绘图元素的基本信息
}

/**
 * @brief 保存表格样式任务
 *
 * 遍历样式注册表（按 DAData 键控，会话级存活，不依赖 widget 是否打开），
 * 收集各非空 styleManager 的样式，合并为 table-styles.xml，以 data-id 为匹配键。
 * @param archive 归档器
 */
void DAAppProject::makeSaveTableStyleTask(DAZipArchiveThreadWrapper* archive)
{
    QDomDocument doc;
    QDomProcessingInstruction pi = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(pi);
    QDomElement root = doc.createElement(QStringLiteral("root"));
    root.setAttribute("type", "table-styles");
    doc.appendChild(root);
    QDomElement projectEle = doc.createElement(QStringLiteral("project"));
    projectEle.setAttribute("version", getProjectVersion().toString());
    root.appendChild(projectEle);
    QDomElement stylesEle = doc.createElement(QStringLiteral("table-styles"));
    projectEle.appendChild(stylesEle);

    // 遍历样式注册表收集样式（样式随数据存在，不依赖 widget 是否打开）
    DADataOperateWidget* optWidget = getDataOperateWidget();
    if (optWidget && optWidget->styleRegistry()) {
        QList< QPair< DAData, DATableStyleManager* > > entries = optWidget->styleRegistry()->nonEmptyEntries();
        for (const auto& entry : entries) {
            const DAData& d      = entry.first;
            DATableStyleManager* mgr = entry.second;
            if (!mgr || mgr->isEmpty()) {
                continue;
            }
            QDomElement tableEle = doc.createElement(QStringLiteral("table"));
            // 主键 data-id 跨会话稳定（id 持久化于 data-manager.xml）
            tableEle.setAttribute(QStringLiteral("data-id"), QString::number(d.id()));
            // data-name 作人类可读与旧工程回退
            tableEle.setAttribute(QStringLiteral("data-name"), d.getName());
            mgr->toXml(doc, tableEle);
            stylesEle.appendChild(tableEle);
        }
    }

    auto t = archive->appendXmlSaveTask(c_tablestylesxml_save_filename, doc);
    t->setName(tr("Save table styles"));  // cn:保存表格样式
    t->setDescribe(tr("Save table cell styles, including background, font, foreground"));  // cn:保存表格单元格样式
}

/**
 * @brief 保存数据操作窗口嵌套停靠区布局任务
 *
 * 遍历 DADataOperateWidget 已打开数据页列表（按插入顺序），写 <data-page data-id/data-name>，
 * 再追加 <data-layout> 节点装 saveDataLayout() 的 base64。顶层 ads::CDockManager::saveState
 * 不捕获嵌套管理器布局，故需单独保存。data-id 跨会话稳定（持久化于 data-manager.xml），
 * data-name 作人类可读与旧工程回退。
 * @param archive 归档器
 */
void DAAppProject::makeSaveDataOperateLayoutTask(DAZipArchiveThreadWrapper* archive)
{
    QDomDocument doc;
    QDomProcessingInstruction pi = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(pi);
    QDomElement root = doc.createElement(QStringLiteral("root"));
    root.setAttribute("type", "data-operate");
    doc.appendChild(root);
    QDomElement projectEle = doc.createElement(QStringLiteral("project"));
    projectEle.setAttribute("version", getProjectVersion().toString());
    root.appendChild(projectEle);
    QDomElement dataOperateEle = doc.createElement(QStringLiteral("data-operate"));
    projectEle.appendChild(dataOperateEle);

    DADataOperateWidget* optWidget = getDataOperateWidget();
    if (optWidget) {
        // 已打开数据页列表（插入顺序），供加载时按顺序 showData 重建 dock
        QList< DAData > openedDatas = optWidget->getOpenedDataList();
        for (const DAData& d : std::as_const(openedDatas)) {
            if (d.isNull()) {
                continue;
            }
            QDomElement pageEle = doc.createElement(QStringLiteral("data-page"));
            pageEle.setAttribute(QStringLiteral("data-id"), QString::number(d.id()));
            pageEle.setAttribute(QStringLiteral("data-name"), d.getName());
            dataOperateEle.appendChild(pageEle);
        }
        // 嵌套停靠区布局：base64 编码避免二进制内容破坏 XML 文本
        QByteArray layoutState = optWidget->saveDataLayout();
        if (!layoutState.isEmpty()) {
            QDomElement layoutEle = doc.createElement(QStringLiteral("data-layout"));
            QDomText layoutText   = doc.createTextNode(QString::fromLatin1(layoutState.toBase64()));
            layoutEle.appendChild(layoutText);
            dataOperateEle.appendChild(layoutEle);
        }
    }

    auto t = archive->appendXmlSaveTask(c_dataoperatexml_save_filename, doc);
    t->setName(tr("Save data operate layout"));                // cn:保存数据操作布局
    t->setDescribe(tr("Save opened data pages and dock layout"));  // cn:保存已打开数据页与停靠布局
}

/**
 * @brief 保存Agent会话任务（plan-05）
 *
 * 主线程先经 DAAgentInterface::exportActiveSessions() 收集当前活跃会话字节
 * （关键：不能在子线程调 UI/Module），再循环 appendByteSaveTask 把字节交给
 * 子线程写入 agent_sessions/<id>.jsonl。仿 makeSaveWorkflowDataTask 先序列化再
 * appendByteSaveTask 的子线程约束模式。
 * @param archive ZIP归档线程包装器
 */
void DAAppProject::makeSaveAgentSessionsTask(DAZipArchiveThreadWrapper* archive)
{
    DAAgentInterface* agent = core()->getAgentInterface();
    if (!agent) {
        return;  // 无 Agent 模块（未启用），跳过
    }
    // 主线程收集会话字节（exportActiveSessions 同主线程，无并发）
    QHash< QString, QByteArray > sessions = agent->exportActiveSessions();
    if (sessions.isEmpty()) {
        return;  // 无活跃会话，不写 agent_sessions/（zip 无该文件夹，加载时正常空操作）
    }
    for (auto it = sessions.constBegin(); it != sessions.constEnd(); ++it) {
        if (it.key().isEmpty()) {
            continue;
        }
        QString relPath = QStringLiteral("agent_sessions/%1.jsonl").arg(it.key());
        // 子线程写字节
        auto t = archive->appendByteSaveTask(relPath, it.value());
        if (t) {
            t->setName(tr("Save agent session"));           // cn:保存Agent会话
            t->setDescribe(tr("Save agent chat session history"));  // cn:保存Agent聊天会话历史
        }
    }
}

QDomDocument DAAppProject::createWorkflowUIDomDocument()
{
    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    QDomDocument doc;
    QDomProcessingInstruction processInstruction =
        doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(processInstruction);
    QDomElement root = doc.createElement("root");
    root.setAttribute("type", "workflow");
    doc.appendChild(root);
    QDomElement project = doc.createElement("project");
    project.setAttribute("version", getProjectVersion().toString());  // 版本
    root.appendChild(project);
    // 把所有的工作流保存
    QDomElement workflowsElement = mXml.makeElement(wfo, "workflows", &doc);
    project.appendChild(workflowsElement);
    return doc;
}

/**
 * @brief 创建绘图xml
 * @param chartItems
 * @return
 */
QDomDocument DAAppProject::createChartsUIDomDocument(DAChartItemsManager& chartItems)
{
    DAChartOperateWidget* chartOpt = getChartOperateWidget();
    Q_CHECK_PTR(chartOpt);
    QDomDocument doc;
    QDomProcessingInstruction processInstruction =
        doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(processInstruction);
    QDomElement root = doc.createElement("root");
    root.setAttribute("type", "chart");
    doc.appendChild(root);
    QDomElement project = doc.createElement("project");
    project.setAttribute("version", getProjectVersion().toString());  // 版本
    root.appendChild(project);
    // 把所有的chart保存
    QDomElement chartsElement = mXml.makeElement(chartOpt, "charts", &doc, &chartItems);
    project.appendChild(chartsElement);
    return doc;
}

bool DAAppProject::loadWorkflowUI(const QByteArray& data)
{
    // 加载之前先清空
    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    bool isok = appendWorkflowInProject(data, true);
    return isok;
}

void DAAppProject::onBeginSave(const QString& path)
{
    daInfo << tr("Begin saving archive to %1").arg(path);  // cn:开始保存档案到%1
}

void DAAppProject::onBeginLoad(const QString& path)
{
    daInfo << tr("Begin loading archive from %1").arg(path);  // cn:开始加载%1
}

/**
 * @brief 任务进度,对于读取操作，这个函数会携带读取的结果
 * @param total 总共
 * @param pos 当前位置
 */
void DAAppProject::onTaskProgress(std::shared_ptr< DAAbstractArchiveTask > t, int mode)
{
    if (DAAbstractArchiveTask::WriteMode == mode) {

    } else {
        // 读任务
        switch (t->getCode()) {
        case DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW: {
            setCurrentStatusText(tr("Loading workflow"));  // cn:正在加载工作流
        } break;
        case DAAPPPROJECT_TASK_LOAD_ID_DATAMANAGER: {
            setCurrentStatusText(tr("Loading data"));  // cn:正在加载数据
        } break;
        }
    }
}

/**
 * @brief 保存任务结束
 * @param code
 */
void DAAppProject::onSaveFinish(bool success)
{
    QString savePath = getProjectFilePath();
    if (success) {
        setModified(false);
        Q_EMIT projectSaved(savePath);
        daInfo << tr("Successfully saved archive: %1").arg(savePath);  // cn:成功保存工程:%1
        setStatusBarNotBusy(tr("Project saved successfully"));         // cn:成功保存工程
    } else {
        daWarning << tr("Failed to save archive: %1").arg(savePath);  // cn:无法保存工程:%1
        setStatusBarNotBusy(tr("Failed to save project"));            // cn:无法保存工程
    }
}

/**
 * @brief 读取任务结束
 * @param code
 */
void DAAppProject::onLoadFinish(bool success)
{
    QString loadPath = getProjectFilePath();
    if (success) {
        setModified(false);
        daInfo << tr("Successfully loaded archive: %1").arg(loadPath);  // cn:成功加载工程:%1
        Q_EMIT projectLoaded(loadPath);
        setStatusBarNotBusy(tr("Project loaded successfully"));  // cn:成功加载工程
    } else {
        setProjectPath(QString());
        daWarning << tr("Failed to load archive: %1").arg(loadPath);  // cn:无法加载工程:%1
        setStatusBarNotBusy(tr("Failed to load project"));            // cn:无法加载工程
    }
}

void DAAppProject::loadedWorkflowInfo(const std::shared_ptr< DAAbstractArchiveTask >& t)
{
    const std::shared_ptr< DAZipArchiveTask_Xml > xmlArchive = std::static_pointer_cast< DAZipArchiveTask_Xml >(t);
    QDomDocument xmlDoc                                      = xmlArchive->getDomDocument();
    if (xmlDoc.isNull()) {
        return;
    }

    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    if (wfo && wfo->count() > 0) {
        // 新路径：tab已由loadedWorkflowData创建，仅加载视图布局
        appendWorkflowView(xmlDoc);
    } else {
        // 旧路径：向后兼容旧文件（无workflow-data.xml）
        appendWorkflowInProject(xmlDoc);
    }
}

/**
 * @brief Python工作流逻辑数据加载回调
 *
 * 解析workflow-data.xml中的CDATA，通过DAPyWorkFlowSerializer反序列化
 * 得到完整的DAWorkflow（含节点拓扑、参数值、连接关系），
 * 为每个工作流创建空tab并通过manager->setWorkflow()注入Python数据。
 * 此回调先于loadedWorkflowInfo执行（FIFO顺序保证）。
 */
void DAAppProject::loadedWorkflowData(const std::shared_ptr< DAAbstractArchiveTask >& t)
{
    const std::shared_ptr< DAZipArchiveTask_ByteArray > byteTask =
        std::static_pointer_cast< DAZipArchiveTask_ByteArray >(t);
    QByteArray data = byteTask->getData();
    if (data.isEmpty()) {
        // 旧文件可能没有workflow-data.xml，跳过
        return;
    }

    // 解析外层XML获取<workflows>列表
    QDomDocument doc;
    if (!doc.setContent(data)) {
        daWarning << tr("Failed to parse workflow-data.xml");  // cn:解析workflow-data.xml失败
        return;
    }

    QDomElement rootEle      = doc.documentElement();
    QDomElement workflowsEle = rootEle.firstChildElement("workflows");
    if (workflowsEle.isNull()) {
        return;
    }

    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    DAPyWorkFlowSerializer serializer;

    DAPyGILGuard gilGuard;  // GIL保护：serializer.fromXml() 和 setWorkflow() 调用 Python
    QDomNodeList wfList = workflowsEle.childNodes();
    for (int i = 0; i < wfList.size(); ++i) {
        QDomElement wfEle = wfList.at(i).toElement();
        if (wfEle.tagName() != "workflow") {
            continue;
        }

        QString tabName = wfEle.attribute("name");
        // 创建空tab（Manager自动创建空的Python workflow）
        DAPyWorkFlowEditWidget* wfe = wfo->appendWorkflow(tabName);
        if (!wfe) {
            daWarning << tr("Failed to create workflow tab: %1").arg(tabName);  // cn:创建工作流标签页失败:%1
            continue;
        }

        // 提取CDATA中的Python XML字符串
        QString pyXml = wfEle.text();  // QDomCDATASection的text()返回CDATA内容
        if (pyXml.isEmpty()) {
            daWarning << tr("Empty Python workflow data for tab: %1").arg(tabName);  // cn:工作流标签页%1的Python数据为空
            continue;
        }

        // 反序列化Python workflow
        DAPyNodeFactory* factory = wfe->getManager()->getFactory();
        DAPyWorkFlow wf          = serializer.fromXml(pyXml, DAPyNodeFactory(*factory));
        if (!wf.isValid()) {
            daWarning << tr("Failed to deserialize Python workflow: %1").arg(tabName);  // cn:反序列化Python工作流失败:%1
            continue;
        }

        // 替换空的Python workflow
        wfe->getManager()->setWorkflow(wf);
    }
}

/**
 * @brief 加载工作流视图数据（Python数据已就绪）
 *
 * 遍历workflow.xml中的<workflow>元素，按name匹配已有tab，
 * 调用DAXmlHelper::loadWorkflowView()加载视图布局。
 */
void DAAppProject::appendWorkflowView(const QDomDocument& doc)
{
    DAPyWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);

    QDomElement docElem      = doc.documentElement();
    QDomElement proEle       = docElem.firstChildElement("project");
    QDomElement workflowsEle = proEle.firstChildElement("workflows");

    // 设置版本号
    QString verStr = workflowsEle.attribute("ver");
    if (!verStr.isEmpty()) {
        QVersionNumber version = QVersionNumber::fromString(verStr);
        if (!version.isNull()) {
            mXml.setLoadedVersionNumber(version);
        }
    } else {
        mXml.setLoadedVersionNumber(QVersionNumber(1, 1, 0));
    }

    QDomNodeList wfList = workflowsEle.childNodes();
    for (int i = 0; i < wfList.size(); ++i) {
        QDomElement workflowEle = wfList.at(i).toElement();
        if (workflowEle.tagName() != "workflow") {
            continue;
        }

        QString tabName = workflowEle.attribute("name");
        // 查找已有tab（由loadedWorkflowData创建）
        DAPyWorkFlowEditWidget* wfe = nullptr;
        for (int j = 0; j < wfo->count(); ++j) {
            if (wfo->getWorkFlowWidgetName(j) == tabName) {
                wfe = wfo->getWorkFlowWidget(j);
                break;
            }
        }
        if (wfe) {
            mXml.loadWorkflowView(wfe, &workflowEle);
        } else {
            daWarning << tr("appendWorkflowView: tab '%1' not found, skipping view load")
                             .arg(tabName);  // cn:appendWorkflowView: 未找到标签页'%1'，跳过视图加载
        }
    }
}

void DAAppProject::loadedDataManager(const std::shared_ptr< DAAbstractArchiveTask >& t)
{
    //! 读取datamanager
    const std::shared_ptr< DAZipArchiveTask_LoadDataManager > datamgrTask =
        std::static_pointer_cast< DAZipArchiveTask_LoadDataManager >(t);
    DADataManagerInterface* dataMgr = getDataManagerInterface();
    QDomDocument xmlDoc             = datamgrTask->getDataManagerDomDocument();
    if (xmlDoc.isNull()) {
        daWarning << tr("Missing data content");  // cn:缺少数据内容
        return;
    }
    QDomElement docElem  = xmlDoc.documentElement();                            // root
    QDomElement datasEle = docElem.firstChildElement(QStringLiteral("datas"));  // datas
    auto datasNodes      = datasEle.childNodes();
    for (int i = 0; i < datasNodes.size(); ++i) {
        QDomElement dEle = datasNodes.at(i).toElement();
        // 获取数据名字
        QString name               = dEle.attribute(QStringLiteral("name"));
        QString type               = dEle.attribute(QStringLiteral("type"));
        QDomElement valueEle       = dEle.firstChildElement(QStringLiteral("v"));
        QString valueText          = valueEle.text();
        QDomElement describeEle    = dEle.firstChildElement(QStringLiteral("describe"));
        QString describeText       = describeEle.text();
        DAAbstractData::DataType t = stringToEnum(type, DAAbstractData::TypeNone);
        switch (t) {
        case DAAbstractData::TypePythonDataFrame: {
            if (!DAPyScripts::isInitScripts()) {
                daCritical << tr("Python script is not initialized");  // cn:脚本没有初始化
                continue;  // 跳过当前数据项，继续加载后续数据
            }
            QString tempLocalFilePath = datamgrTask->getLocalTempFilePath(valueText);
            if (tempLocalFilePath.isEmpty()) {
                daCritical << tr("Unable to find the temporary file corresponding to %1").arg(valueText);  // cn:无法找到%1对应的临时文件
                continue;  // 跳过当前数据项，继续加载后续数据
            }
            DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
            DAPyDataFrame df;
            if (!pydf.from_parquet(df, tempLocalFilePath)) {
                daCritical << tr("Unable to serialize file %1 into a DataFrame").arg(tempLocalFilePath);  // cn:无法把文件%1序列化为DataFrame
                continue;  // 跳过当前数据项，继续加载后续数据
            }
            qDebug() << df;
            // 创建DAData
            DAData dataDataframe(df);
            dataDataframe.setName(name);
            dataDataframe.setDescribe(describeText);
            // 恢复持久化的 id，作为 table-styles.xml 跨会话匹配键
            // 旧工程无 id 属性时跳过（dataDataframe 构造时已生成新 id，按 name 回退匹配）
            if (dEle.hasAttribute(QStringLiteral("id"))) {
                bool ok = false;
                DAAbstractData::IdType savedId = dEle.attribute(QStringLiteral("id")).toULongLong(&ok);
                if (ok && savedId != 0) {
                    dataDataframe.rawPointer()->setID(savedId);
                }
            }
            // 不使用dataMgr->addData(),因为这个是带回退的
            dataMgr->dataManager()->addData(dataDataframe);
        } break;
        default:
            break;
        }
    }
}

void DAAppProject::loadedChartsInfo(const std::shared_ptr< DAAbstractArchiveTask >& t)
{
    // 加载绘图
    const std::shared_ptr< DAZipArchiveTask_Xml > xmlArchive = std::static_pointer_cast< DAZipArchiveTask_Xml >(t);
    // 读取xml
    QDomDocument xmlDoc = xmlArchive->getDomDocument();
    if (xmlDoc.isNull()) {
        return;
    }
    //
    appendChartsInProject(xmlDoc, &mChartItemManager);
}

/**
 * @brief 表格样式加载回调
 *
 * 解析 table-styles.xml，按 data-id 优先匹配 DAData（getDataById），回退 data-name。
 * 样式回填到会话级 DATableStyleRegistry（随数据存在，不依赖 widget 是否打开），
 * 若该数据已有打开的 widget 则触发刷新重绘。
 * @param t 归档任务
 */
void DAAppProject::loadedTableStyles(const std::shared_ptr< DAAbstractArchiveTask >& t)
{
    const std::shared_ptr< DAZipArchiveTask_Xml > xmlArchive = std::static_pointer_cast< DAZipArchiveTask_Xml >(t);
    QDomDocument xmlDoc = xmlArchive->getDomDocument();
    if (xmlDoc.isNull()) {
        return;
    }
    QDomElement root = xmlDoc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String("root")) {
        return;
    }
    QDomElement projectEle = root.firstChildElement(QStringLiteral("project"));
    if (projectEle.isNull()) {
        return;
    }
    QDomElement stylesEle = projectEle.firstChildElement(QStringLiteral("table-styles"));
    if (stylesEle.isNull()) {
        return;
    }
    DADataOperateWidget* optWidget = getDataOperateWidget();
    if (!optWidget || !optWidget->styleRegistry()) {
        return;
    }
    DADataManagerInterface* dataMgr = getDataManagerInterface();
    QDomNode n = stylesEle.firstChild();
    while (!n.isNull()) {
        QDomElement tableEle = n.toElement();
        if (tableEle.isNull() || tableEle.tagName() != QLatin1String("table")) {
            n = n.nextSibling();
            continue;
        }
        // 优先按 data-id 匹配（跨会话稳定），回退 data-name（旧工程）
        DAData matched;
        if (tableEle.hasAttribute(QStringLiteral("data-id"))) {
            bool ok = false;
            DAAbstractData::IdType id = tableEle.attribute(QStringLiteral("data-id")).toULongLong(&ok);
            if (ok && id != 0) {
                matched = dataMgr->getDataById(id);
            }
        }
        if (matched.isNull()) {
            QString dataName = tableEle.attribute(QStringLiteral("data-name"));
            if (!dataName.isEmpty()) {
                matched = dataMgr->dataManager()->findData(dataName);
            }
        }
        if (matched.isNull()) {
            QString hint = tableEle.attribute(QStringLiteral("data-name"));
            if (hint.isEmpty()) {
                hint = tableEle.attribute(QStringLiteral("data-id"));
            }
            daWarning << tr("Table style for data '%1' has no matching data, skipped").arg(hint);  // cn:数据'%1'的表格样式未找到匹配数据，已跳过
            n = n.nextSibling();
            continue;
        }
        // 回填到会话级注册表（随数据存在，后续打开 widget 即可见）
        DATableStyleManager* mgr = optWidget->styleRegistry()->getOrCreate(matched);
        if (mgr) {
            mgr->fromXml(tableEle);
            // 若该数据已有打开的 widget，触发刷新重绘
            if (auto* w = optWidget->findDataFrameWidget(matched)) {
                w->refreshTable();
            }
        }
        n = n.nextSibling();
    }
}

/**
 * @brief 数据操作窗口布局加载回调
 *
 * 先遍历 <data-page>：按 data-id 匹配（跨会话稳定，持久化于 data-manager.xml），
 * 回退 data-name（旧工程）；命中则 showData 重建数据页 dock（objectName=data id），
 * 供 restoreDataLayout 按 objectName 匹配布局。所有页重建后 restoreDataLayout 恢复
 * dock 排布；无 <data-layout> 节点则保持默认标签顺序（向后兼容）。
 * @note 依赖时序：本回调在 loadedDataManager、loadedTableStyles 之后触发（FIFO），
 *       DAData 及 id 已恢复，样式注册表已回填。
 * @param t 归档任务
 */
void DAAppProject::loadedDataOperateLayout(const std::shared_ptr< DAAbstractArchiveTask >& t)
{
    const std::shared_ptr< DAZipArchiveTask_Xml > xmlArchive = std::static_pointer_cast< DAZipArchiveTask_Xml >(t);
    QDomDocument xmlDoc = xmlArchive->getDomDocument();
    if (xmlDoc.isNull()) {
        return;
    }
    QDomElement root = xmlDoc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String("root")) {
        return;
    }
    QDomElement projectEle = root.firstChildElement(QStringLiteral("project"));
    if (projectEle.isNull()) {
        return;
    }
    QDomElement dataOperateEle = projectEle.firstChildElement(QStringLiteral("data-operate"));
    if (dataOperateEle.isNull()) {
        return;
    }
    DADataOperateWidget* optWidget = getDataOperateWidget();
    if (!optWidget) {
        return;
    }
    DADataManagerInterface* dataMgr = getDataManagerInterface();
    // 第一遍：按顺序重建所有数据页 dock（restoreState 需所有 dock 已存在且 objectName 匹配）
    QDomNode n = dataOperateEle.firstChild();
    QByteArray dataLayout;
    while (!n.isNull()) {
        QDomElement childEle = n.toElement();
        if (childEle.isNull()) {
            n = n.nextSibling();
            continue;
        }
        // data-layout 节点，延迟到所有页重建完成后恢复
        if (childEle.tagName() == QLatin1String("data-layout")) {
            dataLayout = QByteArray::fromBase64(childEle.text().toLatin1());
            n = n.nextSibling();
            continue;
        }
        // 仅处理 data-page 节点（兼容性：跳过未知子节点）
        if (childEle.tagName() != QLatin1String("data-page")) {
            n = n.nextSibling();
            continue;
        }
        // 优先按 data-id 匹配（跨会话稳定），回退 data-name（旧工程）
        DAData matched;
        if (childEle.hasAttribute(QStringLiteral("data-id"))) {
            bool ok = false;
            DAAbstractData::IdType id = childEle.attribute(QStringLiteral("data-id")).toULongLong(&ok);
            if (ok && id != 0) {
                matched = dataMgr->getDataById(id);
            }
        }
        if (matched.isNull()) {
            QString dataName = childEle.attribute(QStringLiteral("data-name"));
            if (!dataName.isEmpty()) {
                matched = dataMgr->dataManager()->findData(dataName);
            }
        }
        if (matched.isNull()) {
            QString hint = childEle.attribute(QStringLiteral("data-name"));
            if (hint.isEmpty()) {
                hint = childEle.attribute(QStringLiteral("data-id"));
            }
            daWarning << tr("Data operate page '%1' has no matching data, skipped").arg(hint);  // cn:数据操作页'%1'未找到匹配数据，已跳过
            n = n.nextSibling();
            continue;
        }
        optWidget->showData(matched);
        n = n.nextSibling();
    }
    // 第二遍：所有数据页创建完毕后恢复停靠布局（按 objectName=data id 匹配；无布局时保持默认标签顺序）
    if (!dataLayout.isEmpty()) {
        optWidget->restoreDataLayout(dataLayout);
    }
}

void DAAppProject::setStatusBarInBusy(const QString& info)
{
    DAStatusBarInterface* statusBar = getStatusBar();
    if (!statusBar) {
        return;
    }
    statusBar->showProgressBar();
    statusBar->setBusy(true);
    if (!info.isNull()) {
        statusBar->showMessage(info);
    } else {
        statusBar->clearMessage();
    }
}

void DAAppProject::setStatusBarNotBusy(const QString& info)
{
    DAStatusBarInterface* statusBar = getStatusBar();
    if (!statusBar) {
        return;
    }
    statusBar->setBusy(false);
    statusBar->hideProgressBar();
    if (!info.isNull()) {
        statusBar->showMessage(info);
    } else {
        statusBar->clearMessage();
    }
}

void DAAppProject::setCurrentStatusText(const QString& info)
{
    DAStatusBarInterface* statusBar = getStatusBar();
    if (!statusBar) {
        return;
    }
    statusBar->setProgressText(info);
}

DAStatusBarInterface* DAAppProject::getStatusBar() const
{
    if (!core() || !core()->getUiInterface()) {
        return nullptr;
    }
    return core()->getUiInterface()->getStatusBar();
}

}  // end DA
