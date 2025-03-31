#include "DAZipArchive.h"
#include "quazip/quazipfile.h"
namespace DA
{
//===============================================================
// DAZipArchive::PrivateData
//===============================================================
class DAZipArchive::PrivateData
{
	DA_DECLARE_PUBLIC(DAZipArchive)
public:
	PrivateData(DAZipArchive* p);

public:
	QuaZipFile mZipFile;
};

DAZipArchive::PrivateData::PrivateData(DAZipArchive* p) : q_ptr(p)
{
}

//===============================================================
// DAZipArchive
//===============================================================
DAZipArchive::DAZipArchive() : DAAbstractArchive(), DA_PIMPL_CONSTRUCT
{
}

DAZipArchive::DAZipArchive(const QString& zipPath) : DAAbstractArchive(), DA_PIMPL_CONSTRUCT
{
	setZipFileName(zipPath);
}

void DAZipArchive::setBaseFilePath(const QString& path)
{
	DAAbstractArchive::setBaseFilePath(path);
	setZipFileName(path);
}

void DAZipArchive::setZipFileName(const QString& fileName)
{
	d_ptr->mZipFile.setFileName(fileName);
}

bool DAZipArchive::open()
{
	return d_ptr->mZipFile.open(QIODevice::ReadWrite);
}
}  // end DA
