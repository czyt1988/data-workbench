#ifndef DAABSTRACTARCHIVETASK_H
#define DAABSTRACTARCHIVETASK_H
#include "DAGuiAPI.h"
#include <QObject>
#include <functional>
namespace DA
{
class DAAbstractArchive;
/**
 * @brief 针对@sa DAZipArchiveThreadWrapper 的任务
 *
 * @sa DAZipArchiveThreadWrapper 内部有个任务队列，在保存和加载需要推入任务，具体执行时会逐个任务进行执行，
 * 有特殊的读取和写入需求时，只要派生一个特殊的任务即可
 *
 * @note 注意任务的执行是在别的线程中，不要在任务中操作ui
 */
class DAGUI_API DAAbstractArchiveTask : std::enable_shared_from_this< DAAbstractArchiveTask >
{
public:
	using FpLoadedCallBack = std::function< void(std::shared_ptr< DAAbstractArchiveTask >) >;
public:
	/**
	 * @brief 模式用来区分读写
	 */
	enum Mode
	{
		ReadMode,
		WriteMode
	};
	DAAbstractArchiveTask();
	virtual ~DAAbstractArchiveTask();

	// 执行任务
	virtual bool exec(DAAbstractArchive* archive, Mode mode) = 0;

	// 获取流水号
	int getCode() const;

	// 设置流水号
	void setCode(int code);

	// 任务名称
	QString getName() const;
	void setName(const QString& name);

	// 任务描述
	QString getDescribe() const;
	void setDescribe(const QString& describe);

	void setLoadedCallBack(const FpLoadedCallBack& callBack);
	FpLoadedCallBack getLoadedCallBack() const;
private:
	int mCode { 0 };
	QString mName;
	QString mDescribe;
	FpLoadedCallBack mLoadedCallBack;
};
}  // end DA
Q_DECLARE_METATYPE(DA::DAAbstractArchiveTask::Mode)
Q_DECLARE_METATYPE(std::shared_ptr< DA::DAAbstractArchiveTask >)
#endif  // DAABSTRACTARCHIVETASK_H
