#include "DAPyInterpreter.h"
#include "pybind11/embed.h"
#include <QDebug>
#include <QProcess>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyInterpreter
//===================================================
DAPyInterpreter::DAPyInterpreter()
{
}

DAPyInterpreter::~DAPyInterpreter()
{
    finalizePythonInterpreter();
}

DAPyInterpreter& DAPyInterpreter::getInstance()
{
    static DAPyInterpreter s_python;
    return s_python;
}

QList< QString > DAPyInterpreter::wherePython()
{
    QProcess process;
    QString command = "where python";
    process.start(command);
    if (!process.waitForFinished()) {
        return QList< QString >();
    }
    QString res = process.readAll();
    qDebug() << res;
    return res.split("\r\n");
}

/**
 * @brief 设置python的运行路径
 * @param path
 */
void DAPyInterpreter::setPythonHomePath(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
    std::vector< wchar_t > wp((path.size() + 1) * 4, 0);

    path.toWCharArray(wp.data());
    try {
        Py_SetPythonHome(wp.data());
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
}

void DAPyInterpreter::initializePythonInterpreter()
{
    pybind11::initialize_interpreter();
}

void DAPyInterpreter::finalizePythonInterpreter()
{
    for (DAPyInterpreter::callback_finalize& f : mFinalizeCallbacks) {
        if (f) {
            f();
        }
    }
    pybind11::finalize_interpreter();
}

void DAPyInterpreter::registerFinalizeCallback(DAPyInterpreter::callback_finalize fp)
{
    mFinalizeCallbacks.push_back(fp);
}
