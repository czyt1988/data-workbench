#ifndef DAABSTRACTARCHIVE_H
#define DAABSTRACTARCHIVE_H
#include "DAGuiAPI.h"
#include <QString>
#include <QByteArray>
#include <QObject>
namespace DA
{
class DAAbstractArchiveTask;
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
	/**
	 * @brief 设置基础路径
	 *
	 * 对于文件来说，基础路径就是要保存的目录，所有文件的写入都基于此目录，对应zip来说，这个路径就是zip文件路径
	 *
	 * @note 对于一些单文件类操作，例如zip文件，设置路径后需要打开文件的，应该继承此函数，把打开的操作也叫上，例如：
	 * @code
	 * @endcode
	 * @param path 基础路径
	 */
	virtual void setBaseFilePath(const QString& path);
	QString getBaseFilePath() const;

	/**
	 * @brief 写数据
	 * @param relatePath 相对位置此位置相对BaseFilePath
	 * @param byte
	 * @return
	 */
	virtual bool write(const QString& relatePath, const QByteArray& byte) = 0;

	/**
	 * @brief 读取数据
	 * @param relatePath 相对位置此位置相对BaseFilePath相对位置此位置相对BaseFilePath
	 * @return 读取失败返回一个空QByteArray
	 */
	virtual QByteArray read(const QString& relatePath) = 0;

    /**
     * @brief 删除文件
     * @param relatePath
     * @return
     */
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
     * @param total 总任务
     * @param pos 当前任务的位置
     */
    void taskProgress(int total, int pos, std::shared_ptr< DAAbstractArchiveTask > task);

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
