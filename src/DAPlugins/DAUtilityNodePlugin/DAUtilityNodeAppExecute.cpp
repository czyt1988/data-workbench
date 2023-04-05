#include "DAUtilityNodeAppExecute.h"
#include "DAUtilityNodeAppExecuteGraphicsItem.h"
#include <QProcess>
#include <memory>
#include <QApplication>
#include "DAUtilityNodeFactory.h"

namespace DA
{
//===================================================
// DAUtilityNodeAppExecutePrivate
//===================================================
class DAUtilityNodeAppExecutePrivate
{
    DA_IMPL_PUBLIC(DAUtilityNodeAppExecute)
public:
    DAUtilityNodeAppExecutePrivate(DAUtilityNodeAppExecute* p);
    std::unique_ptr< QProcess > _process;
};

DAUtilityNodeAppExecutePrivate::DAUtilityNodeAppExecutePrivate(DAUtilityNodeAppExecute* p) : q_ptr(p)
{
}

//===================================================
// DAUtilityNodeAppExecute
//===================================================

DAUtilityNodeAppExecute::DAUtilityNodeAppExecute() : DAAbstractNode(), d_ptr(new DAUtilityNodeAppExecutePrivate(this))
{
    addInputKey("input");
    addOutputKey("output");
    metaData().setNodePrototype(DAUTILITYNODEPLUGIN_NODEPROTOTYPE(AppExecute));
    metaData().setNodeName(QObject::tr("App Execute"));
    metaData().setGroup(QObject::tr("App"));
    setProperty("_waitStartTimeout", 10000);
}

DAUtilityNodeAppExecute::~DAUtilityNodeAppExecute()
{
}

bool DAUtilityNodeAppExecute::exec()
{
    QString ep       = getProgram();
    QStringList args = getArgs();
    if (ep.isEmpty()) {
        return false;
    }
    qDebug() << ep << " " << args;
    int ws = getWaitStartTimeout();
    d_ptr->_process.reset(new QProcess());
    d_ptr->_process->setProgram(ep);
    d_ptr->_process->setArguments(args);
    d_ptr->_process->start();
    if (!d_ptr->_process->waitForStarted(ws)) {
        //超时没启动
        return false;
    }
    return true;
}

DAAbstractNodeGraphicsItem* DAUtilityNodeAppExecute::createGraphicsItem()
{
    return new DAUtilityNodeAppExecuteGraphicsItem(this);
}

/**
 * @brief 设置可执行程序路径
 * @param p
 */
void DAUtilityNodeAppExecute::setProgram(const QString& p)
{
    setProperty("_executePath", p);
}

/**
 * @brief 获取可执行程序路径
 * @return
 */
QString DAUtilityNodeAppExecute::getProgram() const
{
    return getProperty("_executePath", QString()).toString();
}

/**
 * @brief 设置参数
 * @param a
 */
void DAUtilityNodeAppExecute::setArgs(const QStringList& a)
{
    setProperty("_args", a);
}

/**
 * @brief 返回参数
 * @return
 */
QStringList DAUtilityNodeAppExecute::getArgs() const
{
    return getProperty("_args").toStringList();
}

/**
 * @brief 设置开始等待的超时时间，默认是10000
 * @param ms
 */
void DAUtilityNodeAppExecute::setWaitStartTimeout(int ms)
{
    setProperty("_waitStartTimeout", ms);
}
/**
 * @brief 等待超时时间
 * @return
 */
int DAUtilityNodeAppExecute::getWaitStartTimeout() const
{
    return getProperty("_waitStartTimeout", 10000).toInt();
}

}
