#include "DAProjectInterface.h"
#include <QFileInfo>
#include "DAWorkFlowOperateWidget.h"
#include "DAStringUtil.h"
#include "DAXmlHelper.h"
namespace DA
{

//===================================================
// DAProjectInterfacePrivate
//===================================================
class DAProjectInterfacePrivate
{
    DA_IMPL_PUBLIC(DAProjectInterface)
public:
    DAProjectInterfacePrivate(DAProjectInterface* p);
    //存在路径
    bool isHaveProjectFilePath() const;

public:
    QFileInfo _projectFileInfo;  ///< 记录工程文件信息
    bool _isDirty;               ///< 脏标识
    DAXmlHelper _xml;
    DAWorkFlowOperateWidget* _workFlowOperateWidget;
};

DAProjectInterfacePrivate::DAProjectInterfacePrivate(DAProjectInterface* p) : q_ptr(p), _workFlowOperateWidget(nullptr)
{
}

bool DAProjectInterfacePrivate::isHaveProjectFilePath() const
{
    return _projectFileInfo.isFile();
}
//===================================================
// DAProjectInterface
//===================================================
DAProjectInterface::DAProjectInterface(DACoreInterface* c, QObject* par)
    : DABaseInterface(c, par), d_ptr(new DAProjectInterfacePrivate(this))
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
    d_ptr->_workFlowOperateWidget = w;
}
/**
 * @brief 获取工作流操作窗口
 * @param w
 */
DAWorkFlowOperateWidget* DAProjectInterface::getWorkFlowOperateWidget() const
{
    return d_ptr->_workFlowOperateWidget;
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
    return (d_ptr->_projectFileInfo.baseName());
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
    return d_ptr->_projectFileInfo.absolutePath();
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
    return d_ptr->_projectFileInfo.absoluteFilePath();
}

/**
 * @brief 设置工程路径
 * @param projectPath
 * @note 注意这个工程路径是工程文件的路径，并不是工作区的路径，但设置工程路径会把工作区设置到当前目录下
 */
void DAProjectInterface::setProjectPath(const QString& projectPath)
{
    d_ptr->_projectFileInfo.setFile(projectPath);
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
    return d_ptr->_projectFileInfo.absolutePath();
}
/**
 * @brief 工程是否脏
 * @return
 */
bool DAProjectInterface::isDirty() const
{
    return d_ptr->_isDirty;
}

/**
 * @brief 清空工程
 */
void DAProjectInterface::clear()
{
    setDirty(false);
    d_ptr->_projectFileInfo = QFileInfo();
    emit projectIsCleaned();
}

/**
 * @brief 把一个工程追加到当前工程中
 * @param path
 * @param skipIndex 是否跳转到保存的tab索引
 */
bool DAProjectInterface::appendWorkflowInProject(const QString& path, bool skipIndex)
{
    //加载之前先清空
    DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    QDomDocument doc;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QString error;
    if (!doc.setContent(&file, &error)) {
        qCritical() << "load setContent error:" << error;
        file.close();
        return false;
    }
    file.close();
    int oldProjectHaveWorkflow = wfo->count();  //已有的工作流数量
    bool isok                  = true;
    QDomElement docElem        = doc.documentElement();                  // root
    QDomElement proEle         = docElem.firstChildElement("project");   // project
    QDomElement workflowsEle   = proEle.firstChildElement("workflows");  // workflows
    QDomNodeList wfListNodes   = workflowsEle.childNodes();
    QSet< QString > names      = wfo->getAllWorkflowNames().toSet();
    for (int i = 0; i < wfListNodes.size(); ++i) {
        QDomElement workflowEle = wfListNodes.at(i).toElement();
        if (workflowEle.tagName() != "workflow") {
            continue;
        }
        QString name = workflowEle.attribute("name");
        //生成一个唯一名字
        name = DA::makeUniqueString(names, name);
        //建立工作流窗口
        DAWorkFlowEditWidget* wfe = wfo->appendWorkflow(name);
        isok &= d_ptr->_xml.loadElement(wfe, &workflowEle);
    }
    if (skipIndex) {
        int index = workflowsEle.attribute("currentIndex").toInt();
        index += oldProjectHaveWorkflow;
        wfo->setCurrentWorkflow(index);
    }
    setDirty(isok);
    return isok;
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
    //加载之前先清空
    DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    clear();
    bool isok = appendWorkflowInProject(path, true);
    if (isok) {
        setProjectPath(path);
        emit projectLoaded(path);
    }
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
    QDomDocument doc;
    QDomProcessingInstruction processInstruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(processInstruction);
    QDomElement root = doc.createElement("root");
    root.setAttribute("type", "project");
    doc.appendChild(root);
    QDomElement project = doc.createElement("project");
    root.appendChild(project);
    //把所有的工作流保存
    QDomElement workflowsElement = d_ptr->_xml.makeElement(wfo, "workflows", &doc);
    project.appendChild(workflowsElement);
    QTextStream outFile(&file);
    doc.save(outFile, 4);
    file.close();
    setProjectPath(path);
    emit projectSaved(path);
    setDirty(false);
    return true;
}

/**
 * @brief 设置为dirty
 * @param on
 */
void DAProjectInterface::setDirty(bool on)
{
    d_ptr->_isDirty = on;
    emit becomeDirty(on);
}

}
