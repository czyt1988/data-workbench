#ifndef DAAPPPROJECT_H
#define DAAPPPROJECT_H
#include <QObject>
#include <QDomElement>
#include <QDomDocument>
#include <QMap>
#include <QTemporaryDir>
#include "DAProjectInterface.h"
#include "DAGlobals.h"
#include "DAPyLinkGraphicsItem.h"
#include <QThread>
#include "DAXmlHelper.h"
#include "DAZipArchiveThreadWrapper.h"
#include "Chart/DAChartItemsManager.h"

namespace DA
{
class DAAbstractArchiveTask;
class DAZipArchiveThreadWrapper;
class DAPyWorkFlowOperateWidget;
class DAPyWorkFlowEditWidget;
class DAPyWorkFlowGraphicsScene;
class DADataOperateWidget;
class DAChartOperateWidget;
class DAAppPluginManager;
class DAStatusBarInterface;
/**
 * @brief 负责整个节点的工程管理
 *
 * DA的工程文件是一个压缩包，因此打开da工程文件时，会在临时目录把这个压缩包解压
 * 在保存文件时，把临时文件对应的压缩包进行压缩并移动到指定位置
 */
class DAAppProject : public DAProjectInterface
{
	Q_OBJECT
public:
	DAAppProject(DACoreInterface* c, QObject* p = nullptr);
	virtual ~DAAppProject() override;
	// 工作流操作窗口
	DAPyWorkFlowOperateWidget* getWorkFlowOperateWidget() const;
	// 数据操作窗口
	DADataOperateWidget* getDataOperateWidget() const;
	// 绘图窗口
	DAChartOperateWidget* getChartOperateWidget() const;
	// 追加一个工厂的工作流进入本工程中，注意这个操作不会清空当前的工作流
	bool appendWorkflowInProject(const QDomDocument& doc, bool skipIndex = false);
	bool appendWorkflowInProject(const QByteArray& data, bool skipIndex = false);
	// 加载工作流视图数据（Python数据已就绪，仅创建图形项）
	void appendWorkflowView(const QDomDocument& doc);
	// 把绘图信息添加到工程
	bool appendChartsInProject(const QDomDocument& doc, DAChartItemsManager* chartmanager);
	// 繁忙状态判断
	virtual bool isBusy() const override;
	// 获取脚本工作区目录（工程文件内 workspace/ 在本地的缓存目录）
	virtual QString getScriptWorkspaceDir() const override;
	// 生成一个数据文件对应的临时文件位置
	QString makeDataTemporaryFilePath(const QString& dataName);
	// 把数据名称转换为zip文档中的相对路径位置
	static QString makeDataArchiveFilePath(const QString& dataName);
	// 设置插件
	void setPluginMgr(DAAppPluginManager* plugin);
public Q_SLOTS:
	// 清除工程
	virtual void clear() override;
	// 保存工程，保存成功后需要发射projectSaved信号
	virtual bool save(const QString& path) override;
	// 加载工程，加载完成后需要发射projectLoaded信号
	virtual bool load(const QString& path) override;
	// 请求保存,会弹出保存对话框让用户选择保存路径保存
	virtual bool requestSave() override;

protected:
	// 保存系统信息
	void makeSaveSystemInfoTask(DAZipArchiveThreadWrapper* archive);
	// 保存Python工作流逻辑数据的任务（节点拓扑+参数值+连接关系）
	void makeSaveWorkflowDataTask(DAZipArchiveThreadWrapper* archive);
	// 保存工作流的任务
	void makeSaveWorkFlowTask(DAZipArchiveThreadWrapper* archive);
	// 保存数据的任务
	void makeSaveDataManagerTask(DAZipArchiveThreadWrapper* archive);
	// 创建保存绘图的任务
	void makeSaveChartTask(DAZipArchiveThreadWrapper* archive);
	// 保存表格样式任务
	void makeSaveTableStyleTask(DAZipArchiveThreadWrapper* archive);
	// 保存数据操作窗口嵌套停靠区布局任务（已打开数据页列表 + dock 布局）
	void makeSaveDataOperateLayoutTask(DAZipArchiveThreadWrapper* archive);
	// 保存Agent会话任务（主线程收集活跃会话字节→子线程写 agent_sessions/<id>.jsonl）
	void makeSaveAgentSessionsTask(DAZipArchiveThreadWrapper* archive);
	// 保存脚本工作区任务（本地缓存目录打包回 zip 内 workspace/）
	void makeSaveWorkspaceTask(DAZipArchiveThreadWrapper* archive);
	// 保存workflow相关内容（以xml形式）
	QDomDocument createWorkflowUIDomDocument();
	// 保存charts相关内容（以xml形式）
	QDomDocument createChartsUIDomDocument(DAChartItemsManager& chartItems);
	bool loadWorkflowUI(const QByteArray& data);

private Q_SLOTS:
	void onBeginSave(const QString& path);
	void onBeginLoad(const QString& path);
	// 任务进度
	void onTaskProgress(std::shared_ptr< DA::DAAbstractArchiveTask > t, int mode);
	// 保存任务结束
	void onSaveFinish(bool success);
	// 保存任务结束
	void onLoadFinish(bool success);

private:
    bool executeSave(DAZipArchiveThreadWrapper* archive, const QString& path, bool* started = nullptr);
    bool executeLoad(DAZipArchiveThreadWrapper* archive, const QString& path, bool* started = nullptr, const QString& agentProjectPath = QString());
    bool createProjectSnapshot(QString* snapshotPath);
    bool restoreProjectSnapshot(const QString& snapshotPath, const QString& projectFilePath, bool isDirty);
	// 加载前预检脚本工作区冲突（本地缓存与工程内容指纹比对，可弹窗保留/覆盖/取消）
	// 返回 false 表示用户取消加载；结果经 localDir/keepLocal 输出，不直接修改成员
	// （避免快照打包阶段误用新工程的工作区目录）
	bool precheckWorkspaceOnLoad(const QString& projectPath, QString* localDir, bool* keepLocal);
	void loadedWorkflowInfo(const std::shared_ptr< DAAbstractArchiveTask >& t);
	// Python工作流逻辑数据加载回调
	void loadedWorkflowData(const std::shared_ptr< DAAbstractArchiveTask >& t);
	void loadedDataManager(const std::shared_ptr< DAAbstractArchiveTask >& t);
	void loadedChartsInfo(const std::shared_ptr< DAAbstractArchiveTask >& t);
	// 表格样式加载回调
	void loadedTableStyles(const std::shared_ptr< DAAbstractArchiveTask >& t);
	// 数据操作窗口布局加载回调（重建已打开数据页 + 恢复 dock 布局）
	void loadedDataOperateLayout(const std::shared_ptr< DAAbstractArchiveTask >& t);
	void setStatusBarInBusy(const QString& info = QString());
	void setStatusBarNotBusy(const QString& info = QString());
	void setCurrentStatusText(const QString& info);
	// 获取状态栏接口，带空指针保护
	DAStatusBarInterface* getStatusBar() const;

private:
	DAZipArchiveThreadWrapper* mArchive { nullptr };
	DAXmlHelper mXml;
	std::unique_ptr< QTemporaryDir > mTempDir;
	DAChartItemsManager mChartItemManager;
	DAAppPluginManager* mPluginMgr { nullptr };
	QString mWorkspaceLocalDir;             ///< 当前工程脚本工作区本地缓存目录
	bool mWorkspaceLocalKept { false };     ///< 加载时用户选择保留本地工作区（加载收尾后延迟置脏）
};

}  // namespace DA
#endif  // FCPROJECT_H
