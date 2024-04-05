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
    DA_DECLARE_PRIVATE(DAPyScriptsIO)
public:
    DAPyScriptsIO();
    ~DAPyScriptsIO();
    // 获取打开对话框的filter da_io.da_get_file_read_filters
    QList< QString > getFileReadFilters() const;
    // 读取内容
    DAPyObjectWrapper read(const QString& filepath);
    // 读取csv
    DAPyDataFrame read_csv(const QString& filepath, const QVariantMap& args);
    // 读取txt
    DAPyDataFrame read_txt(const QString& filepath, const QVariantMap& args);
    // 引入
    bool import();
};
}  // namespace DA
#endif  // DASCRIPTSIO_H
