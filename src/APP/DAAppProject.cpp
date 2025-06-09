#include "DAAppProject.h"
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
// DA
#include "DAWorkFlowOperateWidget.h"
#include "DAXmlHelper.h"
#include "DAQtContainerUtil.hpp"
#include "DAStringUtil.h"
#include "DADataManagerInterface.h"
#include "DAAbstractArchiveTask.h"
#include "DAZipArchive.h"
#include "DAZipArchiveThreadWrapper.h"
#include "DADockingAreaInterface.h"
#include "DAZipArchiveTask_ByteArray.h"
#include "DAZipArchiveTask_Xml.h"
#include "DAZipArchiveTask_ArchiveFile.h"
#include "DADataOperateWidget.h"
#include "DADataEnumStringUtils.h"
#include "DAWaitCursorScoped.h"
// python
#if DA_ENABLE_PYTHON
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#endif

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
namespace DA
{

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

////////////////////////////////////////////////////

DAAppProject::DAAppProject(DACoreInterface* c, QObject* p) : DAProjectInterface(c, p)
{
	// 注册 std::shared_ptr<DAAbstractArchiveTask> 类型
	qRegisterMetaType< std::shared_ptr< DAAbstractArchiveTask > >("std::shared_ptr<DAAbstractArchiveTask>");

	mXml.setLoadedVersionNumber(DAProjectInterface::getProjectVersion());
	mArchive = new DAZipArchiveThreadWrapper(this);
	connect(mArchive, &DAZipArchiveThreadWrapper::beginSave, this, &DAAppProject::onBeginSave);
	connect(mArchive, &DAZipArchiveThreadWrapper::beginLoad, this, &DAAppProject::onBeginLoad);
	connect(mArchive, &DAZipArchiveThreadWrapper::beginSave, this, &DAAppProject::projectBeginSave);
	connect(mArchive, &DAZipArchiveThreadWrapper::beginLoad, this, &DAAppProject::projectBeginLoad);
	connect(mArchive, &DAZipArchiveThreadWrapper::taskProgress, this, &DAAppProject::onTaskProgress);
	connect(mArchive, &DAZipArchiveThreadWrapper::saved, this, &DAAppProject::onSaveFinish);
	connect(mArchive, &DAZipArchiveThreadWrapper::loaded, this, &DAAppProject::onLoadFinish);
}

DAAppProject::~DAAppProject()
{
}

DAWorkFlowOperateWidget* DAAppProject::getWorkFlowOperateWidget() const
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

bool DAAppProject::appendWorkflowInProject(const QDomDocument& doc, bool skipIndex)
{
	// 加载之前先清空
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
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
		// 建立工作流窗口
		DAWorkFlowEditWidget* wfe = wfo->appendWorkflow(name);
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
 * @brief 在parent下，插入一个tag，tag下包含文字text
 *
 * 达到如下效果：
 * @code
 * <parent>
 *   <tagName>text</tagName>
 * </parent>
 * @endcode
 * @param parent
 * @param tagName
 * @param text
 * @param doc
 */
void DAAppProject::appendElementWithText(QDomElement& parent, const QString& tagName, const QString& text, QDomDocument& doc)
{
	QDomElement ele = doc.createElement(tagName);
	ele.appendChild(doc.createTextNode(text));
	parent.appendChild(ele);
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
 * @brief 清除工程
 */
void DAAppProject::clear()
{
	// 清除工作流
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	wfo->clear();
	//! 清除数据
	DADataOperateWidget* dow = getDataOperateWidget();
	Q_CHECK_PTR(wfo);
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
		qInfo() << tr("current project is busy");  // cn:当前工程正繁忙
		return false;
	}

	setProjectPath(path);
	DA_WAIT_CURSOR_SCOPED();
	//! 先把涉及ui的内容保存下来,ui是无法在其它线程操作，因此需要先保存下来
	makeSaveWorkFlowTask(mArchive);

	//! datamanager
	makeSaveDataManagerTask(mArchive);

	//! 绘图

	//! 组件任务队列
	if (!mArchive->save(path)) {
		qCritical() << tr("failed to save archive to %1").arg(path);
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
		qWarning() << tr("current project is busy");  // cn:当前工程正繁忙
		return false;
	}
	// 加载之前先清空
	clear();

	setProjectPath(path);
	// 创建archive任务队列
	mArchive->appendXmlLoadTask(QStringLiteral("workflow.xml"), DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW);

	// 创建datamanager任务
    std::shared_ptr< DAZipArchiveTask_LoadDataManager > loadDataTask = std::make_shared< DAZipArchiveTask_LoadDataManager >();
	loadDataTask->setCode(DAAPPPROJECT_TASK_LOAD_ID_DATAMANAGER);
	mArchive->appendTask(loadDataTask);

	//! 组件任务队列
	if (!mArchive->load(path)) {
		qCritical() << tr("failed to laod archive from %1").arg(path);
		return false;
	}
	return true;
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
    archive->appendXmlSaveTask(QStringLiteral("workflow.xml"), workflowXml);
}

/**
 * @brief 保存数据的任务
 * @param archive
 */
void DAAppProject::makeSaveDataManagerTask(DAZipArchiveThreadWrapper* archive)
{
	DADataManagerInterface* dataMgr = getDataManagerInterface();
	QDomDocument doc;
    QDomProcessingInstruction processInstruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
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
				qCritical() << tr("An exception occurred while serializing the dataframe named %1 to %2")
                                   .arg(name, tempFilePath);  // cn:把名称为%1的dataframe序列化到%2时出现异常
				continue;
			}
			// 创建archive任务队列
			mArchive->appendFileSaveTask(dataZipPath, tempFilePath);
		} break;
		default:
			break;
		}
		// 创建ele
		QDomElement dataEle = doc.createElement(QStringLiteral("d"));

		dataEle.setAttribute(QStringLiteral("name"), name);
		dataEle.setAttribute(QStringLiteral("type"), enumToString(type));

		QDomElement valueEle = doc.createElement(QStringLiteral("v"));
		valueEle.appendChild(doc.createTextNode(dataZipPath));

		QDomElement describeEle = doc.createElement(QStringLiteral("describe"));
		describeEle.appendChild(doc.createTextNode(data.getDescribe()));

		dataEle.appendChild(valueEle);
		dataListEle.appendChild(dataEle);
	}
	root.appendChild(dataListEle);
	// 创建archive任务队列
    archive->appendXmlSaveTask(QStringLiteral("data-manager.xml"), doc);
}

