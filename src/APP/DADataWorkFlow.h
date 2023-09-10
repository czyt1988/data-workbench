#ifndef DAMETHODEDITORWORKFLOW_H
#define DAMETHODEDITORWORKFLOW_H
#include <QtCore/qglobal.h>
#include <QHash>
#include <QPointer>
#include "DAWorkFlow.h"
#include "DAAbstractNodeGraphicsItem.h"
namespace DA
{
/**
 * @brief 创建节点命令
 *
 * 这里所有工作流是共享一批工厂
 */
class DADataWorkFlow : public DAWorkFlow
{
    Q_OBJECT
public:
    DADataWorkFlow(QObject* p = nullptr);
};
}  // namespace DA
#endif  // FCMETHODEDITORNODEFACTORY_H
