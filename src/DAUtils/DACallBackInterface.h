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
    /// 方向感知回调：isUndo=false 表示 apply/redo 方向，true 表示 undo 方向
    using DirectionalCallBack = std::function< void(bool isUndo) >;
    DACallBackInterface();
    virtual ~DACallBackInterface();
    void setCallBack(CallBack fn);
    CallBack getCallBack() const;
    void callback();
    // 是否有回调
    bool hasCallback() const;
    // 设置方向感知回调（用于行/列增删时的样式键同步等需区分 redo/undo 的场景）
    void setDirectionalCallBack(DirectionalCallBack fn);
    void directionalCallback(bool isUndo);
    // 获取方向感知回调（多方接线时链式包裹既有回调，避免相互覆盖）
    DirectionalCallBack getDirectionalCallBack() const;
    // 是否设置了方向感知回调
    bool hasDirectionalCallBack() const;

private:
    CallBack mCallback;
    DirectionalCallBack mDirectionalCallBack;
};
}

#endif  // DACALLBACKINTERFACE_H
