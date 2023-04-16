#include "DAPyScriptsIO.h"
#include "DAPybind11QtTypeCast.h"
#include <QDebug>
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
 * @brief 导入库
 */
bool DAPyScriptsIO::import()
{
    return DAPyModule::import("da_io");
}
}  // namespace DA
