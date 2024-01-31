#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
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

/**
 * @brief 获取系统记录的python环境
 * @return
 */
QList< QFileInfo > DAPyInterpreter::wherePython()
{
    QProcess process;
    QString command = "where python";
    process.start(command);
    if (!process.waitForFinished()) {
        return QList< QFileInfo >();
    }
    QString res = process.readAll();
    qDebug() << res;
    const QList< QString > pys = res.split("\r\n");
    QList< QFileInfo > validFis;
    // 遍历所有环境，确认是否的确是ptython路径,where 有时候会返回一些不正确的路径
    for (QString p : pys) {
        QFileInfo fi(p);
        if (fi.isExecutable()) {
            // 说明是可执行文件，windows下就是pythhon.exe
            validFis.append(fi);
        }
    }
    return validFis;
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
