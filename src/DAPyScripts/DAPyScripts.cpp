#include "DAPyScripts.h"
#include <QObject>
#include <QDebug>
#include "DAPyScriptsIO.h"
#include "DAPybind11QtTypeCast.h"
namespace DA
{
class DAPyScriptsPrivate
{
    DA_IMPL_PUBLIC(DAPyScripts)
public:
    DAPyScriptsPrivate(DAPyScripts* p);

public:
    DAPyModule _pysys;
    DAPyScriptsIO _pyio;                ///< 对应da_io.py
    DAPyScriptsDataFrame _pydataframe;  ///< 对应da_dataframe.py
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyScriptsPrivate
//===================================================
DAPyScriptsPrivate::DAPyScriptsPrivate(DAPyScripts* p) : q_ptr(p)
{
}
//===================================================
// DAPyScripts
//===================================================
DAPyScripts::DAPyScripts() : d_ptr(new DAPyScriptsPrivate(this))
{
    loadSysModule();
}

DAPyScripts::~DAPyScripts()
{
}

/**
 * @brief 添加python环境路径
 *
 * 等同于：
 * @code
 * import sys
 * sys.path.append(xx)
 * @endcode
 *
 * @param path
 */
void DAPyScripts::appendSysPath(const QString& path)
{
    try {
        pybind11::object obj_path_append = d_ptr->_pysys.attr("path").attr("append");
        obj_path_append(DA::PY::toString(path));
    } catch (const std::exception& e) {
        qCritical() << QObject::tr("Initialized import sys module error:%1").arg(e.what());
    }
}

/**
 * @brief 此函数在main函数中调用，若失败应用程序考虑是否继续
 * @param err
 * @return
 */
bool DAPyScripts::initScripts()
{
    try {
        if (!(d_ptr->_pyio.import())) {
            return false;
        }
        if (!(d_ptr->_pydataframe.import())) {
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        qCritical() << QObject::tr("Initialized import py module error:%1").arg(e.what());
    }
    return false;
}

DAPyScripts& DAPyScripts::getInstance()
{
    static DAPyScripts s_pyscripts;
    return s_pyscripts;
}

/**
 * @brief 获取io相关的script
 * @return
 */
DAPyScriptsIO& DAPyScripts::getIO()
{
    return d_ptr->_pyio;
}

/**
 * @brief 获取dataframe相关的script
 * @return
 */
DAPyScriptsDataFrame& DAPyScripts::getDataFrame()
{
    return d_ptr->_pydataframe;
}

/**
 * @brief import sys
 * @return
 */
bool DAPyScripts::loadSysModule()
{
    if (!d_ptr->_pysys.import("sys")) {
        return false;
    }
    return true;
}
