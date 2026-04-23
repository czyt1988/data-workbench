#ifndef DAPYGILGUARD_H
#define DAPYGILGUARD_H
#include "DAPyWorkFlowAPI.h"
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
class DAPYWORKFLOW_API DAPyGILGuard
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
class DAPYWORKFLOW_API DAPyGILRelease
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

/**
 * @brief 安全的PyObject持有者
 *
 * 持有pybind11::object并在析构时安全地释放引用。
 * 析构时检查Py_IsInitialized()，如果Python已终止则使用release()避免崩溃，
 * 否则正常DECREF（赋值为none）。
 * 此模式参考DAPyObjectWrapper的安全析构实现。
 *
 * @code
 * // 安全持有Python对象
 * DA::DAPySafePyObjectHolder holder(pybind11::module_::import("pandas"));
 * if (holder) {
 *     pybind11::object obj = holder.object();
 *     // 使用obj...
 * }
 * @endcode
 */
class DAPYWORKFLOW_API DAPySafePyObjectHolder
{
public:
    // 默认构造，持有none
    DAPySafePyObjectHolder();
    // 从pybind11::object构造
    DAPySafePyObjectHolder(const pybind11::object& obj);
    // 从pybind11::object移动构造
    DAPySafePyObjectHolder(pybind11::object&& obj);
    // 安全析构
    ~DAPySafePyObjectHolder();

    // 拷贝构造
    DAPySafePyObjectHolder(const DAPySafePyObjectHolder& other);
    // 移动构造
    DAPySafePyObjectHolder(DAPySafePyObjectHolder&& other);

    // 拷贝赋值
    DAPySafePyObjectHolder& operator=(const DAPySafePyObjectHolder& other);
    // 移动赋值
    DAPySafePyObjectHolder& operator=(DAPySafePyObjectHolder&& other);
    // 从pybind11::object赋值
    DAPySafePyObjectHolder& operator=(const pybind11::object& obj);
    // 从pybind11::object移动赋值
    DAPySafePyObjectHolder& operator=(pybind11::object&& obj);

    // 判断是否为None
    bool isNone() const;
    // bool操作符，判断是否非None
    explicit operator bool() const;

    // 获取内部pybind11::object的引用
    pybind11::object& object();
    // 获取内部pybind11::object的const引用
    const pybind11::object& object() const;

private:
    pybind11::object mPyObject;  ///< 持有的Python对象
};

}  // namespace DA

#endif  // DAPYGILGUARD_H