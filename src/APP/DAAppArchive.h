#ifndef DAAPPARCHIVE_H
#define DAAPPARCHIVE_H
#include <QString>
#include <deque>
#include <functional>
#include "DAZipArchive.h"
namespace DA
{

class DAAppArchive : public DAZipArchive
{
	Q_OBJECT
public:
	/**
	 * @brief 定义了一个任务
	 */
	class Task
	{
	public:
		using FunPtr = std::function< void(DAAppArchive*, Task&) >;
		enum Mode
		{
			ReadMode,
			WriteMode
		};

	public:
		Task();
		// 写入任务
		Task(const QString& path, const QByteArray& d, const QString& des = QString(), Mode m = WriteMode);
		// 读取任务，读取的结果一般会通过data携带
		Task(const QString& path, const QString& des = QString(), Mode m = ReadMode);
		// 带函数指针回调的任务，可读可写
		Task(const QString& path, const FunPtr& fp, Mode m, const QString& des = QString());
		~Task();
		bool isRead() const;
		bool isWrite() const;

	public:
		QString relatePath;  ///< 相对路径，这个路径将会写入zip文件中
		QByteArray data;  ///< 数据，如果这个不为空，优先把这个数据写入zip中，zip的位置就是relatePath
		QString describe;  ///< 描述，这个描述将会在这个任务完成时，通过taskProgress的info参数带出
		/**
		 * @brief 对于一些特殊的，例如数据文件的保存，需要调用函数，则使用此回调
		 *
		 * 注意，此函数指针将会在DAAppArchive里执行，而DAAppArchive是在线程中执行，因此，不要在此函数指针中操作界面相关的内容
		 * 同时要保证线程安全性
		 */
		FunPtr function;
		Mode mode;  ///< 模式用来表征这个任务是读取还是写入
	};

	enum Code
	{
		SaveSuccess,
		SaveFailed,
		LoadSuccess,
		LoadFailed
	};

public:
	DAAppArchive(QObject* par = nullptr);
	DAAppArchive(const QString& zipPath, QObject* par = nullptr);
	~DAAppArchive();
	// 添加一个任务
	void appendTask(const Task& t);
	// 拿出一个任务，任务从第一个
	Task takeTask();
	// 是否没有任务
	bool isTaskEmpty() const;
	// 获取任务数量
	int getTaskCount() const;
	// 转换为临时路径
	static QString toTemporaryPath(const QString& path);
	// 替换文件
	static bool replaceFile(const QString& file, const QString& beReplaceFile);
public Q_SLOTS:
	// 保存所有，执行任务队列
	void saveAll(const QString& filePath);
	// 读取所有，执行任务队列，内个任务的读取结果通过taskProgress信号携带
	void loadAll(const QString& filePath);
Q_SIGNALS:
	/**
	 * @brief 当前进度信号
	 * 对于读取操作，这个函数会携带读取的结果
	 * @param total 总任务
	 * @param pos 当前任务的位置
	 */
	void taskProgress(int total, int pos, const Task& task);

	/**
	 * @brief 任务完成
	 */
	void taskFinished(int code);

protected:
	// 执行一个任务
	bool execTask(Task& t);

private:
	std::deque< Task > mTaskQueue;
};

}

#endif  // DAAPPARCHIVE_H
