#ifndef DABASEINTERFACE_H
#define DABASEINTERFACE_H
#include <QObject>
#include "DAInterfaceAPI.h"
namespace DA
{
class DACoreInterface;
class DAINTERFACE_API DABaseInterface : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DABaseInterface)
public:
    DABaseInterface(DACoreInterface* c, QObject* par = nullptr);
    ~DABaseInterface();
    //返回核心接口指针
    DACoreInterface* core() const;
};
}  // namespace DA
#endif  // DABASEINTERFACE_H
