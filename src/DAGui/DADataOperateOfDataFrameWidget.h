#ifndef DADATAOPERATEOFDATAFRAMEWIDGET_H
#define DADATAOPERATEOFDATAFRAMEWIDGET_H
#include <QtWidgets/QWidget>
#include <QUndoStack>
#include <optional>
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include "numpy/DAPyDType.h"
#include "DATableDisplayFormat.h"
#include "DADataOperatePageWidget.h"
namespace Ui
{
class DADataOperateOfDataFrameWidget;
}

namespace DA
{
class DADataTableModel;
class DADataTableView;
class DADialogDataframeColumnCastToNumeric;
class DADialogDataframeColumnCastToDatetime;
class DADialogDataframeColumnDescribe;
class DATableStyleManager;
class DATableStyleItemDelegate;
class DATableStyleRegistry;
class DATableCellStyle;


/**
 * @brief 针对DataFrame的操作窗口
 *
 */
class DAGUI_API DADataOperateOfDataFrameWidget : public DADataOperatePageWidget
{
    Q_OBJECT
public:
    int getDataOperatePageType() const override;

public:
    explicit DADataOperateOfDataFrameWidget(const DAData& d, DATableStyleRegistry* registry, QWidget* parent = nullptr);
    ~DADataOperateOfDataFrameWidget();
    // 是否存在data
    bool haveData() const;
    // 获取dataframe
    DAPyDataFrame getDataframe() const;
    // 获取Data的引用
    const DAData& data() const;
    // 创建dataframe的数据描述
    DAPyDataFrame createDataDescribe();
    // dataframe表格是否有选中项
    bool isDataframeTableHaveSelection() const;
    // 返回选中的列数，列数不会重复
    QList< int > getSelectedDataframeCoumns(bool ensureInDataframe = true) const;
    QList< int > getSelectedDataframeRows(bool ensureInDataframe = true) const;
    // 返回选中的列/行
    QList< int > getFullySelectedDataframeColumns(bool ensureInDataframe = true) const;
    QList< int > getFullySelectedDataframeRows(bool ensureInDataframe = true) const;

    QList< QPoint > getSelectedDataframeCells(bool ensureInDataframe = true) const;
    int getSelectedOneDataframeRow(bool ensureInDataframe = true) const;
    int getSelectedOneDataframeColumn(bool ensureInDataframe = true) const;
    // 获取tableview
    DADataTableView* getDataTableView() const;

    // 获取样式管理器
    DATableStyleManager* styleManager() const;
    // 合并样式片段到选中区（fragment 只设部分属性，merge 到目标已有样式）
    // 自动判定层级：整列→列级，整行→行级，否则单元格级
    void mergeStyleToSelection(const DATableCellStyle& fragment);
    // 设置完整样式到选中区（传入什么就设置什么，整体替换，为格式刷准备）
    // 自动判定层级：整列→列级，整行→行级，否则单元格级
    void applyStyleToSelection(const DATableCellStyle& style);
    // 清除选中区样式（自动判定层级）
    void clearStyleSelection();
    // 清除整表所有样式
    void clearStyleAll();
    // 获取选中区代表的样式（各属性独立判断一致性，不一致的属性 valid=false）
    DATableCellStyle getCurrentCellStyle() const;

    // 设置列显示格式到选中列（列级，可撤销）
    void setDisplayFormatToSelection(const DATableDisplayFormat& fmt);
    // 清除选中列的显示格式（列级，保留 bg/fg/font，可撤销）
    void clearDisplayFormatFromSelection();
    // 获取选中列代表的显示格式（不一致返回 invalid）
    DATableDisplayFormat getCurrentColumnDisplayFormat() const;

    // 获取选中的序列，如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，如果用户选中了多列，那么每列作为一个DAData并组成list返回
    QList< DAData > getSlectedSeries() const;
    // 刷新表格
    void refreshTable();
    // 确保列可见
    void ensureColumnVisible(const QString& colName, bool selectCol = true);
Q_SIGNALS:
    // 选中区变化时发射当前代表样式，供 ribbon 控件反向同步
    void currentStyleChanged(const DA::DATableCellStyle& style);
    // 选中列变化时发射当前代表显示格式，供 ribbon 格式下拉框反向同步
    void currentDisplayFormatChanged(const DA::DATableDisplayFormat& fmt);
    /**
     * @brief 表格水平表头单击
     * @param logicalIndex 列逻辑索引
     * @note 用于"选择序列"窗口拾取表格列
     */
    void columnHeaderClicked(int logicalIndex);
public Q_SLOTS:
    void setDAData(const DA::DAData& d);
    // 在选中行后面插入行
    void insertRowAboveBySelect();
    void insertRowBelowBySelect();
    void insertRowAt(int row);
    void insertColumnRightBySelect();
    void insertColumnLeftBySelect();
    void insertColumnAt(int col);
    // 移除选中的行
    int removeSelectRow();
    // 移除选择的列
    int removeSelectColumn();
    // 设置选中单元格为nan,返回设置成功的个数
    int removeSelectCell();
    // 列更名
    void renameColumns();
    // 重命名单列（表头右键用），col 为列位置索引，newName 为新列名
    // 复用 DACommandDataFrame_renameColumns 命令，支持 undo/redo
    // @return 成功返回 true（列为空/重名/越界返回 false）
    bool renameColumn(int col, const QString& newName);
    // 设置选择列的数据类型，发射信号columnTypeChanged
    bool changeSelectColumnType(const DAPyDType& dt);
    // 把选择的行转换为数值，带交互
    void castSelectToNum();
    void castSelectToDatetime();
    // 把选择的列转换为索引
    bool changeSelectColumnToIndex();
    // 显示列统计信息（表头右键），通过 pandas describe 获取
    void showColumnDescribe(int col);
Q_SIGNALS:
    /**
     * @brief 选中的列或者类型发生了变化
     * @param column 列 如果返回空说明影响的列不确定,如果多选，返回
     * @param dt 类型
     * @note 此函数主要是通知主界面ribbon上面的类型变化，调用setDataframeOperateCurrentDType
     */
    void selectTypeChanged(const QList< int >& column, DA::DAPyDType dt);
private Q_SLOTS:
    // 表格点击
    void onTableViewClicked(const QModelIndex& index);

protected:
    void changeEvent(QEvent* e) override;

private:
    Ui::DADataOperateOfDataFrameWidget* ui;
    DAData mData;
    DADataTableModel* mModel { nullptr };
    DATableStyleManager* mStyleManager { nullptr };  ///< 借自 DATableStyleRegistry，非拥有（随数据存在，widget 关闭不销毁）
    DATableStyleItemDelegate* mStyleDelegate { nullptr };

    DADialogDataframeColumnCastToNumeric* mDialogCastNumArgs { nullptr };
    DADialogDataframeColumnCastToDatetime* mDialogCastDatetimeArgs { nullptr };
    DADialogDataframeColumnDescribe* mDialogColumnDescribe { nullptr };
};
}  // end of namespace DA
#endif  // DADATAOPERATEOFDATAFRAMEWIDGET_H
