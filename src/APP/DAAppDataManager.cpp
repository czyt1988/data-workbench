#include "DAAppDataManager.h"
#include <QList>
#include <QFileInfo>
#include <QUndoStack>
#include <QDebug>
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManageWidget.h"
#include "DADataOperateWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
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

/**
 * @brief 获取当前选中的数据，此函数要基于界面数据管理器选择的数据返回
 *  * 当前选中的数据是指数据管理窗口正在选中的数据，如果没有选中任何数据，返回一个空列表
 * @return
 */
QList< DAData > DAAppDataManager::getSelectDatas() const
{
    DA::DADataManageWidget* dmw = core()->getUiInterface()->getDockingArea()->getDataManageWidget();
    if (!dmw) {
        return QList< DAData >();
    }
    return dmw->getCurrentSelectDatas();
}

/**
 * @brief 获取当前正在操作的数据，当前正在操作的数据是指当前正在打开的表格所对应的数据
 *  * 当前正在操作的数据是指数据操作表格正在操作的数据，如果当前没有打开任何数据，此函数返回一个空的DAData
 * @return
 */
DAData DAAppDataManager::getOperateData() const
{
    DADataOperateWidget* dataOperateWidget = core()->getUiInterface()->getDockingArea()->getDataOperateWidget();
    if (!dataOperateWidget) {
        return DAData();
    }
    DADataOperateOfDataFrameWidget* dfWidget = dataOperateWidget->getCurrentDataFrameWidget();
    if (!dfWidget) {
        return DAData();
    }
    return dfWidget->data();
}

/**
 * @brief 获取当前正在操作窗口操作的列名
 *
 * 如果用户当前正在操作一个表格，且选中了某几列，那么此函数会返回选中的列名
 * 结合@ref getOperateData 和此函数，即可获取当前用户正在操作的序列
 * @sa getOperateData
 * @return 如果没有选中任何列，返回空列表
 */
QList< int > DAAppDataManager::getOperateDataSeries() const
{
    DADataOperateWidget* dataOperateWidget = core()->getUiInterface()->getDockingArea()->getDataOperateWidget();
    if (!dataOperateWidget) {
        return QList< int >();
    }
    DADataOperateOfDataFrameWidget* dfWidget = dataOperateWidget->getCurrentDataFrameWidget();
    if (!dfWidget) {
        return QList< int >();
    }
    return dfWidget->getSelectedDataframeCoumns();
}

}  // end DA