/**
 * @brief 创建保存绘图的任务
 * @param archive
 */
void DAAppProject::makeSaveChartTask(DAZipArchiveThreadWrapper* archive)
{
}

/**
 * @brief 添加保存绘图任务
 *
 * 绘图包含了数据和界面，
 * @param archive
 */
void DAAppProject::makeSaveFiguresTask(DAZipArchiveThreadWrapper* archive)
{
}

/**
 * @brief 本机信息
 * @param doc
 * @return
 */
QDomElement DAAppProject::makeLocalInfoElement(QDomDocument& doc) const
{
	QDomElement localInfo = doc.createElement("local-info");
	// 获得计算机的名称
	appendElementWithText(localInfo, "machineHostName", QSysInfo::machineHostName(), doc);
	// 获得计算机的位数
	appendElementWithText(localInfo, "cpuArch", QSysInfo::currentCpuArchitecture(), doc);
	// 获得kernelType
	appendElementWithText(localInfo, "kernelType", QSysInfo::kernelType(), doc);
	// 获得kernelType
	appendElementWithText(localInfo, "kernelVersion", QSysInfo::kernelVersion(), doc);
	// 获得kernelType
	appendElementWithText(localInfo, "prettyProductName", QSysInfo::prettyProductName(), doc);
	return localInfo;
}

