#include "DAZipArchiveThreadWrapper.h"
#include <QThread>
#include <QDebug>
#include "DAZipArchive.h"
#include "DAZipArchiveTask_ByteArray.h"
#include "DAZipArchiveTask_Xml.h"
#include "DAZipArchiveTask_ArchiveFile.h"
#include "DAZipArchiveTask_ChartItem.h"
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
	bool mIsBusy { false };
};

DAZipArchiveThreadWrapper::PrivateData::PrivateData(DAZipArchiveThreadWrapper* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DAZipArchiveThreadWrapper
//----------------------------------------------------
DAZipArchiveThreadWrapper::DAZipArchiveThreadWrapper(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
	init();
}

DAZipArchiveThreadWrapper::~DAZipArchiveThreadWrapper()
{
	if (d_ptr->mThread) {
		// 退出时先把线程安全退出
		d_ptr->mThread->quit();
		d_ptr->mThread->wait();
	}
}

void DAZipArchiveThreadWrapper::init()
{
    // qRegisterMetaType< std::shared_ptr< DA::DAAbstractArchiveTask > >("std::shared_ptr<DA::DAAbstractArchiveTask>");
    //   创建线程
	QThread* thread       = new QThread();
	DAZipArchive* archive = new DAZipArchive();
	// 绑定
	archive->moveToThread(thread);
	// 任务执行完结束线程
	connect(archive, &DAAbstractArchive::taskFinished, this, &DAZipArchiveThreadWrapper::onTaskFinish);
	connect(archive, &DAAbstractArchive::taskProgress, this, &DAZipArchiveThreadWrapper::taskProgress);

	// 信号绑定触发saveall
	connect(this, &DAZipArchiveThreadWrapper::beginSave, archive, &DAAbstractArchive::saveAll);
	connect(this, &DAZipArchiveThreadWrapper::beginLoad, archive, &DAAbstractArchive::loadAll);

	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(thread, &QThread::finished, archive, &DAAbstractArchive::deleteLater);
	thread->start();

	d_ptr->mArchive = archive;
	d_ptr->mThread  = thread;
}

bool DAZipArchiveThreadWrapper::isBusy() const
{
	return d_ptr->mIsBusy;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendByteSaveTask(const QString& zipRelatePath,
                                                                                       const QByteArray& data)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
    auto t = std::make_shared< DAZipArchiveTask_ByteArray >(zipRelatePath, data);
    d_ptr->mArchive->appendTask(t);
    return t;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendXmlSaveTask(const QString& zipRelatePath,
                                                                                      const QDomDocument& data)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
    auto t = std::make_shared< DAZipArchiveTask_Xml >(zipRelatePath, data);
    d_ptr->mArchive->appendTask(t);
    return t;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendFileSaveTask(const QString& zipRelatePath,
                                                                                       const QString& localFilePath)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
    auto t = std::make_shared< DAZipArchiveTask_ArchiveFile >(zipRelatePath, localFilePath);
    d_ptr->mArchive->appendTask(t);
    return t;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendChartItemSaveTask(const QString& zipRelateFolderPath,
                                                                                            DAChartItemsManager chartItemMgr)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
    auto t = std::make_shared< DAZipArchiveTask_ChartItem >(zipRelateFolderPath, chartItemMgr);
    d_ptr->mArchive->appendTask(t);
    return t;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendByteLoadTask(const QString& zipRelatePath, int code)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
	std::shared_ptr< DAZipArchiveTask_ByteArray > task = std::make_shared< DAZipArchiveTask_ByteArray >(zipRelatePath);
	task->setCode(code);
	d_ptr->mArchive->appendTask(task);
    return task;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendXmlLoadTask(const QString& zipRelatePath, int code)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
	std::shared_ptr< DAZipArchiveTask_Xml > task = std::make_shared< DAZipArchiveTask_Xml >(zipRelatePath);
	task->setCode(code);
	d_ptr->mArchive->appendTask(task);
    return task;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendFileLoadTask(const QString& zipRelatePath, int code)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
	std::shared_ptr< DAZipArchiveTask_ArchiveFile > task = std::make_shared< DAZipArchiveTask_ArchiveFile >(zipRelatePath);
	task->setCode(code);
	d_ptr->mArchive->appendTask(task);
    return task;
}

std::shared_ptr< DAAbstractArchiveTask > DAZipArchiveThreadWrapper::appendChartItemLoadTask(const QString& zipRelateFolderPath,
                                                                                            int code)
{
	if (isBusy()) {
        return std::shared_ptr< DAAbstractArchiveTask >();
	}
	std::shared_ptr< DAZipArchiveTask_ChartItem > task = std::make_shared< DAZipArchiveTask_ChartItem >(zipRelateFolderPath);
	task->setCode(code);
	d_ptr->mArchive->appendTask(task);
    return task;
}

bool DAZipArchiveThreadWrapper::appendTask(const std::shared_ptr< DAAbstractArchiveTask >& task)
{
	if (isBusy()) {
		return false;
	}
	d_ptr->mArchive->appendTask(task);
	return true;
}

bool DAZipArchiveThreadWrapper::save(const QString& filePath)
{
	if (isBusy()) {
		return false;
	}
	d_ptr->mIsBusy = true;
	// 发射信号，把消息通知到线程中的DAZipArchive
	Q_EMIT beginSave(filePath);
	return true;
}

bool DAZipArchiveThreadWrapper::load(const QString& filePath)
{
	if (isBusy()) {
		return false;
	}
	d_ptr->mIsBusy = true;
	// 发射信号，把消息通知到线程中的DAZipArchive
	Q_EMIT beginLoad(filePath);
	return true;
}

void DAZipArchiveThreadWrapper::onTaskFinish(int code)
{
	d_ptr->mIsBusy = false;
	switch (code) {
	case DAAbstractArchive::SaveFailed:
		Q_EMIT saved(false);
		break;
	case DAAbstractArchive::SaveSuccess:
		Q_EMIT saved(true);
		break;
	case DAAbstractArchive::LoadFailed:
		Q_EMIT loaded(false);
		break;
	case DAAbstractArchive::LoadSuccess:
		Q_EMIT loaded(true);
		break;
	default:
		break;
	}
}

}  // end DA
