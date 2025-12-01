#ifndef DAPYSCRIPTSIO_H
#define DAPYSCRIPTSIO_H
#include "DAPyScriptsGlobal.h"
#include "DAPyModule.h"
#include <QString>
#include <QList>
#include "DAPyObjectWrapper.h"
#include "pandas/DAPyDataFrame.h"
namespace DA
{

/**
 * @brief 封装的da_io.py
 */
class DAPYSCRIPTS_API DAPyScriptsIO : public DAPyModule
{
public:
    DAPyScriptsIO(bool autoImport = true);
    DAPyScriptsIO(const pybind11::object& obj);
    ~DAPyScriptsIO();
    // 获取打开对话框的filter da_io.da_get_file_read_filters
    QList< QString > getFileReadFilters() const;
    // 读取内容,会自动根据后缀选择读取的函数
    DAPyObjectWrapper read(const QString& filepath, const QVariantMap& args, QString* err = nullptr);
    // 读取csv
    DAPyDataFrame read_csv(const QString& filepath, const QVariantMap& args, QString* err = nullptr);
    // 读取txt
    DAPyDataFrame read_txt(const QString& filepath, const QVariantMap& args, QString* err = nullptr);
    // 读取pkl
    DAPyDataFrame read_pkl(const QString& filepath, const QVariantMap& args, QString* err = nullptr);
    // 引入
    bool import();
};
}  // namespace DA
#endif  // DASCRIPTSIO_H
