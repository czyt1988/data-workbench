#ifndef DAZIPARCHIVETASK_DADATA_H
#define DAZIPARCHIVETASK_DADATA_H
#include "DAGuiAPI.h"
#include <QDir>
#include "DAAbstractArchiveTask.h"
#include "DAData.h"
#include <QTemporaryDir>

namespace DA
{
class DAZipArchive;
/**
 * @brief 针对dadata的序列化
 *
 * 此任务不需要指定zip里的路径，数据存放规则为:
 *
 * - datas/dataframe/{dataframe相关数据}
 * - datas/series/{series相关数据}
 * - datas/object/{object相关数据}
 */
class DAZipArchiveTask_DAData : public DAAbstractArchiveTask
{
public:
    // 加载构造
    DAZipArchiveTask_DAData();
    // 保存构造
    DAZipArchiveTask_DAData(const DAData& data);
    ~DAZipArchiveTask_DAData();
    // data
    DAData getData() const;
    void setData(const DAData& data);
    //
    virtual bool exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode) override;

private:
    bool saveData(DAZipArchive* archive, const DAData& data);

private:
    DAData mData;
    QTemporaryDir mTempDir;
};

}

#endif  // DAZIPARCHIVETASK_DADATA_H
