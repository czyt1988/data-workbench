#include "DAPyScriptsIO.h"
#include "DAPybind11QtTypeCast.h"
#include <QDebug>

/**
 * @def FUNCTION_STR_DICT
 * @brief 映射py中的函数fun(str,args)函数
 *
 * 如：
 *
 * @code
 * FUNCTION_STR_DICT(DAPyDataFrame, read_csv, read_csv)
 * @endcode
 *
 * 将会扩充为：
 *
 * @code
 * DAPyDataFrame DAPyScriptsIO::read_csv(const QString& filepath, const QVariantMap& args)
 * {
 *     try {
 *         pybind11::object fn = attr("read_csv");
 *         if (fn.is_none()) {
 *             qDebug() << "da_io.py have no attr read_csv";
 *             return DAPyDataFrame();
 *         }
 *         pybind11::object v = fn(DA::PY::toString(filepath), DA::PY::toDict(args));
 *         return DAPyDataFrame(std::move(v));
 *     } catch (const std::exception& e) {
 *         qCritical() << e.what();
 *     }
 *     return DAPyDataFrame();
 * }
 * @endcode
 */
#define FUNCTION_STR_DICT(returnType, functionName, pyFunctionName)                                                    \
    returnType DAPyScriptsIO::functionName(const QString& filepath, const QVariantMap& args, QString* err)             \
    {                                                                                                                  \
        try {                                                                                                          \
            pybind11::object fn = attr(#pyFunctionName);                                                               \
            if (fn.is_none()) {                                                                                        \
                qDebug() << "da_io.py have no attr " #pyFunctionName;                                                  \
                return returnType();                                                                                   \
            }                                                                                                          \
            pybind11::object v = fn(DA::PY::toPyStr(filepath), DA::PY::toPyDict(args));                                \
            return returnType(v);                                                                                      \
        } catch (const std::exception& e) {                                                                            \
            if (err) {                                                                                                 \
                *err = e.what();                                                                                       \
            }                                                                                                          \
            qDebug() << e.what();                                                                                      \
        }                                                                                                              \
        return returnType();                                                                                           \
    }

namespace DA
{

//===================================================
// DAPyScriptsIO
//===================================================
DAPyScriptsIO::DAPyScriptsIO(bool autoImport) : DAPyModule()
{
    if (autoImport) {
        if (!import()) {
            qCritical() << QObject::tr("can not import da_io module");
            throw std::runtime_error("Failed to import module: DAWorkbench.io");
        }
    }
}

DAPyScriptsIO::DAPyScriptsIO(const pybind11::object& obj) : DAPyModule(obj)
{
    if (isModule()) {
        qCritical() << QObject::tr("can not import DAWorkBench.io");
    }
}

DAPyScriptsIO::~DAPyScriptsIO()
{
}

/**
 * @brief DAPyScriptsIO::getFileReadFilters
 * @return 返回一个list，包含支持的文件[Images (*.png *.xpm *.jpg)] [Text files (*.txt)]
 */
QList< QString > DAPyScriptsIO::getFileReadFilters() const
{
    QList< QString > res;
    try {
        pybind11::object da_get_file_read_filters = attr("da_get_file_read_filters");
        if (da_get_file_read_filters.is_none()) {
            qDebug() << "da_io.py have no attr da_get_file_read_filters";
            return res;
        }
        pybind11::object f = da_get_file_read_filters();
        if (f.is_none()) {
            qDebug() << "da_io.da_get_file_read_filters() = none";
            return res;
        }
        QString err;
        res = DA::PY::toStringList(f, &err);
        if (!err.isEmpty()) {
            qCritical() << err;
        }
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return res;
}

/**
 * @brief 读取内容
 * @param filepath
 * @return
 */
FUNCTION_STR_DICT(DAPyObjectWrapper, read, da_read)

// DAPyObjectWrapper DAPyScriptsIO::read(const QString& filepath, const QVariantMap& args, QString* err)
// {
//     try {
//         pybind11::object da_read = attr("da_read");
//         if (da_read.is_none()) {
//             qDebug() << "da_io.py have no attr da_read";
//             return DAPyObjectWrapper();
//         }
//         pybind11::object f = da_read(DA::PY::toString(filepath));
//         return DAPyObjectWrapper(std::move(f));
//     } catch (const std::exception& e) {
//         if (err) {
//             *err = e.what();
//         }
//         qDebug() << e.what();
//     }
//     return DAPyObjectWrapper();
// }


/**
 * @brief 读取csv
 * @param filepath
 * @return
 */
FUNCTION_STR_DICT(DAPyDataFrame, read_csv, read_csv)

/**
 * @brief 读取txt
 * @param filepath
 * @return
 */
FUNCTION_STR_DICT(DAPyDataFrame, read_txt, read_txt)

/**
 * @brief 读取pkl
 * @param filepath
 * @return
 */
FUNCTION_STR_DICT(DAPyDataFrame, read_pkl, read_pkl)


/**
 * @brief 读取并直接添加到datamanager
 * @param filepath
 * @param args
 * @param err
 */
void DAPyScriptsIO::read_and_add_to_datamanager(const QString& filepath, const QVariantMap& args, QString* err)
{
    try {
        pybind11::object fun = attr("da_read_and_add_to_datamanager");
        if (fun.is_none()) {
            qDebug() << "da_io.py have no attr da_read";
            return;
        }
        fun(DA::PY::toPyStr(filepath), DA::PY::toPyDict(args));
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
}

/**
 * @brief 导入库
 */
bool DAPyScriptsIO::import()
{
    try {
        pybind11::module m = pybind11::module::import("DAWorkbench");
        object()           = m.attr("io");
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}

}  // namespace DA
