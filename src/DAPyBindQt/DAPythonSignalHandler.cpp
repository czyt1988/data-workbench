#include "DAPythonSignalHandler.h"
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <exception>

namespace DA
{

DAPythonSignalHandler::DAPythonSignalHandler(QObject* parent) : QObject(parent), m_destroying(false)
{
    // 连接信号槽，使用Qt::QueuedConnection确保跨线程调用
    connect(this, &DAPythonSignalHandler::executeRequested, this, &DAPythonSignalHandler::onExecuteRequested, Qt::QueuedConnection);
}

DAPythonSignalHandler::~DAPythonSignalHandler()
{
    m_destroying = true;

    // 清理所有未执行的函数
    clearPendingFunctions();
}

void DAPythonSignalHandler::callInMainThread(std::function< void() > func)
{
    if (!func) {
        qDebug() << "DAPythonSignalHandler: Attempted to call empty function";
        return;
    }

    // 检查是否正在销毁中
    if (m_destroying) {
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
        std::lock_guard< std::mutex > lock(m_mutex);
        funcId                  = ++m_nextFuncId;
        m_functionMap[ funcId ] = std::make_shared< FunctionWrapper >(std::move(func));
    }

    // 发射信号，触发在主线程执行
    qDebug() << "DAPythonSignalHandler: Scheduling function for main thread execution, ID:" << funcId;
    Q_EMIT executeRequested(funcId);
}

void DAPythonSignalHandler::clearPendingFunctions()
{
    std::lock_guard< std::mutex > lock(m_mutex);

    int count = static_cast< int >(m_functionMap.size());
    if (count > 0) {
        qDebug() << "DAPythonSignalHandler: Clearing" << count << "pending functions";
    }

    m_functionMap.clear();
}

void DAPythonSignalHandler::onExecuteRequested(int funcWrapperId)
{
    // 检查是否正在销毁中
    if (m_destroying) {
        qDebug() << "DAPythonSignalHandler: Skipping execution during destruction, ID:" << funcWrapperId;
        return;
    }

    FunctionWrapperPtr wrapper;

    // 从映射中获取函数包装器
    {
        std::lock_guard< std::mutex > lock(m_mutex);
        auto it = m_functionMap.find(funcWrapperId);
        if (it == m_functionMap.end()) {
            qWarning() << "DAPythonSignalHandler: Function wrapper not found for ID:" << funcWrapperId;
            return;
        }
        wrapper = it.value();
        m_functionMap.erase(it);
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
