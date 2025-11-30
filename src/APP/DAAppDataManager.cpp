#include "DAAppDataManager.h"
#include <QList>
#include <QFileInfo>
#include <QUndoStack>
#include <QDebug>
// DAUtils
#include "DAStringUtil.h"
#if DA_ENABLE_PYTHON
// DAPyScript
#include "DAPyScripts.h"
#endif

namespace DA
{

//===================================================
// DAAppDataManager
//===================================================

DAAppDataManager::DAAppDataManager(DACoreInterface* c, QObject* p) : DADataManagerInterface(c, p)
{
}

DAAppDataManager::~DAAppDataManager()
{
}

bool DAAppDataManager::importFromFile(const QString& f, const QVariantMap& args, QString* err)
{
#if DA_ENABLE_PYTHON
    qInfo() << tr("begin import file:%1").arg(f);
    try {
        DAPyScripts::getInstance().getIO().read_and_add_to_datamanager(f, args, err);
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
#endif
    return true;
}

/**
 * @brief 从文件导入数据
 * @param files 文件
 * @return 如果成功导入，返回导入的数量，如果返回0，说明没有导入成功
 */
int DAAppDataManager::importFromFiles(const QStringList& fileNames)
{
#if 0
    qDebug() << "data manager begin import files:" << fileNames;
    QList< DAData > importDatas;
    for (const QString& f : qAsConst(fileNames)) {
#if DA_ENABLE_PYTHON
        qInfo() << tr("begin import file:%1").arg(f);
        DAPyObjectWrapper res = DAPyScripts::getInstance().getIO().read(f);
        if (DAPyDataFrame::isDataFrame(res.object())) {
            qInfo() << tr("file:%1,conver to dataframe").arg(f);
            QFileInfo fi(f);
            DAPyDataFrame df = res;  // 调用的是DAPyDataFrame(const DAPyObjectWrapper& df)
            if (df.size() == 0) {
                qWarning() << tr("The file '%1' has been successfully imported, "
                                 "but no data can be read from the file")  // cn: 导入文件'%1'成功，但无法从文件中读取到数据
                                  .arg(f);
                continue;
            }
            DAData data = df;
            data.setName(fi.baseName());
            data.setDescribe(fi.absoluteFilePath());
            importDatas.append(data);
        }  // else if() //其他格式
        else if (res.isNone()) {
            qWarning() << tr("can not import file:%1").arg(f);
            continue;
        }
#endif
    }
    if (importDatas.size() > 0) {
        addDatas(importDatas);
    }
    return importDatas.size();
#else
    qDebug() << "data manager begin import files:" << fileNames;
    int successCnt = 0;
    for (const QString& f : qAsConst(fileNames)) {
        if (importFromFile(f)) {
            ++successCnt;
        }
    }
    return successCnt;
#endif
}

}  // end DA
