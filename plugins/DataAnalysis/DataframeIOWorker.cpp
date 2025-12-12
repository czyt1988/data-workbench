#include "DataframeIOWorker.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include "DAData.h"
#include "DAWaitCursorScoped.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
#include "DAStatusBarInterface.h"
#include "Dialogs/DataframeExportSettingsDialog.h"
#include "DAPyModule.h"
DataframeIOWorker::DataframeIOWorker(QObject* par) : DataAnalysisBaseWorker(par)
{
    initializePythonEnv();
}

DataframeIOWorker::~DataframeIOWorker()
{
}

bool DataframeIOWorker::initializePythonEnv()
{
    try {
        DA::DAPyModule DADataAnalysis("DADataAnalysis");
        m_dataAnalysisModule  = std::make_unique< DA::DAPyModule >();
        *m_dataAnalysisModule = DADataAnalysis.attr("dataframe_io");

        DA::DAPyModule DAWorkbench("DAWorkbench");
        m_threadStatusMgrModule  = std::make_unique< DA::DAPyModule >();
        *m_threadStatusMgrModule = DAWorkbench.attr("thread_status_manager");
        return true;

    } catch (const std::exception& e) {
        m_dataAnalysisModule.reset();
        m_threadStatusMgrModule.reset();
        qCritical() << e.what();
    }
    return false;
}

/**
 * @brief 导出单一数据
 */
void DataframeIOWorker::exportIndividualData()
{
    // 获取当前Data
    QList< DA::DAData > selDatas = dataManagerInterface()->getSelectDatas();
    if (selDatas.empty()) {
        uiInterface()->addWarningLogMessage(
            tr("No data is selected. Please select the data to export first."));  // cn:没有选中任何数据，请先选中要导出的数据
        return;
    }
    QString dataPath = QFileDialog::getSaveFileName(
        mainWindow(),
        tr("Export Data"),  // 导出数据
        QString(),
        tr("Text Files (*.txt *.csv);;Excel Files (*.xlsx);;Python Files (*.pkl);;All Files(*.*)")  // 数据文件
    );
    if (dataPath.isEmpty()) {
        // 取消退出
        return;
    }
    DA_WAIT_CURSOR_SCOPED_NS();

    QFileInfo fi(dataPath);
    QString dataName     = fi.completeBaseName();
    QString dataSuffix   = fi.suffix();
    QString baseDir      = fi.absolutePath();
    QString dataFilePath = QString("%1/%2.%3").arg(baseDir, dataName, dataSuffix);
    DA::DAData data      = selDatas.first();
    if (data.isDataFrame()) {
        if (!DA::DAData::exportToFile(data, dataFilePath)) {
            uiInterface()->addInfoLogMessage(
                tr("An exception occurred while serializing the dataframe named %1").arg(dataFilePath));  // cn:把名称为%1的dataframe序列化时出现异常
        } else {
            uiInterface()->addInfoLogMessage(
                tr("Data %1 successfully exported to %2.").arg(data.getName(), dataFilePath));  // cn:成功导出数据到%1
        }
    }
}

void DataframeIOWorker::exportMultipleData()
{
    if (!m_dataAnalysisModule) {
        return;
    }
    if (!m_exportSettingDialog) {
        m_exportSettingDialog = new DataframeExportSettingsDialog(mainWindow());
    }
    if (QDialog::Accepted != m_exportSettingDialog->exec()) {
        return;
    }
    std::string path = m_exportSettingDialog->getSavePath().toStdString();
    std::string type = m_exportSettingDialog->getSelectSuffix().toStdString();
    bool isExportAll = m_exportSettingDialog->isExportAll();
    try {
        auto process_zip_data_thread = m_dataAnalysisModule->attr("export_datamanager_thread");
        auto taskid                  = process_zip_data_thread(path, type, isExportAll);
        if (taskid.is_none()) {
            return;
        }
        m_exportDatasTaskID = taskid.cast< std::string >();
        // 进行任务进度查询
        onExportMultipleDataTask();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
}

void DataframeIOWorker::onExportMultipleDataTask()
{
    if (!m_threadStatusMgrModule) {
        return;
    }
    try {
        {
            DA::DAWaitCursorScoped cursorScoped;
            // 在调用Python函数之前释放GIL
            pybind11::gil_scoped_release release;

            // 短暂休眠，让Python线程有机会获得GIL
            QThread::msleep(500);
        }
        // 休眠过后开始查询状态，看看python处理的结果
        //  调用Python函数检查状态
        auto get_task_status  = m_threadStatusMgrModule->attr("get_task_status");
        pybind11::dict status = get_task_status(m_exportDatasTaskID);  // 通过任务id获取线程状态
                                                                       /*
                                                                           "task_id": self._task_id,
                                                                           "task_name": self._task_name,
                                                                           "is_running": self._is_processing and not self._is_paused and not self._is_canceled,
                                                                           "is_paused": self._is_paused,
                                                                           "is_canceled": self._is_canceled,
                                                                           "is_success": self._is_success,
                                                                           "current_stage": self._current_stage,
                                                                           "progress": round(self._progress, 2),
                                                                           "elapsed_seconds": elapsed,
                                                                           "message": self._message,
                                                                           "custom_data": self._custom_data.copy()  # 自定义数据副本
                                                                        */
        DA::DAStatusBarInterface* statusBar = uiInterface()->getStatusBar();
        bool is_running                     = status[ "is_running" ].cast< bool >();
        if (is_running) {
            // 说明线程还在运行
            // 构建进度文本
            double progress        = status[ "progress" ].cast< double >();  // 百分比进度
            std::string message    = status[ "message" ].cast< std::string >();
            double elapsed_seconds = status[ "elapsed_seconds" ].cast< double >();

            int elapsed_min = static_cast< int >(elapsed_seconds / 60);
            int elapsed_sec = static_cast< int >(elapsed_seconds) % 60;
            QString progress_text;
            progress_text = QString("%1 已用时: %2:%3")
                                .arg(QString::fromStdString(message))
                                .arg(elapsed_min, 2, 10, QChar('0'))
                                .arg(elapsed_sec, 2, 10, QChar('0'));
            statusBar->setProgress(progress);
            statusBar->setProgressText(progress_text);
            // 关键，在20ms后继续查询
            QTimer::singleShot(20, this, &DataframeIOWorker::onExportMultipleDataTask);
        } else {
            // 说明线程已经运行完成
            bool is_success = status[ "is_success" ].cast< bool >();
            if (is_success) {
                statusBar->showMessage(QString(u8"导入数据完成!"));
                // 获取状态的自定义数据
                double elapsed_seconds = status[ "elapsed_seconds" ].cast< double >();
                int elapsed_min        = static_cast< int >(elapsed_seconds / 60);
                int elapsed_sec        = static_cast< int >(elapsed_seconds) % 60;
                // 生成日志
                QString message = QString(tr("Finish Export,Cost %1:%2")).arg(elapsed_min).arg(elapsed_sec);
                qInfo() << message;
            } else {
                statusBar->showMessage(QString(u8"导入数失败!"));
                qCritical().noquote() << QString(u8"导入数据失败!");
            }
            statusBar->hideProgressBar();
            statusBar->setBusy(false);
        }
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
}
