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
class DataframeCleanerWorker;
class DataframeOperateWorker;

/**
 * @brief 负责处理ui相关事务
 */
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
    void bind(DataframeIOWorker* worker);
    void bind(DataframeCleanerWorker* worker);
    void bind(DataframeOperateWorker* worker);

private:
    // 构建Data标签页
    void buildDataCategory();

public:
    // 这里你可以用于构建你的界面，保存你的action
    //===================================================
    // 数据标签 Data Category
    //===================================================
    SARibbonPanel* panelDataOperate { nullptr };
    QAction* actionExportIndividualData { nullptr };  ///< 导出单个数据
    QAction* actionExportMultipleData { nullptr };    ///< 导出多个数据
    QAction* actionExportToOneExcel { nullptr };      ///< 把数据导出到一个excel中

    //===================================================
    // 数据标签 Dataframe Context Category
    //===================================================
    SARibbonPanel* panelDataCleaner { nullptr };
    // 数据清洗 panel
    QAction* actionDataFrameDropNone { nullptr };              ///< 删除Nan值
    QAction* actionDropDuplicates { nullptr };                 ///< 重复值处理
    QAction* actionDataFrameFillNone { nullptr };              ///< 填充缺失值
    QAction* actionDataFrameFillInterpolate { nullptr };       ///< 插值法填充缺失值
    QAction* actionDataFrameRemoveOutlierIQR { nullptr };      ///< 基于IQR方法移除异常值
    QAction* actionDataFrameRemoveOutliersZScore { nullptr };  ///< 基于Z-score替换异常值
    QAction* actionDataFrameTransformSkewedData { nullptr };   ///< 转换偏态数值数据以改善分布

    // Statistic Panel 数据统计panel
    SARibbonPanel* panelDataStatistic { nullptr };
    QAction* actionCreateDataDescribe;  ///< 数据描述
    QAction* actionCreatePivotTable;    ///< 创建数据透视表
private:
    DA::DACoreInterface* m_core { nullptr };
    DA::DAUIInterface* m_ui { nullptr };
    DA::DAActionsInterface* m_actions { nullptr };
};

#endif  // DATAANALYSISPLUGIN_H
