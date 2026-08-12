#include "DACallBackInterface.h"
namespace DA
{

/**
 * @brief 构造函数
 */
DACallBackInterface::DACallBackInterface()
{
}

/**
 * @brief 析构函数
 */
DACallBackInterface::~DACallBackInterface()
{
}

/**
 * @brief 设置回调函数
 * @param fn 回调函数对象
 */
void DACallBackInterface::setCallBack(DACallBackInterface::CallBack fn)
{
    mCallback = fn;
}

/**
 * @brief 执行回调
 */
void DACallBackInterface::callback()
{
    if (mCallback) {
        mCallback();
    }
}

/**
 * @brief 判断是否设置了回调
 * @return 如果设置了回调返回true，否则返回false
 */
bool DACallBackInterface::hasCallback() const
{
    return mCallback != nullptr;
}

/**
 * @brief 获取回调函数
 * @return 回调函数对象
 */
DACallBackInterface::CallBack DACallBackInterface::getCallBack() const
{
    return mCallback;
}

/**
 * @brief 设置方向感知回调函数
 * @param fn 方向感知回调函数对象
 */
void DACallBackInterface::setDirectionalCallBack(DACallBackInterface::DirectionalCallBack fn)
{
    mDirectionalCallBack = fn;
}

/**
 * @brief 执行方向感知回调
 * @param isUndo 是否为撤销方向，false表示apply/redo方向，true表示undo方向
 */
void DACallBackInterface::directionalCallback(bool isUndo)
{
    if (mDirectionalCallBack) {
        mDirectionalCallBack(isUndo);
    }
}

}
