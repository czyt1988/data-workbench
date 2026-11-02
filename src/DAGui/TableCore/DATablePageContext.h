#ifndef DATABLEPAGECONTEXT_H
#define DATABLEPAGECONTEXT_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include <QList>
#include <QString>
#include <QStringList>
#include <functional>

class QAbstractItemModel;
class QUndoCommand;
class QWidget;

namespace DA
{
class DADataTableModel;
class DADataTableView;
class DATablePaneContainer;
class DATableMetadata;
class DATableStyleManager;
class DATableViewStateStore;
class DATableHighlightOverlay;
class DATableConditionalFormatStore;
class DATableComputedColumnStore;
class DATableValidationStore;
class DATableNoteStore;

/**
 * @brief 表格页上下文：表格页协作对象的依赖注入载体（008 期1）
 *
 * 只做「聚合与转发」：模型、主窗格、元数据束与各子存储的指针聚合在一个值对象里，
 * 协作对象（选区服务/各控制器）经构造注入拿到它，不再持有具体窗口指针。
 * 边界（防新上帝类）：不允许出现业务分支、不允许弹窗、不允许构造命令；
 * 方法体超过 3 行就应该属于某个控制器。
 * 生命周期：由宿主页面（DADataOperateOfDataFrameWidget）构造并装配，非拥有任何指针。
 */
class DAGUI_API DATablePageContext
{
public:
    DATablePageContext();

public:
    // ===== 装配（宿主页面构造/换数据时调用；指针均为非拥有）=====
    void setModel(DADataTableModel* m);
    void setMainView(DADataTableView* v);
    void setPaneContainer(DATablePaneContainer* c);
    void setMetadata(DATableMetadata* md);
    void setHighlightOverlay(DATableHighlightOverlay* o);
    void setData(const DAData& d);
    // 宿主部件（不透明 QWidget：弹层父级/子窗口视觉宿主/页面坐标系锚点，
    // 协作者不得经此回访页面类型）
    void setHostWidget(QWidget* w);
    // ===== 数据 =====
    const DAData& data() const;
    DAPyDataFrame df() const;
    // 真实数据行数（不含尾部预留空行；模型优先，无模型回退 dataframe shape）
    int dataRowCount() const;
    // 真实数据列数（不含尾部预留空列；模型优先，无模型回退 dataframe shape）
    int dataColumnCount() const;
    // 数据列名表（数据列序；空数据返回空表）
    QStringList columnNames() const;
    // 视图行（滑动窗内逻辑行）→ 数据源绝对行（唯一换算点；无主窗格时原样返回）
    int toActualRow(int viewRow) const;
    // 数据源是否可编辑（tableSource 非空且只读——如数据库惰性表——为假）
    bool isDataSourceEditable() const;
    // 色阶/数据条的数据源可用性：惰性加载（无法全量列统计）或只读为假；
    // 无 tableSource 的普通 dataframe 可用（条件格式的统计端点判定）
    bool isColumnStatsSourceAvailable() const;
    // ===== 视图 =====
    DADataTableModel* model() const;
    DADataTableView* mainView() const;
    QWidget* hostWidget() const;
    // 本页所有需要投影的窗格（主窗格在前；冻结窗格懒建后追加）
    QList< DADataTableView* > panes() const;
    DATablePaneContainer* paneContainer() const;
    // ===== 元数据束与子存储（借自注册表，可能为 nullptr）=====
    DATableMetadata* metadata() const;
    DATableStyleManager* styleManager() const;
    DATableViewStateStore* viewStateStore() const;
    DATableHighlightOverlay* highlightOverlay() const;
    DATableConditionalFormatStore* conditionalFormatStore() const;
    DATableComputedColumnStore* computedColumnStore() const;
    DATableValidationStore* validationStore() const;
    DATableNoteStore* noteStore() const;
    // ===== 宿主桥（宿主页面注入的副作用回调；协作者经此触达页面级动作，
    //       不持窗口指针，见计划 §6.1 notifyDirty/notifyHost 同款设计）=====
    // 撤销栈推入（push 即激活页面撤销栈）；空 = 无撤销栈（只读场景）
    std::function< void(QUndoCommand*) > pushCommand;
    // DADataManager 树刷新（ChangeValue：删列后重建列子项等）；空 = 不通知
    std::function< void() > notifyHostValueChange;
    // DADataManager 树刷新（ChangeDataframeColumnName：改列名后重建列子项）；空 = 不通知
    std::function< void() > notifyHostColumnRename;
    // 关闭页级浮窗（快速绘图子窗口等，结构变更后列身份失效）；空 = 不关闭
    std::function< void() > closeFloatingWindows;
    // 定位单入口（007 语义：数据源绝对行 + 列位置 + 是否选中命中格；隐藏列自动
    // 取消隐藏 + 冻结区处置；select=false 只摆位不收缩选区——查找「选区范围」导航用）；
    // 报告窗等控制器经此触达页面的定位能力；空 = 不定位
    std::function< void(int, int, bool) > locateActualCell;
    // 数据脏标（页面 dataModified 信号；计算列重算等改数据的操作经此标脏）；空 = 不标
    std::function< void() > notifyDataModified;
    // 表头拾取模式查询（图表「选择序列」会话期间排序入口不生效）；空 = 非拾取模式
    std::function< bool() > isHeaderPickMode;
    // 就地替换生效后的整页刷新（模型 reset + 清排序规格；超大表降级路径用）；空 = 不刷新
    std::function< void() > refreshAfterInplaceReplace;

private:
    DAData mData;
    DADataTableModel* mModel { nullptr };
    DADataTableView* mMainView { nullptr };
    QWidget* mHostWidget { nullptr };
    DATablePaneContainer* mPaneContainer { nullptr };
    DATableMetadata* mMetadata { nullptr };
    DATableHighlightOverlay* mHighlightOverlay { nullptr };
};
}  // end of namespace DA
#endif  // DATABLEPAGECONTEXT_H
