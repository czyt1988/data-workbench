#ifndef DAPROJECTINTERFACE_H
#define DAPROJECTINTERFACE_H
#include <QObject>
#include <QVersionNumber>
#include "DAInterfaceAPI.h"
#include "DABaseInterface.h"
namespace DA
{
class DACoreInterface;
class DAWorkFlowOperateWidget;
/**
 * @brief 负责总体工程的接口
 *
 * 通过此接口可以获取当前工程的基本信息
 */
class DAINTERFACE_API DAProjectInterface : public DABaseInterface
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAProjectInterface)
public:
    DAProjectInterface(DACoreInterface* c, QObject* par = nullptr);
    ~DAProjectInterface();
    //设置工作流操作窗口
    void setWorkFlowOperateWidget(DAWorkFlowOperateWidget* w);
    DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const;
    //获取工程文件的基础名
    QString getProjectBaseName() const;
    //工程路径,如D:/project
    QString getProjectDir() const;
    //获取工程文件的路径,如D:/project/da-project.dapro
    QString getProjectFilePath() const;
    //在重写load函数需要调用此函数设置工程路径
    void setProjectPath(const QString& projectPath);
    //获取工作区
    QString getWorkingDirectory() const;
    //是否dirty
    bool isDirty() const;
    //清空工程
    virtual void clear();
    //追加一个工厂的工作流进入本工程中，注意这个操作不会清空当前的工作流
    bool appendWorkflowInProject(const QString& path, bool skipIndex = false);
    //工程文件的版本,版本组成有大版本.中间版本.小版本组成，例如0.1.1
    static QVersionNumber getProjectVersion();
public:
	//工程文件的后缀
	static QString getProjectFileSuffix();
	static void setProjectFileSuffix(const QString& f);
public:
    //加载工程，加载完成后需要发射projectLoaded信号
    virtual bool load(const QString& path);
    //保存工程，保存成功后需要发射projectSaved信号
    virtual bool save(const QString& path);
public slots:
    //设置为dirty
    void setDirty(bool on);
signals:
    /**
     * @brief 工程加载完成
     * @param path 工程的路径
     */
    void projectLoaded(const QString& path);
    /**
     * @brief 工程成功保存
     * @param path 保存的路径
     */
    void projectSaved(const QString& path);
    /**
     * @brief 工程变为脏
     * @param on 脏标识
     */
    void becomeDirty(bool on);
    /**
     * @brief 工程被清空触发信号
     */
    void projectIsCleaned();
};
}

#endif  // DAPROJECTINTERFACE_H
