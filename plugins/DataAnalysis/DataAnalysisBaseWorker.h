#ifndef DATAANALYSISBASEWORKER_H
#define DATAANALYSISBASEWORKER_H
#include <QObject>
#include <QMainWindow>
namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DADockingAreaInterface;
class DADataManagerInterface;
class DACommandInterface;
class DAProjectInterface;
}

/**
 * @brief 本插件的基础工作类
 */
class DataAnalysisBaseWorker : public QObject
{
    Q_OBJECT
public:
    explicit DataAnalysisBaseWorker(QObject* par = nullptr);
    ~DataAnalysisBaseWorker();
    virtual void initialize(DA::DACoreInterface* core);
    //
    DA::DACoreInterface* core() const;
    DA::DAUIInterface* uiInterface() const;
    DA::DADockingAreaInterface* dockAreaInterface() const;
    DA::DADataManagerInterface* dataManagerInterface() const;
    DA::DACommandInterface* commandInterface() const;
    DA::DAProjectInterface* projectInterface() const;
    QMainWindow* mainWindow();

private:
    DA::DACoreInterface* m_core { nullptr };
    DA::DAUIInterface* m_ui { nullptr };
    DA::DADockingAreaInterface* m_dockArea { nullptr };
    DA::DADataManagerInterface* m_dataManager { nullptr };
    DA::DACommandInterface* m_cmd { nullptr };
    DA::DAProjectInterface* m_project { nullptr };
};

#endif  // DATAANALYSISBASEWORKER_H
