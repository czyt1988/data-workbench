#ifndef DASETTINGCONTEXTPROVIDER_H
#define DASETTINGCONTEXTPROVIDER_H
class QUndoStack;

namespace DA
{
/**
 * @brief dock 内容窗口的「设置上下文提供者」
 *
 * 插件（或任意 dock 内容）实现本接口后，通过
 * DADockingAreaInterface::registerSettingContextProvider() 注册；APP 在该 dock 内容被
 * 激活时调用 activateSettingContext()，由提供者自己决定右侧设置页、Ribbon 上下文标签、
 * 图元树目标等设置上下文，从而不必依赖固定的数据/绘图/工作流操作窗口。
 *
 * 说明：本接口为纯抽象接口（无 out-of-line 成员、无静态数据），插件派生与跨库虚调用
 * 不需要导出符号，因此不加 DAINTERFACE_API，便于注册表逻辑被单元测试直接编译覆盖。
 *
 * 生命周期：实现方必须保证对象在注册期内有效，并在自身销毁前调用
 * DADockingAreaInterface::unregisterSettingContextProvider()（否则注册表后续回调会
 * 解引用已销毁的 provider）。
 */
class DASettingContextProvider
{
public:
    virtual ~DASettingContextProvider() = default;
    // 该 dock 内容成为激活页时调用：显示/切换设置面板与 ribbon 上下文
    virtual void activateSettingContext() = 0;
    // 失活时调用：隐藏本上下文的 ribbon 页、还原只在激活期出现的 UI
    virtual void deactivateSettingContext() = 0;
    // 本上下文的撤销栈（可为空，由宿主负责 setActive）
    virtual QUndoStack* contextUndoStack() const
    {
        return nullptr;
    }
};
}  // namespace DA
#endif  // DASETTINGCONTEXTPROVIDER_H
