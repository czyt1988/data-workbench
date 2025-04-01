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
namespace DA
{

////////////////////////////////////////////////////

DAAppProject::DAAppProject(DACoreInterface* c, QObject* p) : DAProjectInterface(c, p)
{
    mXml.setLoadedVersionNumber(DAProjectInterface::getProjectVersion());
}

DAAppProject::~DAAppProject()
{
}

/**
 * @brief 创建Archive
 * @note 注意此archive是在另外一个线程中运行，不能在里面操作界面
 * @return
 */
DAAppArchive* DAAppProject::createSaveArchive()
{
	// 创建线程
	mThread  = new QThread();
	mArchive = new DAAppArchive();
	// 绑定
	mArchive->moveToThread(mThread);
	// 任务执行完结束线程
	connect(mArchive, &DAAppArchive::taskFinished, this, &DAAppProject::onSaveTaskFinish);
	connect(mArchive, &DAAppArchive::taskProgress, this, &DAAppProject::onTaskProgress);
	connect(mArchive, &DAAppArchive::taskFinished, mThread, &QThread::quit);
	// beginsave信号触发saveall
	connect(this, &DAAppProject::projectBeginSave, mArchive, &DAAppArchive::saveAll);

	connect(mThread, &QThread::finished, mThread, &QThread::deleteLater);
	connect(mThread, &QThread::finished, mArchive, &DAAppArchive::deleteLater);
	mThread->start();
	return mArchive;
}

DAAppArchive* DAAppProject::createLoadArchive()
{
	// 创建线程
	mThread  = new QThread();
	mArchive = new DAAppArchive();
	// 绑定
	mArchive->moveToThread(mThread);
	// 任务执行完结束线程
	connect(mArchive, &DAAppArchive::taskFinished, this, &DAAppProject::onSaveTaskFinish);
	connect(mArchive, &DAAppArchive::taskProgress, this, &DAAppProject::onTaskProgress);
	connect(mArchive, &DAAppArchive::taskFinished, mThread, &QThread::quit);
	// beginsave信号触发saveall
	connect(this, &DAAppProject::projectBeginSave, mArchive, &DAAppArchive::saveAll);

	connect(mThread, &QThread::finished, mThread, &QThread::deleteLater);
	connect(mThread, &QThread::finished, mArchive, &DAAppArchive::deleteLater);
	mThread->start();
	return mArchive;
}

/**
 * @brief 把一个工程追加到当前工程中
 * @param path
 * @param skipIndex 是否跳转到保存的tab索引
 */
bool DAAppProject::appendWorkflowInProject(const QByteArray& data, bool skipIndex)
{
	// 加载之前先清空
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	QDomDocument doc;
	QString error;
	if (!doc.setContent(data, &error)) {
		qCritical() << "load setContent error:" << error;
		return false;
	}
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
	setBusy(true);
	DAProjectInterface::save(path);
	//! 先把涉及ui的内容保存下来,ui是无法在其它线程操作，因此需要先保存下来
	QByteArray workflowUI = saveWorkflowUI();
	// 创建archive
	DAAppArchive* archive = createSaveArchive();
	//! 组件任务队列
	archive->appendTask(DAAppArchive::Task(".workflow.xml", workflowUI, tr("Save workflow information")));  // cn:保存工作流信息
	Q_EMIT projectBeginSave(path);
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
		qInfo() << tr("current project is busy");  // cn:当前工程正繁忙
		return false;
	}
	setBusy(true);
	DAProjectInterface::load(path);
	// 加载之前先清空
	clear();

	setProjectPath(path);
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}
	QByteArray dataWorkflow = file.readAll();
	bool isok               = loadWorkflowUI(dataWorkflow);

	if (isok) {
		emit projectLoaded(path);
	} else {
		setProjectPath(QString());
	}
	setModified(false);
	return isok;
}

void DAAppProject::saveLocalInfo(QDomElement& root, QDomDocument& doc) const
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
	root.appendChild(localInfo);
}

QByteArray DAAppProject::saveWorkflowUI()
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
	saveLocalInfo(root, doc);
	QDomElement project = doc.createElement("project");
	project.setAttribute("version", getProjectVersion().toString());  // 版本
	root.appendChild(project);
	// 把所有的工作流保存
	QDomElement workflowsElement = mXml.makeElement(wfo, "workflows", &doc);
	project.appendChild(workflowsElement);
	return doc.toByteArray();
}

bool DAAppProject::loadWorkflowUI(const QByteArray& data)
{
	// 加载之前先清空
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	bool isok = appendWorkflowInProject(data, true);
	return isok;
}

QByteArray DAAppProject::saveDataManager()
{
	DADataManagerInterface* dataMgr = getDataManagerInterface();
	int datacnt                     = dataMgr->getDataCount();
	QDomDocument doc;
	QDomProcessingInstruction processInstruction =
		doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild(processInstruction);
	QDomElement root = doc.createElement("root");
	root.setAttribute("type", "data manager");
	doc.appendChild(root);
	// 保存DAData基本信息
	QDomElement dataListEle = doc.createElement("datas");

	return doc.toByteArray();
}

/**
 * @brief 任务进度,对于读取操作，这个函数会携带读取的结果
 * @param total 总共
 * @param pos 当前位置
 */
void DAAppProject::onTaskProgress(int total, int pos, const DAAppArchive::Task& t)
{
	if (t.isWrite()) {
		// TODO:这里仅仅更新进度
	} else {
		// 说明是读操作，这里要处理读取的结果
		if (0 == t.relatePath.compare(".workflow.xml", Qt::CaseInsensitive)) {
			// 说明读取了workflow
		}
	}
}

/**
 * @brief 保存任务结束
 * @param code
 */
void DAAppProject::onSaveTaskFinish(int code)
{
	setBusy(false);
	QString savePath = getProjectFilePath();
	if (code == DAAppArchive::SaveSuccess) {
		qInfo() << tr("Successfully save archive : %1").arg(savePath);  // cn:成功保存工程:%1
	} else {
		qWarning() << tr("Failed to save archive : %1").arg(savePath);  // cn:无法保存工程:%1
	}
}

/**
 * @brief 读取任务结束
 * @param code
 */
void DAAppProject::onLoadTaskFinish(int code)
{
	setBusy(false);
	QString loadPath = getProjectFilePath();
	if (code == DAAppArchive::LoadSuccess) {
		qInfo() << tr("Successfully load archive : %1").arg(loadPath);  // cn:成功加载工程:%1
	} else {
		qWarning() << tr("Failed to load archive : %1").arg(loadPath);  // cn:无法加载工程:%1
	}
}

}  // end DA
