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

//===================================================
// DAPySafePyObjectHolder
//===================================================

/**
 * @brief 默认构造，持有Python None对象
 *
 * 创建一个持有pybind11::none()的空持有者，
 * isNone()将返回true，operator bool()将返回false。
 *
 * @note 需要Python解释器已初始化
 */
DAPySafePyObjectHolder::DAPySafePyObjectHolder()
    : mPyObject(pybind11::none())
{
}

/**
 * @brief 从pybind11::object拷贝构造
 *
 * 拷贝传入的Python对象，增加其引用计数。
 *
 * @param[in] obj 要持有的Python对象
 */
DAPySafePyObjectHolder::DAPySafePyObjectHolder(const pybind11::object& obj)
    : mPyObject(obj)
{
}

/**
 * @brief 从pybind11::object移动构造
 *
 * 移动传入的Python对象，不增加引用计数。
 *
 * @param[in] obj 要持有的Python对象（移动后原对象变为none）
 */
DAPySafePyObjectHolder::DAPySafePyObjectHolder(pybind11::object&& obj)
    : mPyObject(std::move(obj))
{
}

/**
 * @brief 安全析构Python对象
 *
 * 析构时检查Python解释器是否仍然初始化：
 * - 如果Py_IsInitialized()返回true，正常将mPyObject赋值为none，
 *   pybind11会自动处理DECREF
 * - 如果Py_IsInitialized()返回false（Python已终止），使用release()
 *   避免在未初始化的Python中DECREF导致崩溃
 *
 * 此模式参考DAPyObjectWrapper的安全析构实现，确保在程序退出时
 * Python解释器可能先于C++对象销毁的场景下不会崩溃。
 *
 * @note 此析构函数不需要GIL，因为它仅在Python未初始化时使用release()
 */
DAPySafePyObjectHolder::~DAPySafePyObjectHolder()
{
    if (!mPyObject.is_none()) {
        if (Py_IsInitialized()) {
            mPyObject = pybind11::none();
        } else {
            mPyObject.release();
        }
    }
}

/**
 * @brief 拷贝构造
 *
 * 拷贝另一个持有者的Python对象，增加引用计数。
 *
 * @param[in] other 要拷贝的持有者
 */
DAPySafePyObjectHolder::DAPySafePyObjectHolder(const DAPySafePyObjectHolder& other)
    : mPyObject(other.mPyObject)
{
}

/**
 * @brief 移动构造
 *
 * 移动另一个持有者的Python对象，原持有者变为None。
 *
 * @param[in] other 要移动的持有者（移动后变为None）
 */
DAPySafePyObjectHolder::DAPySafePyObjectHolder(DAPySafePyObjectHolder&& other)
    : mPyObject(std::move(other.mPyObject))
{
}

/**
 * @brief 拷贝赋值
 *
 * 将另一个持有者的Python对象拷贝赋值给当前持有者，
 * 当前持有者的旧对象将被安全释放。
 *
 * @param[in] other 要拷贝的持有者
 * @return 当前持有者的引用
 */
DAPySafePyObjectHolder& DAPySafePyObjectHolder::operator=(const DAPySafePyObjectHolder& other)
{
    if (this != &other) {
        mPyObject = other.mPyObject;
    }
    return *this;
}

/**
 * @brief 移动赋值
 *
 * 将另一个持有者的Python对象移动赋值给当前持有者，
 * 当前持有者的旧对象将被安全释放，原持有者变为None。
 *
 * @param[in] other 要移动的持有者（移动后变为None）
 * @return 当前持有者的引用
 */
DAPySafePyObjectHolder& DAPySafePyObjectHolder::operator=(DAPySafePyObjectHolder&& other)
{
    if (this != &other) {
        mPyObject = std::move(other.mPyObject);
    }
    return *this;
}

/**
 * @brief 从pybind11::object拷贝赋值
 *
 * 将Python对象拷贝赋值给当前持有者，
 * 当前持有者的旧对象将在赋值时自动DECREF。
 *
 * @param[in] obj 要赋值的Python对象
 * @return 当前持有者的引用
 */
DAPySafePyObjectHolder& DAPySafePyObjectHolder::operator=(const pybind11::object& obj)
{
    mPyObject = obj;
    return *this;
}

/**
 * @brief 从pybind11::object移动赋值
 *
 * 将Python对象移动赋值给当前持有者，
 * 当前持有者的旧对象将在赋值时自动DECREF。
 *
 * @param[in] obj 要赋值的Python对象（移动后变为None）
 * @return 当前持有者的引用
 */
DAPySafePyObjectHolder& DAPySafePyObjectHolder::operator=(pybind11::object&& obj)
{
    mPyObject = std::move(obj);
    return *this;
}

/**
 * @brief 判断持有的对象是否为Python None
 *
 * @return true表示持有None对象，false表示持有有效Python对象
 */
bool DAPySafePyObjectHolder::isNone() const
{
    return mPyObject.is_none();
}

/**
 * @brief bool操作符，判断持有对象是否有效（非None）
 *
 * @return true表示持有有效Python对象，false表示持有None
 */
DAPySafePyObjectHolder::operator bool() const
{
    return !isNone();
}

/**
 * @brief 获取内部pybind11::object的引用
 *
 * 返回内部Python对象的引用，可用于直接操作Python对象。
 * 调用此方法后进行的Python操作需确保GIL已被获取。
 *
 * @return pybind11::object的引用
 * @note 调用者需确保在使用返回值时持有GIL
 */
pybind11::object& DAPySafePyObjectHolder::object()
{
    return mPyObject;
}

/**
 * @brief 获取内部pybind11::object的const引用
 *
 * 返回内部Python对象的const引用，可用于读取Python对象属性。
 *
 * @return pybind11::object的const引用
 * @note 调用者需确保在使用返回值时持有GIL
 */
const pybind11::object& DAPySafePyObjectHolder::object() const
{
    return mPyObject;
}