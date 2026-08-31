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
#include "DALogCategory.h"
#include "DAPyScripts.h"
#include "pandas/DAPyDataFrame.h"

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
    daInfo << tr("Begin importing file: %1").arg(f);  // cn:开始导入文件:%1
    try {
        if (DAPyScripts::isInitScripts()) {
            DAPyScripts::getIO().read_and_add_to_datamanager(f, args, err);
            return true;
        }
        daWarning << tr("Python scripts not initialized, cannot import file: %1").arg(f);  // cn:Python脚本未初始化，无法导入文件:%1
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
    return false;
}

/**
 * @brief 导出数据到文件
 *
 * 支持四格式（csv/excel/pickle/parquet），调用 io.py 的 da_to_* 系列（args 字典版），
 * 与 importFromFile 对称
 * @param d 待导出的数据（须为 DataFrame）
 * @param f 目标文件路径
 * @param format 小写格式名：csv/excel/pickle/parquet
 * @param err 失败原因出参
 * @return 成功返回 true
 */
bool DAAppDataManager::exportToFile(const DAData& d, const QString& f, const QString& format, QString* err)
{
    if (!d.isDataFrame()) {
        if (err) {
            *err = tr("Only DataFrame data can be exported");  // cn:仅支持导出 DataFrame 数据
        }
        return false;
    }
    try {
        if (!DAPyScripts::isInitScripts()) {
            if (err) {
                *err = tr("Python scripts not initialized");  // cn:Python 脚本未初始化
            }
            daWarning << tr("Python scripts not initialized, cannot export file: %1").arg(f);  // cn:Python脚本未初始化，无法导出文件:%1
            return false;
        }
        DAPyDataFrame df = d.toDataFrame();
        if (df.isNone()) {
            if (err) {
                *err = tr("Data is empty, cannot export");  // cn:数据为空，无法导出
            }
            return false;
        }
        DAPyScriptsIO& io = DAPyScripts::getIO();
        // 默认不带 index 导出（对齐用户直觉），特殊需求可经 args 扩展
        QVariantMap args { { "index", false } };
        bool ok = false;
        if (format == "csv") {
            ok = io.to_csv(df, f, args, err);
        } else if (format == "excel") {
            ok = io.to_excel(df, f, args, err);
        } else if (format == "pickle") {
            ok = io.to_pickle(df, f, QVariantMap(), err);  // to_pickle 无 index 参数
        } else if (format == "parquet") {
            ok = io.to_parquet(df, f, QVariantMap(), err);  // to_parquet 无 index 参数
        } else {
            if (err) {
                *err = tr("Unsupported export format: %1").arg(format);  // cn:不支持的导出格式:%1
            }
            return false;
        }
        return ok;
    } catch (const std::exception& e) {
        if (err) {
            *err = QString::fromStdString(e.what());
        }
        qCritical() << e.what();
    }
    return false;
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
    for (const QString& f : std::as_const(fileNames)) {
        qInfo() << QString("Begin import file: %1").arg(f);  // cn:开始导入文件:%1
        if (!DAPyInterpreter::isPythonInitialized()) {
            return 0;
        }
        DAPyObjectWrapper res = DAPyScripts::getIO().read(f);
        if (DAPyDataFrame::isDataFrame(res.object())) {
            qInfo() << QString("File: %1, convert to DataFrame").arg(f);  // cn:文件:%1，转换为DataFrame
            QFileInfo fi(f);
            DAPyDataFrame df = res;  // 调用的是DAPyDataFrame(const DAPyObjectWrapper& df)
            if (df.size() == 0) {
                qWarning() << QString("The file '%1' has been successfully imported, "
                                 "but no data can be read from the file")  // cn:导入文件'%1'成功，但无法从文件中读取到数据
                                  .arg(f);
                continue;
            }
            DAData data = df;
            data.setName(fi.baseName());
            data.setDescribe(fi.absoluteFilePath());
            importDatas.append(data);
        }  // else if() //其他格式
        else if (res.isNone()) {
            qWarning() << QString("Cannot import file: %1").arg(f);  // cn:无法导入文件:%1
            continue;
        }
    }
    if (importDatas.size() > 0) {
        addDatas(importDatas);
    }
    return importDatas.size();
#else
    qDebug() << "data manager begin import files:" << fileNames;
    int successCnt = 0;
    for (const QString& f : std::as_const(fileNames)) {
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
    return dmw->getAllSelectDatas();
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
