#include "DATablePageContext.h"
// qt
#include <QAbstractItemModel>
// DAData
#include "DATableDataSource.h"
// DAGui
#include "Models/DADataTableModel.h"
#include "DADataTableView.h"
#include "DATablePaneContainer.h"
#include "DATableHighlightOverlay.h"
#include "DATableMetadata.h"
#include "DATableStyleManager.h"
#include "DATableViewStateStore.h"
#include "DATableConditionalFormatStore.h"
#include "DATableComputedColumnStore.h"
#include "DATableValidationStore.h"
#include "DATableNoteStore.h"

namespace DA
{
//===================================================
// DATablePageContext
//===================================================

DATablePageContext::DATablePageContext()
{
}

void DATablePageContext::setModel(DADataTableModel* m)
{
    mModel = m;
}

void DATablePageContext::setMainView(DADataTableView* v)
{
    mMainView = v;
}

void DATablePageContext::setPaneContainer(DATablePaneContainer* c)
{
    mPaneContainer = c;
}

void DATablePageContext::setMetadata(DATableMetadata* md)
{
    mMetadata = md;
}

void DATablePageContext::setHighlightOverlay(DATableHighlightOverlay* o)
{
    mHighlightOverlay = o;
}

void DATablePageContext::setHostWidget(QWidget* w)
{
    mHostWidget = w;
}

void DATablePageContext::setData(const DAData& d)
{
    mData = d;
}

/**
 * @brief 数据引用
 * @return 宿主页面当前持有的数据（常引用，值语义共享底层对象）
 */
const DAData& DATablePageContext::data() const
{
    return mData;
}

/**
 * @brief 取 dataframe 视图
 *
 * 非 dataframe 数据（series/惰性表）返回空对象（isNone() 为 true）。
 * @return dataframe；非 dataframe 数据返回空对象
 */
DAPyDataFrame DATablePageContext::df() const
{
    return mData.toDataFrame();
}

/**
 * @brief 真实数据行数（不含尾部预留空行）
 *
 * 模型优先（actualRowCount 本就不含尾部预留行；此前误减 getExtraRowCount 会把
 * N 行表算成 N-1 行，007 修复的口径在此延续）；无模型回退 dataframe shape。
 * @return 行数
 */
int DATablePageContext::dataRowCount() const
{
    if (mModel) {
        return qMax(0, mModel->actualRowCount());
    }
    DAPyDataFrame d = df();
    if (d.isNone()) {
        return 0;
    }
    return static_cast< int >(d.shape().first);
}

/**
 * @brief 真实数据列数（不含尾部预留空列）
 * @return 列数
 */
int DATablePageContext::dataColumnCount() const
{
    if (mModel) {
        return qMax(0, mModel->columnCount() - mModel->getExtraColumnCount());
    }
    DAPyDataFrame d = df();
    if (d.isNone()) {
        return 0;
    }
    return static_cast< int >(d.shape().second);
}

/**
 * @brief 数据列名表
 *
 * 尾部预留空白列（列名为空）跳过。
 * @return 列名表（数据列序）
 */
QStringList DATablePageContext::columnNames() const
{
    QStringList res;
    DAPyDataFrame d = df();
    if (d.isNone()) {
        return res;
    }
    const QList< QString > cols = d.columns();
    for (const QString& c : cols) {
        if (!c.isEmpty()) {
            res.append(c);
        }
    }
    return res;
}

/**
 * @brief 视图行号转数据源绝对行号
 *
 * 表格视图带滑动窗，选中项的行号是窗口内行号；凡是拿行号去读写 dataframe、
 * 样式表、undo 命令的地方都必须先经过这里，否则滚动之后就会操作到别的行。
 * @param viewRow 视图行号（滑动窗内逻辑行）
 * @return 数据源绝对行号（无主窗格时原样返回）
 */
int DATablePageContext::toActualRow(int viewRow) const
{
    if (!mMainView) {
        return viewRow;
    }
    return mMainView->toActualRow(viewRow);
}

/**
 * @brief 数据源是否可编辑
 *
 * 判定路径同 DADataTableModel::actualFlags 的只读源分支：tableSource 非空且
 * isTableEditable() 为假（如数据库惰性表）→ 只读。
 * @return 可编辑返回 true
 */
bool DATablePageContext::isDataSourceEditable() const
{
    if (mData.isNull()) {
        return false;
    }
    DAData d                    = mData;
    const DATableDataSource* ts = d.tableSource();
    return !(ts && !ts->isTableEditable());
}

DADataTableModel* DATablePageContext::model() const
{
    return mModel;
}

DADataTableView* DATablePageContext::mainView() const
{
    return mMainView;
}

QWidget* DATablePageContext::hostWidget() const
{
    return mHostWidget;
}

/**
 * @brief 本页所有需要投影的窗格
 *
 * 主窗格在前；冻结窗格懒建后追加（由窗格容器提供）。
 * @return 窗格列表
 */
QList< DADataTableView* > DATablePageContext::panes() const
{
    QList< DADataTableView* > res;
    if (mMainView) {
        res.append(mMainView);
    }
    if (mPaneContainer) {
        if (DADataTableView* frozen = mPaneContainer->frozenView()) {
            res.append(frozen);
        }
    }
    return res;
}

DATablePaneContainer* DATablePageContext::paneContainer() const
{
    return mPaneContainer;
}

DATableMetadata* DATablePageContext::metadata() const
{
    return mMetadata;
}

DATableStyleManager* DATablePageContext::styleManager() const
{
    return mMetadata ? mMetadata->styleManager() : nullptr;
}

DATableViewStateStore* DATablePageContext::viewStateStore() const
{
    return mMetadata ? mMetadata->viewState() : nullptr;
}

DATableHighlightOverlay* DATablePageContext::highlightOverlay() const
{
    return mHighlightOverlay;
}

DATableConditionalFormatStore* DATablePageContext::conditionalFormatStore() const
{
    return mMetadata ? mMetadata->conditionalFormat() : nullptr;
}

DATableComputedColumnStore* DATablePageContext::computedColumnStore() const
{
    return mMetadata ? mMetadata->computedColumns() : nullptr;
}

DATableValidationStore* DATablePageContext::validationStore() const
{
    return mMetadata ? mMetadata->validations() : nullptr;
}

DATableNoteStore* DATablePageContext::noteStore() const
{
    return mMetadata ? mMetadata->notes() : nullptr;
}
}  // end of namespace DA
