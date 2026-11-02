#ifndef DATABLEPAGECONTEXT_H
#define DATABLEPAGECONTEXT_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include <QList>
#include <QString>
#include <QStringList>

class QAbstractItemModel;

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
    // ===== 视图 =====
    DADataTableModel* model() const;
    DADataTableView* mainView() const;
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

private:
    DAData mData;
    DADataTableModel* mModel { nullptr };
    DADataTableView* mMainView { nullptr };
    DATablePaneContainer* mPaneContainer { nullptr };
    DATableMetadata* mMetadata { nullptr };
    DATableHighlightOverlay* mHighlightOverlay { nullptr };
};
}  // end of namespace DA
#endif  // DATABLEPAGECONTEXT_H
