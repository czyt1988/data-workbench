#ifndef DAUTILITYNODEAPPEXECUTE_H
#define DAUTILITYNODEAPPEXECUTE_H
#include "DAUtilityNodePluginAPI.h"
#include "DAAbstractNode.h"
// Qt
#include <QStringList>

namespace DA
{
DA_IMPL_FORWARD_DECL(DAUtilityNodeAppExecute)
/**
 * @brief 应用执行器
 */
class DAUtilityNodeAppExecute : public DAAbstractNode
{
    DA_IMPL(DAUtilityNodeAppExecute)
public:
    DAUtilityNodeAppExecute();
    ~DAUtilityNodeAppExecute();

public:
    //执行
    virtual bool exec() override;
    //创建图元
    virtual DAAbstractNodeGraphicsItem* createGraphicsItem() override;

public:
    //设置可执行程序路径
    void setProgram(const QString& p);
    QString getProgram() const;
    //设置参数
    void setArgs(const QStringList& a);
    QStringList getArgs() const;
    //设置开始等待的超时时间，默认是10000
    void setWaitStartTimeout(int ms);
    int getWaitStartTimeout() const;
};

}

#endif  // DAUTILITYNODEAPPEXECUTE_H
