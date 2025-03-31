#include "DAAbstractArchive.h"
namespace DA
{

//===============================================================
// DAAbstractArchive::PrivateData
//===============================================================
class DAAbstractArchive::PrivateData
{
	DA_DECLARE_PUBLIC(DAAbstractArchive)
public:
	PrivateData(DAAbstractArchive* p);

public:
	QString mBaseFilePath;
};

DAAbstractArchive::PrivateData::PrivateData(DAAbstractArchive* p) : q_ptr(p)
{
}

//===============================================================
// DAAbstractArchive
//===============================================================
DAAbstractArchive::DAAbstractArchive(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

DAAbstractArchive::~DAAbstractArchive()
{
}

void DAAbstractArchive::setBaseFilePath(const QString& path)
{
	d_ptr->mBaseFilePath = path;
}

QString DAAbstractArchive::getBaseFilePath() const
{
	return d_ptr->mBaseFilePath;
}

}
