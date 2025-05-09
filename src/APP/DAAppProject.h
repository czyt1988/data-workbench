#ifndef DAAPPPROJECT_H
#define DAAPPPROJECT_H
#include <QObject>
#include <QDomElement>
#include <QDomDocument>
#include <QTemporaryDir>
#include "DAProjectInterface.h"
#include "DAGlobals.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include <QThread>
#include "DAXmlHelper.h"
#include "DAZipArchiveThreadWrapper.h"

namespace DA
{
class DAAbstractArchiveTask;
class DAZipArchiveThreadWrapper;
class DAWorkFlowOperateWidget;
class DAWorkFlowGraphicsScene;
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
	~DAAppProject();
	// 工作流操作窗口
	DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const;
	// 追加一个工厂的工作流进入本工程中，注意这个操作不会清空当前的工作流
	bool appendWorkflowInProject(const QDomDocument& doc, bool skipIndex = false);
	bool appendWorkflowInProject(const QByteArray& data, bool skipIndex = false);
	// 在parent下，插入一个tag，tag下包含文字text
	static void appendElementWithText(QDomElement& parent, const QString& tagName, const QString& text, QDomDocument& doc);
	// 繁忙状态判断
	virtual bool isBusy() const override;
	// 生成一个数据文件对应的临时文件位置
	QString makeDataTemporaryFilePath(const QString& dataName);
	// 把数据名称转换为zip文档中的相对路径位置
	static QString makeDataArchiveFilePath(const QString& dataName);
public Q_SLOTS:
	// 清除工程
	virtual void clear() override;
	// 保存工程，保存成功后需要发射projectSaved信号
	virtual bool save(const QString& path) override;
	// 加载工程，加载完成后需要发射projectLoaded信号
	virtual bool load(const QString& path) override;

protected:
	// 保存工作流的任务
	void makeSaveWorkFlowTask(DAZipArchiveThreadWrapper* archive);
	// 保存数据的任务
	void makeSaveDataManagerTask(DAZipArchiveThreadWrapper* archive);
	// 保存绘图任务
	void makeSaveFiguresTask(DAZipArchiveThreadWrapper* archive);
	// 保存本地信息包括时间日期等等
	QDomElement makeLocalInfoElement(QDomDocument& doc) const;
	// 保存workflow相关内容（以xml形式）
	QDomDocument createWorkflowUIDomDocument();
	bool loadWorkflowUI(const QByteArray& data);

private Q_SLOTS:
	void onBeginSave(const QString& path);
	void onBeginLoad(const QString& path);
	// 任务进度
	void onTaskProgress(int total, int pos, const std::shared_ptr< DAAbstractArchiveTask >& t);
	// 保存任务结束
	void onSaveFinish(bool success);
	// 保存任务结束
	void onLoadFinish(bool success);

private:
	DAZipArchiveThreadWrapper* mArchive { nullptr };
	DAXmlHelper mXml;
	std::unique_ptr< QTemporaryDir > mTempDir;
};

}  // namespace DA
#endif  // FCPROJECT_H
