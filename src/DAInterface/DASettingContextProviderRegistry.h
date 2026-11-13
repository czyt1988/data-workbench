#ifndef DASETTINGCONTEXTPROVIDERREGISTRY_H
#define DASETTINGCONTEXTPROVIDERREGISTRY_H
#include <QHash>
#include <QPointer>
#include "DASettingContextProvider.h"

class QWidget;

// 导出注解：本头文件列在安装公开头（DA_LIB_HEADER_FILES）中，插件按公开头实例化时
// 需要 dllimport（否则 MSVC 链接失败 LNK2019）。但 APPRefactorTest 直接把
// DASettingContextProviderRegistry.cpp 编入测试可执行文件且不链接 DAInterface 库，
// 该测试目标定义 DASETTINGCONTEXTREGISTRY_STATIC 以获得无注解声明（dllimport 类的
// 类外成员定义会被 MSVC 拒绝：C2491），同时跳过 DAInterfaceAPI.h（其 DAGlobals.h
// 依赖不在该测试的 include 路径内）。
#if defined(DASETTINGCONTEXTREGISTRY_STATIC)
#define DASETTINGCONTEXTREGISTRY_API
#else
#include "DAInterfaceAPI.h"
#define DASETTINGCONTEXTREGISTRY_API DAINTERFACE_API
#endif

namespace DA
{
/**
 * @brief 设置上下文提供者注册表（dock 内容 -> 提供者）
 *
 * 由 DADockingAreaInterface 持有，APP 路由层（DAAppController）在 dock 内容被激活时
 * 用它判断是否由插件接管设置上下文，并负责「失活上一个提供者 + 激活当前提供者 +
 * 激活其撤销栈」。与具体 dock 管理器解耦，便于单独测试。
 *
 * 生命周期约定：
 * - provider 的生命周期必须长于注册期：**注册方必须在 provider 销毁前调用
 *   unregisterProvider()**（否则注册表后续回调会解引用已销毁的 provider）；
 * - dock 内容先于 provider 销毁时，注册表用 QPointer 惰性清理条目、只清状态不回调
 *   （提供者可能随之失效）；
 * - 同一 provider 可以注册给多个 dock 内容，注册表按「内容 + 提供者」共同判断当前项。
 */
class DASETTINGCONTEXTREGISTRY_API DASettingContextProviderRegistry
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
    bool mActivating { false };  ///< 激活回调中的重入保护
};
}  // namespace DA
#endif  // DASETTINGCONTEXTPROVIDERREGISTRY_H
