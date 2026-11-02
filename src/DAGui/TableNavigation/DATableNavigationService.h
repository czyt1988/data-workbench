#ifndef DATABLENAVIGATIONSERVICE_H
#define DATABLENAVIGATIONSERVICE_H
#include "DAGuiAPI.h"
#include "Dialog/DADialogGotoCell.h"
#include <functional>
#include <QObject>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;

/**
 * @brief 表格导航服务（008 期5，007 的定位入口）
 *
 * showGotoDialog：定位对话框（懒建复用；setup 重置初值）；resolveGotoRequest：
 * 行/列解析（首末行/序号/索引标签/列名）→ locateActualCell 定位单入口。
 * 定位执行（locateActualCell/ensureColumnVisible）在页面侧（依赖冻结区处置与
 * 列视图取消隐藏），经宿主桥 locateActualCell 回调。
 * 生命周期：宿主页面构造并持有（QObject 父子持有定位对话框）。
 */
class DAGUI_API DATableNavigationService : public QObject
{
    Q_OBJECT
public:
    DATableNavigationService(const DATablePageContext& ctx,
                             const DATableSelectionService& sel,
                             const std::function< void(int, int, bool, bool) >& locate,
                             QObject* parent = nullptr);

public:
    // 打开定位对话框（无数据静默返回；确定后解析请求并定位）
    void showGotoDialog();
    // 解析定位请求并调定位单入口；返回是否定位成功（已给用户提示）
    bool resolveGotoRequest(const DADialogGotoCell::Request& req);

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    std::function< void(int, int, bool, bool) > mLocate;  ///< 定位单入口宿主桥（actualRow, col, select, keepHidden）
    DADialogGotoCell* mGotoDialog { nullptr };  ///< 定位对话框（007，懒建复用）
};
}  // end of namespace DA
#endif  // DATABLENAVIGATIONSERVICE_H
