#ifndef DAZIPARCHIVE_H
#define DAZIPARCHIVE_H
#include "DAGuiAPI.h"
#include "DAAbstractArchive.h"
class QuaZip;
namespace DA
{
/**
 * @brief zip档案
 */
class DAGUI_API DAZipArchive : public DAAbstractArchive
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAZipArchive)
public:
	DAZipArchive(QObject* par = nullptr);
	DAZipArchive(const QString& zipPath, QObject* par = nullptr);
	~DAZipArchive();

	// 设置zip文件名
	virtual bool setBaseFilePath(const QString& path) override;
	// 设置zip文件名,包含完整路径的名字
	bool setZipFileName(const QString& fileName);
	// 打开
	bool open();
	bool create();
	// 是否打开
	bool isOpened() const;
	// 关闭
	bool close();
	// 写数据
	bool write(const QString& relatePath, const QByteArray& byte) override;
	// 将本地文件写入ZIP压缩包中的指定路径
	bool writeFileToZip(const QString& relatePath, const QString& localFilePath, std::size_t chunk_mb = 4);
	// 读取数据
	QByteArray read(const QString& relatePath) override;
	// 从 ZIP 归档中提取指定文件到本地路径
	bool readToFile(const QString& zipRelatePath, const QString& localFilePath, std::size_t chunk_mb = 4);
	// 删除文件--注意，此函数非常耗时，轻易不要使用
	bool remove(const QString& fileToRemove) override;

	//==============
	// 文件管理
	//==============
	// 列出所有文件
	QStringList getAllFiles() const;
	// 检查文件是否存在
	bool contains(const QString& relatePath) const;

	//==============
	// 归档信息
	//==============
	// 获取 ZIP 文件大小
	qint64 getFileSize() const;
	// 获取文件数量
	int getFileCount() const;
	// 获取注释
	QString getComment() const;
	// 设置注释
	void setComment(const QString& comment);

	//==============
	// 整体解压到目录下
	//==============
	// 把zip整体解压到目录下
	bool extractToDirectory(const QString& extractDir);
	// 把一个zip整体压缩到压缩包
	bool compressDirectory(const QString& folderPath);
	// 获取文件列表
	QStringList getFileNameList() const;
	// 获取zip中一个文件夹下的所有文件列表，注意不包括这个文件下的子目录及子目录下的文件
	QStringList getFolderFileNameList(const QString& zipFolderPath) const;
	// 获取QuaZip指针
	QuaZip* quazip() const;
    // 获取最后的错误内容
    QString getLastErrorString() const;
public Q_SLOTS:
	// 保存所有，执行任务队列
	virtual void saveAll(const QString& filePath) override;
	// 读取所有，执行任务队列，内个任务的读取结果通过taskProgress信号携带
	virtual void loadAll(const QString& filePath) override;

public:
    // 判断是否是正确的工程
    static bool isCorrectFile(const QString& filePath);
	static bool extractToDirectory(const QString& zipFilePath, const QString& extractDir);
	static bool extractToDirectory(QuaZip* zip, const QString& extractDir);
	static bool compressDirectory(const QString& folderPath, const QString& zipFilePath);
	static bool compressDirectory(const QString& folderPath, QuaZip* zip, const QString& relativeBase = QString("./"));
	static bool writeFileToZip(QuaZip* zip, const QString& relatePath, const QString& localFilePath, std::size_t chunk_mb = 4);
	static bool readToFile(QuaZip* zip, const QString& zipRelatePath, const QString& localFilePath, std::size_t chunk_mb = 4);
};
}

#endif  // DAZIPARCHIVE_H
