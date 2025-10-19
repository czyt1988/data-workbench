#ifndef DAZIPARCHIVETHREADWRAPPER_H
#define DAZIPARCHIVETHREADWRAPPER_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QString>
#include <QDomDocument>
#include "DAAbstractArchiveTask.h"
#include "DAChartItemsManager.h"
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
	std::shared_ptr< DAAbstractArchiveTask > appendByteSaveTask(const QString& zipRelatePath, const QByteArray& data);
	std::shared_ptr< DAAbstractArchiveTask > appendXmlSaveTask(const QString& zipRelatePath, const QDomDocument& data);
	std::shared_ptr< DAAbstractArchiveTask > appendFileSaveTask(const QString& zipRelatePath, const QString& localFilePath);
	std::shared_ptr< DAAbstractArchiveTask > appendChartItemSaveTask(const QString& zipRelateFolderPath,
																	 DAChartItemsManager chartItemMgr);
	// 读取任务
	std::shared_ptr< DAAbstractArchiveTask > appendByteLoadTask(const QString& zipRelatePath, int code);
	std::shared_ptr< DAAbstractArchiveTask > appendXmlLoadTask(const QString& zipRelatePath, int code);
	std::shared_ptr< DAAbstractArchiveTask > appendFileLoadTask(const QString& zipRelatePath, int code);
	std::shared_ptr< DAAbstractArchiveTask > appendChartItemLoadTask(const QString& zipRelateFolderPath, int code);
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
	void taskProgress(std::shared_ptr< DA::DAAbstractArchiveTask > task, int mode);

private:
	void init();
};
}  // end DA

#endif  // DAZIPARCHIVETHREADWRAPPER_H
