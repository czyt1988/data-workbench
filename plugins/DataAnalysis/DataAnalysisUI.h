#ifndef DATAANALYSISUI_H
#define DATAANALYSISUI_H
#include <QObject>
#include <QAction>

// SARibbon
class SARibbonPanel;

// DA
namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DAActionsInterface;
}

class DataframeIOWorker;

class DataAnalysisUI : public QObject
{
    Q_OBJECT
public:
    explicit DataAnalysisUI(QObject* par = nullptr);
    virtual ~DataAnalysisUI() override;
    // 初始化
    bool initialize(DA::DACoreInterface* core);
    //
    void retranslateUi();
    // 绑定信号槽
    void bind(DataframeIOWorker* io);

private:
    QAction* createAction(const char* objname, const char* iconpath);
    // 构建Data标签页
    void buildDataCategory();

public:
    // 这里你可以用于构建你的界面，保存你的action
    //===================================================
    // 数据标签 Data Category
    //===================================================
    QAction* actionExportIndividualData { nullptr };  ///< 导出单个数据
    QAction* actionExportMultipleData { nullptr };    ///< 导出多个数据
    QAction* actionExportToOneExcel { nullptr };      ///< 把数据导出到一个excel中

    //===================================================
    // 数据标签 Dataframe Context Category
    //===================================================
    SARibbonPanel* panelDataCleaner { nullptr };
    // 数据清洗
    QAction* actionDataFrameDropNone { nullptr };

private:
    DA::DACoreInterface* m_core { nullptr };
    DA::DAUIInterface* m_ui { nullptr };
    DA::DAActionsInterface* m_actions { nullptr };
};

#endif  // DATAANALYSISPLUGIN_H
