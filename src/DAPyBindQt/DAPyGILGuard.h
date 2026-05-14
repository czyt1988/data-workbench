#ifndef DAPYGILGUARD_H
#define DAPYGILGUARD_H
#include "DAPyBindQtGlobal.h"
#include "DAPybind11InQt.h"

namespace DA
{

/**
 * @brief GIL获取的RAII守卫类
 *
 * 在构造时获取Python全局解释器锁(GIL)，在析构时释放。
 * 用于在非Python线程（如Qt主线程）中调用Python代码时确保线程安全。
 *
 * GIL安全规则：
 * - error_already_set异常必须在gil_scoped_acquire作用域内消费，
 *   否则异常析构时尝试获取GIL会导致死锁
 * - 不允许在持有GIL时调用QThread::wait()或类似阻塞等待操作
 * - 禁止在静态初始化阶段调用Python API（GIL尚未就绪）
 *
 * @code
 * // 在Qt线程中调用Python代码的典型用法
 * {
 *     DA::DAPyGILGuard gil;
 *     pybind11::module_::import("numpy");
 * } // gil析构时自动释放
 * @endcode
 */
class DAPYBINDQT_API DAPyGILGuard
{
public:
    // 获取GIL
    DAPyGILGuard();
    // 释放GIL
    ~DAPyGILGuard();

    // 判断GIL是否已被当前守卫获取
    bool isAcquired() const;

    // 主动释放GIL（之后析构不再重复释放）
    void release();

    // 禁止拷贝
    DAPyGILGuard(const DAPyGILGuard&)            = delete;
    DAPyGILGuard& operator=(const DAPyGILGuard&) = delete;

private:
    bool mIsAcquired { false };                       ///< GIL获取状态
    std::unique_ptr< pybind11::gil_scoped_acquire > mAcquire;  ///< pybind11 GIL获取器
};

/**
 * @brief GIL释放的RAII守卫类
 *
 * 在构造时释放Python全局解释器锁(GIL)，在析构时重新获取。
 * 用于在持有GIL的Python调用过程中，需要临时释放GIL以允许其他Python线程执行，
 * 或在Python回调中需要发射Qt信号时避免死锁。
 *
 * @code
 * // 在Python调用中临时释放GIL以发射Qt信号
 * {
 *     DA::DAPyGILGuard gil; // 先获取GIL
 *     // 执行Python代码...
 *     {
 *         DA::DAPyGILRelease release; // 临时释放GIL
 *         emit someSignal();          // 安全发射Qt信号
 *     } // 重新获取GIL
 *     // 继续Python操作...
 * } // 释放GIL
 * @endcode
 */
class DAPYBINDQT_API DAPyGILRelease
{
public:
    // 释放GIL
    DAPyGILRelease();
    // 重新获取GIL
    ~DAPyGILRelease();

    // 判断GIL是否已被当前守卫释放
    bool isReleased() const;

    // 主动重新获取GIL（之后析构不再重复获取）
    void reacquire();

    // 禁止拷贝
    DAPyGILRelease(const DAPyGILRelease&)            = delete;
    DAPyGILRelease& operator=(const DAPyGILRelease&) = delete;

private:
    bool mIsReleased { false };                        ///< GIL释放状态
    std::unique_ptr< pybind11::gil_scoped_release > mRelease; ///< pybind11 GIL释放器
};

}  // namespace DA

#endif  // DAPYGILGUARD_H