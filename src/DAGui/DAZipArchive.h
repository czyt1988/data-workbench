#ifndef DAZIPARCHIVE_H
#define DAZIPARCHIVE_H
#include "DAGuiAPI.h"
#include "DAAbstractArchive.h"

namespace DA
{
/**
 * @brief zip档案
 */
class DAGUI_API DAZipArchive : public DAAbstractArchive
{
	DA_DECLARE_PRIVATE(DAZipArchive)
public:
    DAZipArchive(QObject* par = nullptr);
    DAZipArchive(const QString& zipPath, QObject* par = nullptr);
	// 设置zip文件名
	virtual void setBaseFilePath(const QString& path) override;
    // 设置zip文件名,包含完整路径的名字
    bool setZipFileName(const QString& fileName);
	// 打开
	bool open();
    // 是否打开
    bool isOpened() const;
    // 关闭
    bool close();
    // 写数据
    bool write(const QString& relatePath, const QByteArray& byte) override;

    // 读取数据
    QByteArray read(const QString& relatePath) override;

    //==============
    // 文件管理
    //==============
    // 列出所有文件
    QStringList getAllFiles() const;
    // 检查文件是否存在
    bool contains(const QString& relatePath) const;
    // 删除文件--注意，此函数非常耗时，轻易不要使用
    bool remove(const QString& fileToRemove) override;

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
};
}

#endif  // DAZIPARCHIVE_H
