#include "DAPythonSignalHandler.h"
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <QMap>
#include <exception>
#include <mutex>

namespace DA
{

class DAPythonSignalHandler::PrivateData
{
    DA_DECLARE_PUBLIC(DAPythonSignalHandler)
public:
    PrivateData(DAPythonSignalHandler* p);

    QMap< int, FunctionWrapperPtr > mFunctionMap;  ///< 存储函数包装器的映射，键是唯一ID
    std::mutex mMutex;                             ///< 线程安全保护
    int mNextFuncId { 0 };                         ///< 下一个函数包装器的ID
    bool mDestroying { false };                    ///< 是否正在销毁中
};

DAPythonSignalHandler::PrivateData::PrivateData(DAPythonSignalHandler* p) : q_ptr(p)
{
}

/**
 * @brief 构造DAPythonSignalHandler对象
 *
 * 连接executeRequested信号到onExecuteRequested槽，使用Qt::QueuedConnection确保跨线程调用。
 * @param parent 父QObject对象
 */
DAPythonSignalHandler::DAPythonSignalHandler(QObject* parent)
    : QObject(parent), DA_PIMPL_CONSTRUCT
{
    // 连接信号槽，使用Qt::QueuedConnection确保跨线程调用
    connect(this, &DAPythonSignalHandler::executeRequested, this, &DAPythonSignalHandler::onExecuteRequested, Qt::QueuedConnection);
}

/**
 * @brief 析构DAPythonSignalHandler，清理所有未执行的函数
 */
DAPythonSignalHandler::~DAPythonSignalHandler()
{
    DA_D(d);
    d->mDestroying = true;

    // 清理所有未执行的函数
    clearPendingFunctions();
}

/**
 * @brief 从Python线程调用，请求在主线程执行函数
 *
 * 这个函数是线程安全的，可以从任何线程调用。
 * 如果当前已经在主线程，则直接执行函数；
 * 否则将函数包装后通过信号槽机制调度到主线程执行。
 * @param func 要在主线程执行的函数
 */
void DAPythonSignalHandler::callInMainThread(std::function< void() > func)
{
    DA_D(d);
    if (!func) {
        qDebug() << "DAPythonSignalHandler: Attempted to call empty function";
        return;
    }

    // 检查是否正在销毁中
    if (d->mDestroying) {
        qDebug() << "DAPythonSignalHandler: Ignoring call during destruction";
        return;
    }

    // 检查是否已经在主线程
    // 获取应用程序实例
    QCoreApplication* app = QCoreApplication::instance();
    if (!app) {
        qWarning() << "DAPythonSignalHandler: No QCoreApplication instance exists";
        return;
    }
    // 检查是否已经在主线程（正确的方式）
    if (QThread::currentThread() == app->thread()) {
        // 已经在主线程，直接执行
        func();
        return;
    }

    // 创建函数包装器
    int funcId;
    {
        std::lock_guard< std::mutex > lock(d->mMutex);
        funcId                   = ++d->mNextFuncId;
        d->mFunctionMap[ funcId ] = std::make_shared< FunctionWrapper >(std::move(func));
    }

    // 发射信号，触发在主线程执行
    qDebug() << "DAPythonSignalHandler: Scheduling function for main thread execution, ID:" << funcId;
    Q_EMIT executeRequested(funcId);
}

/**
 * @brief 清理所有待执行的函数
 *
 * 在主窗口销毁前调用，确保所有函数都被清理。
 */
void DAPythonSignalHandler::clearPendingFunctions()
{
    DA_D(d);
    std::lock_guard< std::mutex > lock(d->mMutex);

    int count = static_cast< int >(d->mFunctionMap.size());
    if (count > 0) {
        qDebug() << "DAPythonSignalHandler: Clearing" << count << "pending functions";
    }

    d->mFunctionMap.clear();
}

/**
 * @brief 在主线程执行的槽函数
 *
 * 从映射中查找对应ID的函数包装器并执行。
 * 执行过程中捕获异常并通过qCritical输出错误信息。
 * @param funcWrapperId 函数包装器的ID
 */
void DAPythonSignalHandler::onExecuteRequested(int funcWrapperId)
{
    DA_D(d);
    // 检查是否正在销毁中
    if (d->mDestroying) {
        qDebug() << "DAPythonSignalHandler: Skipping execution during destruction, ID:" << funcWrapperId;
        return;
    }

    FunctionWrapperPtr wrapper;

    // 从映射中获取函数包装器
    {
        std::lock_guard< std::mutex > lock(d->mMutex);
        auto it = d->mFunctionMap.find(funcWrapperId);
        if (it == d->mFunctionMap.end()) {
            qWarning() << "DAPythonSignalHandler: Function wrapper not found for ID:" << funcWrapperId;
            return;
        }
        wrapper = it.value();
        d->mFunctionMap.erase(it);
    }

    // 执行函数
    qDebug() << "DAPythonSignalHandler: Executing function in main thread, ID:" << funcWrapperId;
    try {
        wrapper->execute();
    } catch (const std::exception& e) {
        qCritical() << "DAPythonSignalHandler: Exception in main thread function:" << e.what();
    } catch (...) {
        qCritical() << "DAPythonSignalHandler: Unknown exception in main thread function";
    }
}

}  // end DA
