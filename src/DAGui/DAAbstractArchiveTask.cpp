#include "DAAbstractArchiveTask.h"
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
}
