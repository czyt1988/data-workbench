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
class DADataManagerInterface;
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
	// 设置工作流操作窗口
	void setWorkFlowOperateWidget(DAWorkFlowOperateWidget* w);
	DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const;
	// 设置数据管理接口
	void setDataManagerInterface(DADataManagerInterface* d);
	DADataManagerInterface* getDataManagerInterface();
	// 获取工程文件的基础名
	QString getProjectBaseName() const;
	// 工程路径,如D:/project
	QString getProjectDir() const;
	// 获取工程文件的路径,如D:/project/da-project.dapro
	QString getProjectFilePath() const;
	// 在重写load函数需要调用此函数设置工程路径
	void setProjectPath(const QString& projectPath);
	// 获取工作区
	QString getWorkingDirectory() const;
	// 是否dirty
	bool isDirty() const;
	// 工程文件的版本,版本组成有大版本.中间版本.小版本组成，例如0.1.1
	static QVersionNumber getProjectVersion();
    // 是否繁忙，正在保存文件过程中会为繁忙状态
    bool isBusy() const;

public:
	// 工程文件的后缀
	static QString getProjectFileSuffix();
	static void setProjectFileSuffix(const QString& f);

public Q_SLOTS:
	// 加载工程，加载完成后需要发射projectLoaded信号
	virtual bool load(const QString& path);
	// 保存工程，保存成功后需要发射projectSaved信号
    virtual bool save(const QString& path);
    // 清空工程
    virtual void clear();
	// 设置为dirty,会发射becomeDirty
	void setModified(bool on = true);
protected Q_SLOTS:
    // 设置繁忙
    void setBusy(bool on);
Q_SIGNALS:
	/**
	 * @brief 工程加载完成
	 * @param path 工程的路径
	 */
	void projectLoaded(const QString& path);

    /**
     * @brief 工程开始保存
     *
     * 此信号发射代表工程开始保存，但还没保存完
     * @param path 保存的路径
     */
    void projectBeginSave(const QString& path);

	/**
     * @brief 工程成功保存
     *
     * 此信号发射代表工程已经保存完成
	 * @param path 保存的路径
	 */
	void projectSaved(const QString& path);
	/**
	 * @brief 工程脏信号改变
	 * @param on 脏标识
	 */
	void dirtyStateChanged(bool on);

	/**
	 * @brief 工程被清空触发信号
	 */
	void projectIsCleaned();
};
}

#endif  // DAPROJECTINTERFACE_H
