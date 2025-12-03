#ifndef DAUIEXTENDINTERFACE_H
#define DAUIEXTENDINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DABaseInterface.h"
namespace DA
{
class DAUIInterface;
class DACoreInterface;
/**
 * @brief DAAppUIInterface下面的扩展模块
 *
 *
 */
class DAINTERFACE_API DAUIExtendInterface : public DABaseInterface
{
    Q_OBJECT
public:
    DAUIExtendInterface(DAUIInterface* u);
    ~DAUIExtendInterface();
    // 获取DAAppUIInterface
    DAUIInterface* ui() const;
    // 发生语言变更时会调用此函数
    virtual void retranslateUi() = 0;

private:
    DAUIInterface* mUI { nullptr };
};
}  // namespace DA
#endif  // DAUIEXTENDINTERFACE_H
