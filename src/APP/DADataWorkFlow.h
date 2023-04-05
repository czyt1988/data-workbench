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
 */
class DADataWorkFlow : public DAWorkFlow
{
    Q_OBJECT
public:
    DADataWorkFlow(QObject* p = nullptr);
};
}  // namespace DA
#endif  // FCMETHODEDITORNODEFACTORY_H
