#include "DAZipArchiveThreadWrapper.h"
#include "DAZipArchive.h"
#include <QThread>

#include <QDebug>

namespace DA
{
class DAZipArchiveThreadWrapper::PrivateData
{
    DA_DECLARE_PUBLIC(DAZipArchiveThreadWrapper)
public:
    PrivateData(DAZipArchiveThreadWrapper* p);

public:
    DAZipArchive* mArchive { nullptr };
    QThread* mThread { nullptr };
};

DAZipArchiveThreadWrapper::PrivateData::PrivateData(DAZipArchiveThreadWrapper* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DAZipArchiveThreadWrapper
//----------------------------------------------------
DAZipArchiveThreadWrapper::DAZipArchiveThreadWrapper(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    // 创建线程
    QThread* thread       = new QThread();
    DAZipArchive* archive = new DAZipArchive();
}

DAZipArchiveThreadWrapper::~DAZipArchiveThreadWrapper()
{
}

void DAZipArchiveThreadWrapper::save(const QString& filePath)
{
}

void DAZipArchiveThreadWrapper::load(const QString& filePath)
{
}

}  // end DA
