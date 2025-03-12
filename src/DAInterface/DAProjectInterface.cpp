#include "DAProjectInterface.h"
#include <QFileInfo>
#include <QSysInfo>
#include "DAWorkFlowOperateWidget.h"
#include "DAStringUtil.h"
#include "DAXmlHelper.h"
#include "DAQtContainerUtil.hpp"
#include "DADataManagerInterface.h"
namespace DA
{

//===================================================
// DAProjectInterfacePrivate
//===================================================
class DAProjectInterface::PrivateData
{
	DA_DECLARE_PUBLIC(DAProjectInterface)
public:
	PrivateData(DAProjectInterface* p);
	// 存在路径
	bool isHaveProjectFilePath() const;
	// 在parent下，插入一个tag，tag下包含文字text
	void appendElementWithText(QDomElement& parent, const QString& tagName, const QString& text, QDomDocument& doc) const;
	// 保存本地信息包括时间日期等等
	void saveLocalInfo(QDomElement& root, QDomDocument& doc) const;

public:
	bool mIsDirty { false };  ///< 脏标识
	DAWorkFlowOperateWidget* mWorkFlowOperateWidget { nullptr };
	DADataManagerInterface* mDataManagerInterface { nullptr };
	QFileInfo mProjectFileInfo;  ///< 记录工程文件信息
	DAXmlHelper mXml;
	static QString s_suffix;  ///< 工程文件后缀
};

/**
 * @brief 工程文件默认后缀
 */
QString DAProjectInterface::PrivateData::s_suffix = QString("asproj");

DAProjectInterface::PrivateData::PrivateData(DAProjectInterface* p) : q_ptr(p)
{
    mXml.setLoadedVersionNumber(DAProjectInterface::getProjectVersion());
}

bool DAProjectInterface::PrivateData::isHaveProjectFilePath() const
{
    return mProjectFileInfo.isFile();
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
void DAProjectInterface::PrivateData::appendElementWithText(QDomElement& parent,
                                                            const QString& tagName,
                                                            const QString& text,
                                                            QDomDocument& doc) const
{
	QDomElement ele = doc.createElement(tagName);
	ele.appendChild(doc.createTextNode(text));
	parent.appendChild(ele);
}

/**
 * @brief 保存本地信息包括时间日期等等
 * @param root
 * @param doc
 */
void DAProjectInterface::PrivateData::saveLocalInfo(QDomElement& root, QDomDocument& doc) const
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
//===================================================
// DAProjectInterface
//===================================================
DAProjectInterface::DAProjectInterface(DACoreInterface* c, QObject* par) : DABaseInterface(c, par), DA_PIMPL_CONSTRUCT
{
}

DAProjectInterface::~DAProjectInterface()
{
}

/**
 * @brief 设置工作流操作窗口
 * @param w
 */
void DAProjectInterface::setWorkFlowOperateWidget(DAWorkFlowOperateWidget* w)
{
    d_ptr->mWorkFlowOperateWidget = w;
}
/**
 * @brief 获取工作流操作窗口
 * @param w
 */
DAWorkFlowOperateWidget* DAProjectInterface::getWorkFlowOperateWidget() const
{
    return d_ptr->mWorkFlowOperateWidget;
}

/**
 * @brief 设置数据管理接口
 * @param d
 */
void DAProjectInterface::setDataManagerInterface(DADataManagerInterface* d)
{
    d_ptr->mDataManagerInterface = d;
}

/**
 * @brief 获取数据管理接口
 * @return
 */
DADataManagerInterface* DAProjectInterface::getDataManagerInterface()
{
    return d_ptr->mDataManagerInterface;
}

/**
 * @brief 获取工程名
 *
 * 返回工程的文件名(不含后缀)
 * @return 如果没有设置工程，将返回空字符串
 */
QString DAProjectInterface::getProjectBaseName() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return (d_ptr->mProjectFileInfo.baseName());
}

/**
 * @brief 获取工程路径
 *
 * @sa setProjectPath
 * @return 如果没有设置工程，将返回空字符串
 */
QString DAProjectInterface::getProjectDir() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return d_ptr->mProjectFileInfo.absolutePath();
}

/**
 * @brief DAProjectInterface::getProjectFilePath
 * @note 注意这个工程路径是工程文件的路径，并不是工作区的路径，但设置工程路径会把工作区设置到当前目录下
 * @return
 */
