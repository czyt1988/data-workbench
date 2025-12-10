#include "DACallBackInterface.h"
namespace DA
{

DACallBackInterface::DACallBackInterface()
{
}

DACallBackInterface::~DACallBackInterface()
{
}

void DACallBackInterface::setCallBack(DACallBackInterface::CallBack fn)
{
    m_callback = fn;
}

void DACallBackInterface::callback()
{
    if (m_callback) {
        m_callback();
    }
}

bool DACallBackInterface::hasCallback() const
{
    return m_callback != nullptr;
}

DACallBackInterface::CallBack DACallBackInterface::getCallBack() const
{
    return m_callback;
}

}
