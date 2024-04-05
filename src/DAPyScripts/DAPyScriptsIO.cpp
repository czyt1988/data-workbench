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
    returnType DAPyScriptsIO::functionName(const QString& filepath, const QVariantMap& args)                           \
    {                                                                                                                  \
        try {                                                                                                          \
            pybind11::object fn = attr(#pyFunctionName);                                                               \
            if (fn.is_none()) {                                                                                        \
                qDebug() << "da_io.py have no attr " #pyFunctionName;                                                  \
                return returnType();                                                                                   \
            }                                                                                                          \
            pybind11::object v = fn(DA::PY::toString(filepath), DA::PY::toDict(args));                                 \
            return returnType(std::move(v));                                                                           \
        } catch (const std::exception& e) {                                                                            \
            qCritical() << e.what();                                                                                   \
        }                                                                                                              \
        return returnType();                                                                                           \
    }

namespace DA
{
class DAPyScriptsIO::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyScriptsIO)
public:
    PrivateData(DAPyScriptsIO* p);
};

//===================================================
// DAPyScriptsIOPrivate
//===================================================
DAPyScriptsIO::PrivateData::PrivateData(DAPyScriptsIO* p) : q_ptr(p)
{
}
//===================================================
// DAPyScriptsIO
//===================================================
DAPyScriptsIO::DAPyScriptsIO() : DAPyModule(), DA_PIMPL_CONSTRUCT
{
    if (!import()) {
        qCritical() << QObject::tr("can not import da_io module");
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
DAPyObjectWrapper DAPyScriptsIO::read(const QString& filepath)
{
    try {
        pybind11::object da_read = attr("da_read");
        if (da_read.is_none()) {
            qDebug() << "da_io.py have no attr da_read";
            return DAPyObjectWrapper();
        }
        pybind11::object f = da_read(DA::PY::toString(filepath));
        return DAPyObjectWrapper(std::move(f));
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return DAPyObjectWrapper();
}

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
 * @brief 导入库
 */
bool DAPyScriptsIO::import()
{
    return DAPyModule::import("da_io");
}
}  // namespace DA
