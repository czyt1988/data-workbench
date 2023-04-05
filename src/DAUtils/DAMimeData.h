#ifndef FCMIMEDATA_H
#define FCMIMEDATA_H
#include <QtCore/qglobal.h>
#include <QMimeData>
#include "DAGlobals.h"
#include "DAUtilsAPI.h"
namespace DA
{
/**
 * @brief mime的基类
 */
class DAUTILS_API DAMimeData : public QMimeData
{
    Q_OBJECT
public:
    DAMimeData();
};
}  // namespace DA
#endif  // DAMIMEDATA_H
