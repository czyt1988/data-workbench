#ifndef DAZIPARCHIVETHREADWRAPPER_H
#define DAZIPARCHIVETHREADWRAPPER_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QString>
#include <QDomDocument>
#include "DAAbstractArchiveTask.h"

namespace DA
{

/**
 * @brief DAZipArchive的多线程封装，此类内部维护着一个线程，封装了@sa DAZipArchive 的操作
 */
class DAGUI_API DAZipArchiveThreadWrapper : public QObject
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAZipArchiveThreadWrapper)
public:
	DAZipArchiveThreadWrapper(QObject* par = nullptr);
	~DAZipArchiveThreadWrapper();
	// 是否繁忙
	bool isBusy() const;
	// 保存任务
	bool appendByteSaveTask(const QString& zipRelatePath, const QByteArray& data);
	bool appendXmlSaveTask(const QString& zipRelatePath, const QDomDocument& data);
	bool appendFileSaveTask(const QString& zipRelatePath, const QString& localFilePath);
	// 读取任务
	bool appendByteLoadTask(const QString& zipRelatePath, int code);
	bool appendXmlLoadTask(const QString& zipRelatePath, int code);
	bool appendFileLoadTask(const QString& zipRelatePath, int code);
	// 添加任务
	bool appendTask(const std::shared_ptr< DAAbstractArchiveTask >& task);
public Q_SLOTS:
	bool save(const QString& filePath);
	bool load(const QString& filePath);
private Q_SLOTS:
	void onTaskFinish(int code);
Q_SIGNALS:
	/**
	 * @brief 开始保存信号
	 * @param path
	 */
	void beginSave(const QString& path);

	/**
	 * @brief 开始加载信号
	 * @param path
	 */
	void beginLoad(const QString& path);

	/**
	 * @brief 保存结束信号
	 * @param success
	 */
	void saved(bool success);

	/**
	 * @brief 加载结束信号
	 * @param success
	 */
	void loaded(bool success);

	/**
	 * @brief 当前进度信号
	 * 对于读取操作，这个函数会携带读取的结果
	 * @param total 总任务
	 * @param pos 当前任务的位置
	 */
	void taskProgress(int total, int pos, std::shared_ptr< DAAbstractArchiveTask > task);

private:
	void init();
};

}  // end DA
#endif  // DAZIPARCHIVETHREADWRAPPER_H
