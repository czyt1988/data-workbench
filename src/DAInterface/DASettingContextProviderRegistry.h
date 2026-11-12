#ifndef DASETTINGCONTEXTPROVIDERREGISTRY_H
#define DASETTINGCONTEXTPROVIDERREGISTRY_H
#include <QHash>
#include <QPointer>
#include "DASettingContextProvider.h"

class QWidget;

namespace DA
{
/**
 * @brief 设置上下文提供者注册表（dock 内容 -> 提供者）
 *
 * 由 DADockingAreaInterface 持有，APP 路由层（DAAppController）在 dock 内容被激活时
 * 用它判断是否由插件接管设置上下文，并负责「失活上一个提供者 + 激活当前提供者 +
 * 激活其撤销栈」。与具体 dock 管理器解耦，便于单独测试。
 *
 * 生命周期约定：注册方必须在 dock 内容销毁前调用 unregisterProvider()（否则注册表
 * 只清理映射、不回调已失效的提供者）。dock 内容先被销毁时，注册表在下一次查询时
 * 惰性清理对应条目。
 */
class DASettingContextProviderRegistry
{
public:
    DASettingContextProviderRegistry() = default;
    ~DASettingContextProviderRegistry() = default;
    DASettingContextProviderRegistry(const DASettingContextProviderRegistry&)            = delete;
    DASettingContextProviderRegistry& operator=(const DASettingContextProviderRegistry&) = delete;

    // 注册 dock 内容的设置上下文提供者（同一内容重复注册以最后一次为准）
    void registerProvider(QWidget* dockContent, DASettingContextProvider* provider);
    // 注销 dock 内容的提供者（若它是当前提供者，会先失活）
    void unregisterProvider(QWidget* dockContent);
    // 查询 dock 内容是否有注册的提供者
    bool isRegistered(QWidget* dockContent);
    // 查询 dock 内容对应的提供者（未注册或已失效返回 nullptr）
    DASettingContextProvider* providerFor(QWidget* dockContent);
    // 激活 dock 内容对应的提供者：命中返回 true（先失活上一个 + setActive 撤销栈）；未命中返回 false 且不改变当前状态
    bool activateFor(QWidget* dockContent);
    // 失活当前提供者（如果没有则无操作）
    void deactivateCurrent();
    // 当前提供者（dock 内容已销毁时惰性清理并返回 nullptr）
    DASettingContextProvider* currentProvider();
    // 当前提供者对应的 dock 内容
    QWidget* currentDockContent();
    // 已注册的提供者数量（顺带清理 dock 内容已销毁的条目）
    int providerCount();

private:
    struct Entry
    {
        QPointer< QWidget > content;
        DASettingContextProvider* provider { nullptr };
    };
    // 清理 dock 内容已销毁的条目；若清理的正是当前提供者，只清指针（不回调）
    void dropStaleEntry(QHash< QWidget*, Entry >::iterator it);
    void pruneStaleEntries();

private:
    QHash< QWidget*, Entry > mProviders;
    QPointer< QWidget > mCurrentContent;
    DASettingContextProvider* mCurrentProvider { nullptr };
};
}  // namespace DA
#endif  // DASETTINGCONTEXTPROVIDERREGISTRY_H
