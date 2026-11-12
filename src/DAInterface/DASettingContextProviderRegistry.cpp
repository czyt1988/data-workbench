#include "DASettingContextProviderRegistry.h"
#include <QScopedValueRollback>
#include <QUndoStack>
#include <QWidget>

namespace DA
{
/**
 * @brief 清理 dock 内容已销毁的条目
 *
 * 只在提供者指针来自该条目、且条目正是当前提供者时清当前指针；不调用
 * deactivateSettingContext()——dock 内容已销毁时提供者可能随之失效，回调不安全。
 * @param it 指向待清理条目的迭代器（调用后条目被移除）
 */
void DASettingContextProviderRegistry::dropStaleEntry(QHash< QWidget*, Entry >::iterator it)
{
    // 同一提供者可能注册给多个 dock 内容，必须同时比较内容，避免清理非当前条目时误清当前状态
    if (mCurrentProvider && mCurrentProvider == it->provider && mCurrentContent == it.key()) {
        mCurrentProvider = nullptr;
        mCurrentContent.clear();
    }
    mProviders.erase(it);
}

/**
 * @brief 清理所有 dock 内容已销毁的条目
 */
void DASettingContextProviderRegistry::pruneStaleEntries()
{
    for (auto it = mProviders.begin(); it != mProviders.end();) {
        if (it->content.isNull()) {
            it = mProviders.erase(it);
        } else {
            ++it;
        }
    }
    // 当前提供者对应的 dock 内容已销毁：只清指针，不回调（见 dropStaleEntry 说明）
    if (mCurrentProvider && mCurrentContent.isNull()) {
        mCurrentProvider = nullptr;
    }
}

/**
 * @brief 注册 dock 内容的设置上下文提供者
 * @param dockContent dock 内部维护的内容窗口
 * @param provider 提供者指针（注册表不接管所有权）
 */
void DASettingContextProviderRegistry::registerProvider(QWidget* dockContent, DASettingContextProvider* provider)
{
    if (nullptr == dockContent || nullptr == provider) {
        return;
    }
    auto it = mProviders.find(dockContent);
    if (it != mProviders.end()) {
        if (it->provider == provider && !it->content.isNull()) {
            return;
        }
        // 同一内容换提供者：旧的是当前提供者（且当前内容就是它）时先失活
        if (mCurrentProvider && mCurrentProvider == it->provider && mCurrentContent == dockContent) {
            deactivateCurrent();
        }
        it->content  = dockContent;
        it->provider = provider;
        return;
    }
    Entry entry;
    entry.content  = dockContent;
    entry.provider = provider;
    mProviders.insert(dockContent, entry);
}

/**
 * @brief 注销 dock 内容的提供者
 * @param dockContent dock 内部维护的内容窗口
 */
void DASettingContextProviderRegistry::unregisterProvider(QWidget* dockContent)
{
    if (nullptr == dockContent) {
        return;
    }
    auto it = mProviders.find(dockContent);
    if (it == mProviders.end()) {
        // 条目可能已被惰性清理，但当前提供者仍指向该内容：一并失活
        if (mCurrentProvider && mCurrentContent == dockContent) {
            deactivateCurrent();
        }
        return;
    }
    if (mCurrentProvider && mCurrentProvider == it->provider && mCurrentContent == dockContent) {
        deactivateCurrent();
    }
    mProviders.erase(it);
}

/**
 * @brief 查询 dock 内容是否注册了提供者
 * @param dockContent dock 内部维护的内容窗口
 * @return 已注册返回 true
 */
bool DASettingContextProviderRegistry::isRegistered(QWidget* dockContent)
{
    return providerFor(dockContent) != nullptr;
}

/**
 * @brief 查询 dock 内容对应的提供者
 * @param dockContent dock 内部维护的内容窗口
 * @return 提供者指针，未注册或 dock 内容已销毁返回 nullptr
 */
DASettingContextProvider* DASettingContextProviderRegistry::providerFor(QWidget* dockContent)
{
    if (nullptr == dockContent) {
        return nullptr;
    }
    auto it = mProviders.find(dockContent);
    if (it == mProviders.end()) {
        return nullptr;
    }
    if (it->content.isNull()) {
        dropStaleEntry(it);
        return nullptr;
    }
    return it->provider;
}

/**
 * @brief 激活 dock 内容对应的提供者
 *
 * 命中时先失活上一个提供者（同一提供者重复激活也重新走 activateSettingContext，
 * 实现方需保证幂等），再激活其撤销栈；未命中时**不改变当前状态**并返回 false——
 * 失活时机由调用方决定（APP 只在焦点回到固定操作窗口或丢失时失活，避免程序化
 * raise 设置面板等工具 dock 时误清插件上下文）。
 * @param dockContent dock 内部维护的内容窗口
 * @return 命中并接管返回 true
 */
bool DASettingContextProviderRegistry::activateFor(QWidget* dockContent)
{
    // 重入保护：提供者在 activateSettingContext() 里 raise 自己/其它已注册 dock 时，
    // 会经由 dockWidgetRaised → 路由层再次进入本函数；此时不重复激活，直接返回是否已注册。
    if (mActivating) {
        return providerFor(dockContent) != nullptr;
    }
    DASettingContextProvider* provider = providerFor(dockContent);
    if (nullptr == provider) {
        return false;
    }
    if (mCurrentProvider != provider || mCurrentContent != dockContent) {
        deactivateCurrent();
        mCurrentProvider = provider;
        mCurrentContent  = dockContent;
    }
    // 激活回调可能触发程序化 raise（重入），用回滚守卫保证异常/重入路径下标志复位
    QScopedValueRollback< bool > activatingGuard(mActivating, true);
    provider->activateSettingContext();
    if (QUndoStack* stack = provider->contextUndoStack()) {
        if (!stack->isActive()) {
            stack->setActive();
        }
    }
    return true;
}

/**
 * @brief 失活当前的设置上下文提供者
 */
void DASettingContextProviderRegistry::deactivateCurrent()
{
    DASettingContextProvider* provider = mCurrentProvider;
    const bool contentAlive            = !mCurrentContent.isNull();
    mCurrentProvider                   = nullptr;
    mCurrentContent.clear();
    // dock 内容已销毁时不回调（提供者可能已失效）
    if (provider && contentAlive) {
        provider->deactivateSettingContext();
    }
}

/**
 * @brief 获取当前提供者
 * @return 当前提供者指针，无或 dock 内容已销毁返回 nullptr
 */
DASettingContextProvider* DASettingContextProviderRegistry::currentProvider()
{
    if (mCurrentProvider && mCurrentContent.isNull()) {
        // dock 内容已销毁：只清指针，不回调
        mCurrentProvider = nullptr;
    }
    return mCurrentProvider;
}

/**
 * @brief 获取当前提供者对应的 dock 内容
 * @return dock 内容窗口，无当前提供者时返回 nullptr
 */
QWidget* DASettingContextProviderRegistry::currentDockContent()
{
    if (!mCurrentProvider) {
        return nullptr;
    }
    return mCurrentContent.data();
}

/**
 * @brief 已注册的提供者数量
 * @return 有效条目数量（顺带清理 dock 内容已销毁的条目）
 */
int DASettingContextProviderRegistry::providerCount()
{
    pruneStaleEntries();
    return mProviders.size();
}

}  // namespace DA
