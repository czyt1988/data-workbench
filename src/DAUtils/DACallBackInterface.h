#ifndef DACALLBACKINTERFACE_H
#define DACALLBACKINTERFACE_H
#include <functional>
#include "DAUtilsAPI.h"
namespace DA
{

class DAUTILS_API DACallBackInterface
{
public:
    using CallBack = std::function< void() >;
    DACallBackInterface();
    virtual ~DACallBackInterface();
    void setCallBack(CallBack fn);
    CallBack getCallBack() const;
    void callback();
    // 是否有回调
    bool hasCallback() const;

private:
    CallBack m_callback;
};
}

#endif  // DACALLBACKINTERFACE_H