QDomDocument DAAppProject::createWorkflowUIDomDocument()
{
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	QDomDocument doc;
    QDomProcessingInstruction processInstruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild(processInstruction);
	QDomElement root = doc.createElement("root");
	root.setAttribute("type", "project");
	doc.appendChild(root);
	// 保存本机信息
	QDomElement localInfoEle = makeLocalInfoElement(doc);
	root.appendChild(localInfoEle);
	QDomElement project = doc.createElement("project");
	project.setAttribute("version", getProjectVersion().toString());  // 版本
	root.appendChild(project);
	// 把所有的工作流保存
	QDomElement workflowsElement = mXml.makeElement(wfo, "workflows", &doc);
	project.appendChild(workflowsElement);
	return doc;
}

bool DAAppProject::loadWorkflowUI(const QByteArray& data)
{
	// 加载之前先清空
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	bool isok = appendWorkflowInProject(data, true);
	return isok;
}

void DAAppProject::onBeginSave(const QString& path)
{
	qInfo() << tr("begin save archive to %1").arg(path);  // cn:开始保存档案到%1
}

void DAAppProject::onBeginLoad(const QString& path)
{
    qInfo() << tr("begin load archive from %1").arg(path);  // cn:开始加载%1
}

/**
 * @brief 任务进度,对于读取操作，这个函数会携带读取的结果
 * @param total 总共
 * @param pos 当前位置
 */
void DAAppProject::onTaskProgress(int total, int pos, const std::shared_ptr< DAAbstractArchiveTask >& t)
{
	Q_UNUSED(total);
	Q_UNUSED(pos);
	switch (t->getCode()) {
	case DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW: {
		const std::shared_ptr< DAZipArchiveTask_Xml > xmlArchive = std::static_pointer_cast< DAZipArchiveTask_Xml >(t);

		QDomDocument xmlDoc = xmlArchive->getDomDocument();
		qDebug() << "onTaskProgress:(" << total << "," << pos << "),xml=\n" << xmlDoc.toString();
		appendWorkflowInProject(xmlDoc);
	} break;
	case DAAPPPROJECT_TASK_LOAD_ID_DATAMANAGER: {
		//! 读取datamanager
        const std::shared_ptr< DAZipArchiveTask_LoadDataManager >
            datamgrTask                 = std::static_pointer_cast< DAZipArchiveTask_LoadDataManager >(t);
		DADataManagerInterface* dataMgr = getDataManagerInterface();
		QDomDocument xmlDoc             = datamgrTask->getDataManagerDomDocument();
		if (xmlDoc.isNull()) {
			qWarning() << tr("Missing data content");  // cn:缺少数据内容
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
#if DA_ENABLE_PYTHON
			case DAAbstractData::TypePythonDataFrame: {
				QString tempLocalFilePath = datamgrTask->getLocalTempFilePath(valueText);
				if (tempLocalFilePath.isEmpty()) {
					qCritical() << tr("Unable to find the temporary file corresponding to %1").arg(valueText);  // cn:无法在找到%1对应的临时文件
					return;
				}
				DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
				DAPyDataFrame df;
				if (!pydf.from_parquet(df, tempLocalFilePath)) {
					qCritical() << tr("Unable to serialize the file %1 into a Dataframe").arg(tempLocalFilePath);  // cn:无法把文件%1序列化为Dataframe
					return;
				}
				qDebug() << df;
				// 创建DAData
				DAData dataDataframe(df);
				dataDataframe.setName(name);
				dataDataframe.setDescribe(describeText);
				// 不使用dataMgr->addData(),因为这个是带回退的
				dataMgr->dataManager()->addData(dataDataframe);
			} break;
#endif
			default:
				break;
			}
		}
	} break;
	case 0:
		break;
	default: {
		qDebug() << tr("get unknown task code:%1").arg(t->getCode());
	} break;
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
		qInfo() << tr("Successfully save archive : %1").arg(savePath);  // cn:成功保存工程:%1
	} else {
		qWarning() << tr("Failed to save archive : %1").arg(savePath);  // cn:无法保存工程:%1
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
		qInfo() << tr("Successfully load archive : %1").arg(loadPath);  // cn:成功加载工程:%1
		Q_EMIT projectLoaded(loadPath);
	} else {
		setProjectPath(QString());
		qWarning() << tr("Failed to load archive : %1").arg(loadPath);  // cn:无法加载工程:%1
	}
}

}  // end DA