QString DAProjectInterface::getProjectFilePath() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return d_ptr->mProjectFileInfo.absoluteFilePath();
}

/**
 * @brief 设置工程路径
 * @param projectPath
 * @note 注意这个工程路径是工程文件的路径，并不是工作区的路径，但设置工程路径会把工作区设置到当前目录下
 */
void DAProjectInterface::setProjectPath(const QString& projectPath)
{
    d_ptr->mProjectFileInfo.setFile(projectPath);
}

/**
 * @brief 获取工作区
 * @note 工程文件所在目录定义为工作区
 * @return 如果没有设置工程，将返回空字符串
 */
QString DAProjectInterface::getWorkingDirectory() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return d_ptr->mProjectFileInfo.absolutePath();
}
/**
 * @brief 工程是否脏
 * @return
 */
bool DAProjectInterface::isDirty() const
{
    return d_ptr->mIsDirty;
}

/**
 * @brief 清空工程
 */
void DAProjectInterface::clear()
{
	setModified(false);
	d_ptr->mProjectFileInfo = QFileInfo();
	emit projectIsCleaned();
}

/**
 * @brief 把一个工程追加到当前工程中
 * @param path
 * @param skipIndex 是否跳转到保存的tab索引
 */
bool DAProjectInterface::appendWorkflowInProject(const QByteArray& data, bool skipIndex)
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
			d_ptr->mXml.setLoadedVersionNumber(workflowVersion);
		}
	} else {
		// 说明是较低版本，设置为v1.1
		d_ptr->mXml.setLoadedVersionNumber(QVersionNumber(1, 1, 0));
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
		isok &= d_ptr->mXml.loadElement(wfe, &workflowEle);
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
 * @brief 工程文件的版本,版本组成有大版本.小版本.小小版本组成，例如1.0.0
 * @return
 */
QVersionNumber DAProjectInterface::getProjectVersion()
{
	static QVersionNumber s_version = QVersionNumber(1, 3, 0);
	return s_version;
}

/**
 * @brief 获取工程文件的后缀
 * @return
 */
QString DAProjectInterface::getProjectFileSuffix()
{
    return DAProjectInterface::PrivateData::s_suffix;
}

/**
 * @brief 设置工程文件的后缀
 * @param f
 * @return
 */
void DAProjectInterface::setProjectFileSuffix(const QString& f)
{
    DAProjectInterface::PrivateData::s_suffix = f;
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
bool DAProjectInterface::load(const QString& path)
{
	// 加载之前先清空
	clear();
	setProjectPath(path);
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}
	QByteArray dataWorkflow = file.readAll();
	bool isok               = loadWorkflow(dataWorkflow);

	if (isok) {
		emit projectLoaded(path);
	} else {
		setProjectPath(QString());
	}
	setModified(false);
	return isok;
}
/**
 * @brief 保存工程
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
bool DAProjectInterface::save(const QString& path)
{
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	QFile file(path);
	if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
		qCritical() << tr("open failed,path is %1").arg(path);
		return false;
	}
	QByteArray workflow = saveWorkflow();
	file.write(workflow);
	file.close();
	setProjectPath(path);
	emit projectSaved(path);
	setModified(false);
	return true;
}

QByteArray DAProjectInterface::saveWorkflow()
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
	d_ptr->saveLocalInfo(root, doc);
	QDomElement project = doc.createElement("project");
	project.setAttribute("version", getProjectVersion().toString());  // 版本
	root.appendChild(project);
	// 把所有的工作流保存
	QDomElement workflowsElement = d_ptr->mXml.makeElement(wfo, "workflows", &doc);
	project.appendChild(workflowsElement);
	return doc.toByteArray();
}

bool DAProjectInterface::loadWorkflow(const QByteArray& data)
{
	// 加载之前先清空
	DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
	Q_CHECK_PTR(wfo);
	bool isok = appendWorkflowInProject(data, true);
	return isok;
}

/**
 * @brief 保存datamanager
 * @return
 */
QByteArray DAProjectInterface::saveDataManager()
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
 * @brief 设置为dirty
 * @param on
 */
void DAProjectInterface::setModified(bool on)
{
	if (on != d_ptr->mIsDirty) {
		d_ptr->mIsDirty = on;
		emit dirtyStateChanged(on);
	}
}

}
