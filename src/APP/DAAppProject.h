#ifndef DAAPPPROJECT_H
#define DAAPPPROJECT_H
#include <QObject>
#include "DAProjectInterface.h"
#include "DAGlobals.h"
#include "DAAbstractNodeLinkGraphicsItem.h"

namespace DA
{
class DAWorkFlowOperateWidget;
class DAWorkFlowGraphicsScene;

/**
 * @brief 负责整个节点的工程管理
 */
class DAAppProject : public DAProjectInterface
{
    Q_OBJECT
public:
    DAAppProject(DACoreInterface* c, QObject* p = nullptr);
    ~DAAppProject();
    //清除工程
    virtual void clear() override;

public:
    //获取工程文件的后缀
    static QString getProjectFileSuffix();
};
}  // namespace DA
#endif  // FCPROJECT_H
