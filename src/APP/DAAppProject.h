#ifndef DAAPPPROJECT_H
#define DAAPPPROJECT_H
#include <QObject>
#include <QDomElement>
#include <QDomDocument>
#include "DAProjectInterface.h"
#include "DAGlobals.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include <QThread>
#include "DAAppArchive.h"
#include "DAXmlHelper.h"
namespace DA
{
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
    // 创建一个档案，这个档案会在另外一个线程中
    DAAppArchive* createArchive();
    // 追加一个工厂的工作流进入本工程中，注意这个操作不会清空当前的工作流
    bool appendWorkflowInProject(const QByteArray& data, bool skipIndex = false);
    // 在parent下，插入一个tag，tag下包含文字text
    static void appendElementWithText(QDomElement& parent, const QString& tagName, const QString& text, QDomDocument& doc);

public Q_SLOTS:
	// 清除工程
	virtual void clear() override;
    // 保存工程，保存成功后需要发射projectSaved信号
    virtual bool save(const QString& path);
    // 加载工程，加载完成后需要发射projectLoaded信号
    virtual bool load(const QString& path);

protected:
    // 保存本地信息包括时间日期等等
    void saveLocalInfo(QDomElement& root, QDomDocument& doc) const;
    // 保存workflow相关内容（以xml形式）
    QByteArray saveWorkflowUI();
    bool loadWorkflowUI(const QByteArray& data);
    // 保存dataManager结构
    QByteArray saveDataManager();
private Q_SLOTS:
    // 任务进度
    void taskProgress(int total, int pos);
    // 保存任务结束
    void saveTaskFinish(int code);

private:
    DAAppArchive* mArchive { nullptr };
    QThread* mThread { nullptr };
    DAXmlHelper mXml;
};
}  // namespace DA
#endif  // FCPROJECT_H
