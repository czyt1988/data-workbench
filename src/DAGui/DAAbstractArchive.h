#ifndef DAABSTRACTARCHIVE_H
#define DAABSTRACTARCHIVE_H
#include "DAGuiAPI.h"
#include <QString>
#include <QByteArray>
#include <QObject>
namespace DA
{
/**
 * @brief 持久化的基类
 */
class DAGUI_API DAAbstractArchive : public QObject
{
    Q_OBJECT
	DA_DECLARE_PRIVATE(DAAbstractArchive)
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
};
}

#endif  // DAABSTRACTARCHIVE_H
