#ifndef DAAPPUIEXTENDINTERFACE_H
#define DAAPPUIEXTENDINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DABaseInterface.h"
namespace DA
{
class DAAppUIInterface;
class DACoreInterface;
/**
 * @brief DAAppUIInterface下面的扩展模块
 *
 *
 */
class DAINTERFACE_API DAAppUIExtendInterface : public DABaseInterface
{
    Q_OBJECT
public:
    DAAppUIExtendInterface(DAAppUIInterface* u);
    ~DAAppUIExtendInterface();
    //获取DAAppUIInterface
    DAAppUIInterface* ui() const;
    //发生语言变更时会调用此函数
    virtual void retranslateUi() = 0;
};
}  // namespace DA
#endif  // DAAPPUIEXTENDINTERFACE_H
