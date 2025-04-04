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
#include "DAZipArchiveThreadWrapper.h"
#include "DADockingAreaInterface.h"
#include "DAZipArchiveTask_ByteArray.h"
#include "DAZipArchiveTask_Xml.h"
#include "DAZipArchiveTask_ArchiveFile.h"

#ifndef DAAPPPROJECT_TASK_LOAD_ID_BEGIN
#define DAAPPPROJECT_TASK_LOAD_ID_BEGIN 0x234
#endif
/**
 *@def 加载任务id - 工作流的ui
 */
#ifndef DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW
#define DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW (DAAPPPROJECT_TASK_LOAD_ID_BEGIN + 1)
#endif
namespace DA
{

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
 * @brief 清除工程
 */
void DAAppProject::clear()
{
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	wfo->clear();
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
	mTempDir = std::make_unique< QTemporaryDir >();
	setProjectPath(path);

	//! 先把涉及ui的内容保存下来,ui是无法在其它线程操作，因此需要先保存下来
	makeSaveWorkFlowTask(mArchive);

	//! datamanager
	makeSaveDataManagerTask(mArchive);
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
	setProjectPath(path);
	// 加载之前先清空
	clear();

	// 创建archive任务队列
	mArchive->appendXmlLoadTask(QStringLiteral("workflow.xml"), DAAPPPROJECT_TASK_LOAD_ID_WORKFLOW);
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
	mArchive->appendXmlSaveTask(QStringLiteral("workflow.xml"), workflowXml);
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
	QDomElement root = doc.createElement("root");
	root.setAttribute("type", "data manager");
	doc.appendChild(root);
	// 保存DAData基本信息
	QDomElement dataListEle = doc.createElement("datas");
	const int datacnt       = dataMgr->getDataCount();
	for (int i = 0; i < datacnt; ++i) {
		// 逐个遍历DAData，并生成datamanager.xml和把数据文件进行持久化
		DAData data                   = dataMgr->getData(i);
		DAAbstractData::DataType type = data.getDataType();
		QString name                  = data.getName();
		QString valueText;
		switch (type) {
		case DAAbstractData::TypePythonDataFrame: {
			valueText = mTempDir->filePath(name);
			// 写文件，对于大文件，这里可能比较耗时，但python的gli机制，无法在线程里面写
			if (!DAData::writeToFile(data, valueText)) {
				qCritical() << tr("An exception occurred while serializing the dataframe named %1 to %2")
								   .arg(name, valueText);  // cn:把名称为%1的dataframe序列化到%2时出现异常
				continue;
			}
			// 创建archive任务队列
			mArchive->appendFileSaveTask(QString("datas/%1").arg(name), valueText);
		} break;
		default:
			break;
		}
		// 创建ele
		QDomElement dataEle = doc.createElement("d");
		dataEle.setAttribute("name", name);
		dataEle.setAttribute("type", enumToString(type));
		QDomElement valueEle = doc.createElement("v");
		valueEle.appendChild(doc.createTextNode(valueText));
		dataEle.appendChild(valueEle);
		dataListEle.appendChild(dataEle);
	}
	root.appendChild(dataListEle);
	// 创建archive任务队列
	mArchive->appendXmlSaveTask(QStringLiteral("data-manager.xml"), doc);
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
	QDomProcessingInstruction processInstruction =
		doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
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
	} else {
		setProjectPath(QString());
		qWarning() << tr("Failed to load archive : %1").arg(loadPath);  // cn:无法加载工程:%1
	}
}

}  // end DA
