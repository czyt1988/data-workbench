#ifndef DAPYSCRIPTSIO_H
#define DAPYSCRIPTSIO_H
#include "DAPyScriptsGlobal.h"
#include "DAPyModule.h"
#include <QString>
#include <QList>
#include "DAPyObjectWrapper.h"

namespace DA
{
DA_IMPL_FORWARD_DECL(DAPyScriptsIO)

class DAPYSCRIPTS_API DAPyScriptsIO : public DAPyModule
{
    DA_IMPL(DAPyScriptsIO)
public:
    DAPyScriptsIO();
    ~DAPyScriptsIO();
    //获取打开对话框的filter da_io.da_get_file_read_filters
    QList< QString > getFileReadFilters() const;
    //读取内容
    DAPyObjectWrapper read(const QString& filepath);
    //引入
    bool import();
    //获取数据
};
}  // namespace DA
#endif  // DASCRIPTSIO_H
