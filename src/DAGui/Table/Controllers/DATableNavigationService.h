#ifndef DATABLENAVIGATIONSERVICE_H
#define DATABLENAVIGATIONSERVICE_H
#include "DAGuiAPI.h"
#include "Table/Dialogs/DADialogGotoCell.h"
#include <functional>
#include <QObject>
#include <QString>

class QModelIndex;

namespace DA
{
class DAPyDType;
class DATablePageContext;
class DATableSelectionService;
class DATableViewStateController;
class DATableColumnViewController;

/**
 * @brief 表格导航服务（008 期5，007 的定位入口）
 *
 * showGotoDialog：定位对话框（懒建复用；setup 重置初值）；resolveGotoRequest：
 * 行/列解析（首末行/序号/索引标签/列名）→ locateActualCell 定位单入口。
 * locateActualCell/ensureColumnVisible：定位执行（越界夹取 → 隐藏列处置 →
 * 纵向主窗格摆位 → 横向按冻结区分流）；onTableViewClicked/onHeaderClickedWithModifiers：
 * 点击分发（类型上报/拾取模式转发）。Mediator-light：冻结区判定经视图状态控制器、
 * 取消隐藏经列视图控制器。
 * 生命周期：宿主页面构造并持有（QObject 父子持有定位对话框）。
 */
class DAGUI_API DATableNavigationService : public QObject
{
    Q_OBJECT
public:
    DATableNavigationService(const DATablePageContext& ctx,
                             const DATableSelectionService& sel,
                             DATableViewStateController* viewState,
                             DATableColumnViewController* columnView,
                             QObject* parent = nullptr);

Q_SIGNALS:
    // 点击格类型上报（页面转发同名信号 → APP 同步 ribbon 类型显示）
    void selectTypeChanged(const QList< int >& columns, const DA::DAPyDType& dtype);
    // 拾取模式下的表头点击转发（页面转发同名信号 → 图表「选择序列」拾取链路）
    void columnHeaderClicked(int logicalIndex);

public:
    // 打开定位对话框（无数据静默返回；确定后解析请求并定位）
    void showGotoDialog();
    // 解析定位请求并调定位单入口；返回是否定位成功（已给用户提示）
    bool resolveGotoRequest(const DADialogGotoCell::Request& req);
    // 确保整列可见并滚动到位（隐藏列先取消隐藏+提示；selectCol=true 整列选中）
    void ensureColumnVisible(const QString& colName, bool selectCol);
    // 定位单入口（007 语义：数据行 + 列 + 是否选中 + 是否保持隐藏）
    void locateActualCell(int actualRow, int col, bool select, bool keepHidden);
    // 定位单入口的列名重载（重名取第一个同名列，数据列序）
    void locateActualCell(int actualRow, const QString& columnName, bool select, bool keepHidden);
    // 表体点击分发（类型上报；非 dataframe 空类型）
    void onTableViewClicked(const QModelIndex& index);
    // 表头带修饰键点击（拾取模式转发 columnHeaderClicked；非拾取模式无事）
    void onHeaderClickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers);

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableViewStateController* mViewState { nullptr };      ///< 视图状态控制器（冻结区判定）
    DATableColumnViewController* mColumnView { nullptr };    ///< 列视图控制器（隐藏列取消隐藏）
    DADialogGotoCell* mGotoDialog { nullptr };  ///< 定位对话框（007，懒建复用）
};
}  // end of namespace DA
#endif  // DATABLENAVIGATIONSERVICE_H
