#ifndef DADIALOGGROUPBY_H
#define DADIALOGGROUPBY_H
#include <QDialog>
#include <QHash>
#include <QStringList>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"

class QListWidget;
class QTableWidget;
class QLabel;
class QPushButton;
class QTimer;

namespace DA
{
/**
 * @brief 分组聚合对话框（004 T1.22，v1 只出长格式结果表）
 *
 * 交互（计划 §5.3）：左侧分组键多选 + 聚合列多选 + 每列聚合函数选择
 * （mean/sum/size，与透视表 da_create_pivot_table 对齐）；右侧预览（前 N 行）
 * + 预估结果行数（分组数）。参数变化 300ms 去抖后对**全表**跑同一算子
 * da_groupby_aggregate（忙等待光标；groupby 结果与样本无关，预览即所得），
 * 结果缓存在对话框内，接受后由页面 widget 直接取用（不重复聚合）。
 *
 * 产出为新数据集（不改当前表）：命名 xxx_GroupBy，注册走 DADataManager::addData_
 * （可撤销）。折叠视图不在本 WP 范围（R15，独立文档）。
 */
class DAGUI_API DADialogGroupBy : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogGroupBy(const DAPyDataFrame& df, QWidget* parent = nullptr);
    ~DADialogGroupBy();

    // 接受后的完整聚合结果（预览同一份，所见即所得；无有效结果为 none）
    DAPyDataFrame result() const;
    // 分组键列名（对话框选择）
    QStringList groupKeys() const;
    // 结果行数（= 分组数）
    int resultRowCount() const;

private Q_SLOTS:
    // 聚合列选择变化 → 重建函数表并触发去抖聚合
    void onValueSelectionChanged();
    void onInputChanged();

private:
    void setupUI();
    // 按当前选中的聚合列重建「列名+函数」表（保留既有函数选择）
    void rebuildFuncTable();
    // 全表聚合 + 预览（同一算子 da_groupby_aggregate）；更新状态与 OK 可用性
    void runAggregate();
    void setStatusError(const QString& msg);
    void setStatusOk(const QString& msg);

    DAPyDataFrame mDf;
    QListWidget* mListKeys { nullptr };    ///< 分组键（多选）
    QListWidget* mListValues { nullptr };  ///< 聚合列（多选）
    QTableWidget* mTableFuncs { nullptr }; ///< 每列聚合函数（列名 + 下拉）
    QTableWidget* mTablePreview { nullptr };
    QLabel* mLabelStatus { nullptr };
    QPushButton* mButtonOk { nullptr };
    QTimer* mDebounceTimer { nullptr };
    QHash< QString, QString > mFuncChoice;  ///< 列名 → 已选函数（重建函数表时保留）
    DAPyDataFrame mResult;                  ///< 最近一次聚合结果
    int mResultRows { 0 };                  ///< 结果行数（分组数）
    static constexpr int c_previewRows = 10;  ///< 预览行数
};
}  // end of namespace DA
#endif  // DADIALOGGROUPBY_H
