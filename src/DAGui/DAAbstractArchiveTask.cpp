#include "DAAbstractArchiveTask.h"
#include <QMetaType>

namespace DA
{
DAAbstractArchiveTask::DAAbstractArchiveTask()
{
}

DAAbstractArchiveTask::~DAAbstractArchiveTask()
{
}

int DAAbstractArchiveTask::getCode() const
{
	return mCode;
}

void DAAbstractArchiveTask::setCode(int code)
{
	mCode = code;
}

QString DAAbstractArchiveTask::getName() const
{
	return mName;
}

void DAAbstractArchiveTask::setName(const QString& name)
{
	mName = name;
}

QString DAAbstractArchiveTask::getDescribe() const
{
	return mDescribe;
}

void DAAbstractArchiveTask::setDescribe(const QString& describe)
{
	mDescribe = describe;
}

}  // end DA
DA_AUTO_REGISTER_META_TYPE(DA::DAAbstractArchiveTask::Mode)
DA_AUTO_REGISTER_META_TYPE(std::shared_ptr< DA::DAAbstractArchiveTask >)
