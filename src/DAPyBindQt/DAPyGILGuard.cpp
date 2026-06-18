#include "DAPyGILGuard.h"
#include <QDebug>

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyGILGuard
//===================================================

/**
 * @brief 获取Python全局解释器锁(GIL)
 *
 * 构造时通过pybind11::gil_scoped_acquire获取GIL，
 * 确保当前线程可以安全调用Python API。
 * 如果Python未初始化或GIL获取失败，mIsAcquired将保持为false。
 *
 * @note 构造前应确保Python解释器已初始化（Py_IsInitialized()返回true）
 * @note 不允许在静态初始化阶段使用此类
 */
DAPyGILGuard::DAPyGILGuard()
{
    if (!Py_IsInitialized()) {
        qWarning() << "DAPyGILGuard: Python interpreter is not initialized, skip GIL acquire";
        mIsAcquired = false;
        return;
    }
    try {
        mAcquire  = std::make_unique< pybind11::gil_scoped_acquire >();
        mIsAcquired = true;
    } catch (const std::exception& e) {
        qCritical() << "DAPyGILGuard: Failed to acquire GIL:" << e.what();
        mIsAcquired = false;
    }
}

/**
 * @brief 释放Python全局解释器锁(GIL)
 *
 * 析构时释放GIL，如果mIsAcquired为true且mAcquire有效，
 * 销毁gil_scoped_acquire对象将自动释放GIL。
 * 如果之前已通过release()主动释放，析构不再重复操作。
 *
 * @note 析构顺序很重要——确保所有Python对象在GIL守卫析构前清理完毕
 */
DAPyGILGuard::~DAPyGILGuard()
{
    if (mIsAcquired && mAcquire) {
        mAcquire.reset();
        mIsAcquired = false;
    }
}

/**
 * @brief 判断GIL是否已被当前守卫获取
 *
 * @return true表示GIL已被成功获取，false表示获取失败或已释放
 */
bool DAPyGILGuard::isAcquired() const
{
    return mIsAcquired;
}

/**
 * @brief 主动释放GIL
 *
 * 调用后GIL将被释放，mIsAcquired置为false，
 * 后续析构不再执行释放操作。
 * 适用于需要在GIL守卫生命周期内提前释放GIL的场景。
 *
 * @note 释放GIL后不应再调用任何Python API
 */
void DAPyGILGuard::release()
{
    if (mIsAcquired && mAcquire) {
        mAcquire.reset();
        mIsAcquired = false;
    }
}

//===================================================
// DAPyGILRelease
//===================================================

/**
 * @brief 释放Python全局解释器锁(GIL)
 *
 * 构造时通过pybind11::gil_scoped_release释放GIL，
 * 允许其他Python线程获取GIL并执行。
 * 常用于在持有GIL期间需要发射Qt信号或执行阻塞等待的场景。
 *
 * @note 使用前必须确保当前线程已持有GIL
 * @note 释放GIL后当前线程不可调用任何Python API
 */
DAPyGILRelease::DAPyGILRelease()
{
    if (!Py_IsInitialized()) {
        qWarning() << "DAPyGILRelease: Python interpreter is not initialized, skip GIL release";
        mIsReleased = false;
        return;
    }
    try {
        mRelease   = std::make_unique< pybind11::gil_scoped_release >();
        mIsReleased = true;
    } catch (const std::exception& e) {
        qCritical() << "DAPyGILRelease: Failed to release GIL:" << e.what();
        mIsReleased = false;
    }
}

/**
 * @brief 重新获取Python全局解释器锁(GIL)
 *
 * 析构时重新获取GIL，如果mIsReleased为true且mRelease有效，
 * 销毁gil_scoped_release对象将自动重新获取GIL。
 * 如果之前已通过reacquire()主动重新获取，析构不再重复操作。
 *
 * @note GIL重新获取后方可继续调用Python API
 */
DAPyGILRelease::~DAPyGILRelease()
{
    if (mIsReleased && mRelease) {
        mRelease.reset();
        mIsReleased = false;
    }
}

/**
 * @brief 判断GIL是否已被当前守卫释放
 *
 * @return true表示GIL已被成功释放，false表示释放失败或已重新获取
 */
bool DAPyGILRelease::isReleased() const
{
    return mIsReleased;
}

/**
 * @brief 主动重新获取GIL
 *
 * 调用后GIL将被重新获取，mIsReleased置为false，
 * 后续析构不再执行重新获取操作。
 * 适用于需要在GIL释放守卫生命周期内提前恢复GIL的场景。
 *
 * @note 重新获取GIL后其他Python线程将无法获取GIL
 */
void DAPyGILRelease::reacquire()
{
    if (mIsReleased && mRelease) {
        mRelease.reset();
        mIsReleased = false;
    }
}