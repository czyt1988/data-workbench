#ifndef DAABSTRACTARCHIVE_H
#define DAABSTRACTARCHIVE_H
#include "DAGuiAPI.h"
#include <QString>
#include <QByteArray>
#include <QObject>
#include "DAAbstractArchiveTask.h"
namespace DA
{
/**
 * @brief 持久化的基类
 *
 * 这个类设计中包含了一个任务队列，任务队列可以实现多线程执行批量写和读
 */
class DAGUI_API DAAbstractArchive : public QObject
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAAbstractArchive)
public:
	/**
	 * @brief 结果号
	 */
	enum ResultCode
	{
		SaveSuccess,
		SaveFailed,
		LoadSuccess,
		LoadFailed
	};

public:
	DAAbstractArchive(QObject* par = nullptr);
	virtual ~DAAbstractArchive();
	// 设置基础路径（对于文件为保存目录，对于zip为zip文件路径）
	virtual bool setBaseFilePath(const QString& path);
	QString getBaseFilePath() const;

	// 写数据
	virtual bool write(const QString& relatePath, const QByteArray& byte) = 0;

	// 读取数据
	virtual QByteArray read(const QString& relatePath) = 0;

	// 删除文件
	virtual bool remove(const QString& relatePath) = 0;

	// 添加任务
	void appendTask(const std::shared_ptr< DAAbstractArchiveTask >& task);
	// 获取任务数量
	int getTaskCount() const;
	// 是否有任务
	bool isTaskQueueEmpty() const;

public Q_SLOTS:
	// 保存所有，执行任务队列
	virtual void saveAll(const QString& filePath) = 0;
	// 读取所有，执行任务队列，内个任务的读取结果通过taskProgress信号携带
	virtual void loadAll(const QString& filePath) = 0;

public:
	// 转换为临时路径
	static QString toTemporaryPath(const QString& path);
	// 替换文件
	static bool replaceFile(const QString& file, const QString& beReplaceFile);

Q_SIGNALS:
	/**
	 * @brief 当前进度信号
	 * 对于读取操作，这个函数会携带读取的结果
	 * @param task 任务
	 * @param mode 模式，具体为DAAbstractArchiveTask::Mode
	 */
	void taskProgress(std::shared_ptr< DA::DAAbstractArchiveTask > task, int mode);

	/**
	 * @brief 任务完成
	 */
	void taskFinished(int resultCode);

protected:
	// 从顶部提取一个任务
	std::shared_ptr< DAAbstractArchiveTask > takeTask();
};
}

#endif  // DAABSTRACTARCHIVE_H
