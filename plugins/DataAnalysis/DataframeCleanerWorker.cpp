#include "DataframeCleanerWorker.h"
#include <QUndoStack>
#include <QUndoGroup>
#include <QPointer>
#include "DAPyModule.h"
#include "DAPybind11QtTypeCast.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataOperateWidget.h"
#include "DADataUndoCommand.h"
#include "DACommandInterface.h"
#include "DADataOperateOfDataFrameWidget.h"

class DataOperateCommand : public DA::DADataUndoCommand
{
public:
    DataOperateCommand(QUndoCommand* par = nullptr) : DA::DADataUndoCommand(par)
    {
    }
    ~DataOperateCommand()
    {
    }
    void setTableWidget(DA::DADataOperateOfDataFrameWidget* w)
    {
        m_tableWidget = w;
    }
    void redo()
    {
        DA::DADataUndoCommand::redo();
        if (m_skipFirst) {
            m_skipFirst = false;
            return;
        }
        if (m_tableWidget) {
            m_tableWidget->refreshTable();
        }
    }
    void undo()
    {
        DA::DADataUndoCommand::undo();
        if (m_tableWidget) {
            m_tableWidget->refreshTable();
        }
    }

public:
    bool m_skipFirst { true };
    QPointer< DA::DADataOperateOfDataFrameWidget > m_tableWidget;
};

DataframeCleanerWorker::DataframeCleanerWorker(QObject* par) : DataAnalysisBaseWorker(par)
{
    initializePythonEnv();
}

DataframeCleanerWorker::~DataframeCleanerWorker()
{
}

bool DataframeCleanerWorker::initializePythonEnv()
{
    try {
        DA::DAPyModule DADataAnalysis("DADataAnalysis");
        m_dataCleanerModule  = std::make_unique< DA::DAPyModule >();
        *m_dataCleanerModule = DADataAnalysis.attr("dataframe_cleaner");
        return true;
    } catch (const std::exception& e) {
        m_dataCleanerModule.reset();
        qCritical() << e.what();
    }
    return false;
}

void DataframeCleanerWorker::dropna()
{
    try {
        // 获取回退栈
        auto dataOptWidget                           = dockAreaInterface()->getDataOperateWidget();
        QUndoStack* undoStack                        = dataOptWidget->getUndoStack();
        DA::DADataOperateOfDataFrameWidget* dfWidget = dataOptWidget->getCurrentDataFrameWidget();
        // 获取要处理的数据
        QList< int > selectColumnIndexs = dfWidget->getSelectedDataframeCoumns();
        DA::DAData data                 = dfWidget->data();
        // 把数据记录到回退栈
        std::unique_ptr< DataOperateCommand > cmd = std::make_unique< DataOperateCommand >();
        cmd->setOldData(data);
        cmd->setTableWidget(dfWidget);
        pybind11::list list = DA::PY::toPyList(selectColumnIndexs);
        auto dropna         = m_dataCleanerModule->attr("dropna");
        auto drop_count     = dropna(data, list);
        if (drop_count.is_none()) {
            return;
        }
        int v = drop_count.cast< int >();
        uiInterface()->addInfoLogMessage(tr("Removed %1 rows containing NaN values").arg(v));  // cn:删除了%1个含Nan值的行
        // 设置新数据
        cmd->setNewData(data);
        undoStack->push(cmd.release());
        commandInterface()->undoGroup().setActiveStack(undoStack);
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return;
    }
}
