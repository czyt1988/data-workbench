#include "DAPyInterpreter.h"
#include "pybind11/embed.h"
#include <QDebug>
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
    for (DAPyInterpreter::callback_finalize& f : _finalizeCallbacks) {
        if (f) {
            f();
        }
    }
    pybind11::finalize_interpreter();
}

void DAPyInterpreter::registerFinalizeCallback(DAPyInterpreter::callback_finalize fp)
{
    _finalizeCallbacks.push_back(fp);
}
