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
	DAZipArchive();
	DAZipArchive(const QString& zipPath);
	// 设置zip文件名
	virtual void setBaseFilePath(const QString& path) override;
	// 设置zip文件名
	void setZipFileName(const QString& fileName);
	// 打开
	bool open();
};
}

#endif  // DAZIPARCHIVE_H
