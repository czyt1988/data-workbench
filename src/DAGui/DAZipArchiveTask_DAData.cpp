#include "DAZipArchiveTask_DAData.h"
#include "DAZipArchive.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include "DAPyScriptsDataFrame.h"
#include "DADir.h"
#if DA_ENABLE_PYTHON
#include <pybind11/pybind11.h>
#endif
namespace DA
{
DAZipArchiveTask_DAData::DAZipArchiveTask_DAData() : DAAbstractArchiveTask()
{
}

DAZipArchiveTask_DAData::DAZipArchiveTask_DAData(const DAData& data) : DAAbstractArchiveTask()
{
    setData(data);
}

DAZipArchiveTask_DAData::~DAZipArchiveTask_DAData()
{
}

DAData DAZipArchiveTask_DAData::getData() const
{
    return mData;
}

void DAZipArchiveTask_DAData::setData(const DAData& data)
{
    mData = data;
}

bool DAZipArchiveTask_DAData::exec(DAAbstractArchive* archive, DAAbstractArchiveTask::Mode mode)
{
    if (!archive) {
        return false;
    }
    DAZipArchive* zip = static_cast< DAZipArchive* >(archive);
    if (mode == DAAbstractArchiveTask::WriteMode) {
        // 写模式
        if (!saveData(zip, mData)) {
            return false;
        }
    } else {
        // 读取数据模式
        // 读取数据模式
        if (!zip->isOpened()) {
            if (!zip->open()) {
                qDebug() << QString("open archive error:%1").arg(zip->getBaseFilePath());
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 保存数据
 * @param archive
 * @return
 */
bool DAZipArchiveTask_DAData::saveData(DAZipArchive* archive, const DAData& data)
{
// 保存过程先把文件输出为pkl到临时目录下
#if DA_ENABLE_PYTHON
    if (data.isDataFrame()) {
        // 保存为dataframe内容
        try {
            //! 由于此函数是在别的线程，因此必须获取gil
            pybind11::gil_scoped_acquire acquire;

            //! 首先保存为pkl
            DAPyDataFrame df = data.toDataFrame();
            DAPyScriptsDataFrame dfScript;
            QString path = mTempDir.filePath(data.getName());
            if (!dfScript.to_pickle(df, path)) {
                qDebug() << QString("An exception occurred while saving the data file %1").arg(data.getName());  // cn:数据文件%1保存时出现异常
                return false;
            }
            //! 把保存的pkl写入压缩包中
            if (!archive->isOpened()) {
                if (!archive->create()) {
                    qDebug() << QString("create archive error:%1").arg(archive->getBaseFilePath());
                    return false;
                }
            }
            if (!archive->writeFileToZip(QString("/datas/dataframe/%1").arg(data.getName()), path)) {
                qDebug() << QString("write %1 to /datas/dataframe/%2 error").arg(path, data.getName());
                return false;
            }
        } catch (const std::exception& e) {
            qDebug() << QString("An exception occurred while saving the data file %1,reason is %2")
                            .arg(data.getName())
                            .arg(e.what());  // cn:数据文件%1保存时出现异常
            return false;
        }
    }
#endif
    return true;
}
}  // end DA
