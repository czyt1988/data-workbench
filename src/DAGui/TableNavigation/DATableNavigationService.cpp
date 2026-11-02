#include "DATableNavigationService.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
// qt
#include <QDialog>
#include <QModelIndex>
#include <QStringList>
#include <QWidget>
// DAGui
#include "Models/DADataTableModel.h"
#include "Dialog/DADialogGotoCell.h"
#include "DATableViewStateStore.h"
// DAData
#include "DADataPyDataFrame.h"
#include "pandas/DAPyIndex.h"
// python 索引标签查询（findLabelLocations）
#include "DAPybind11QtCaster.hpp"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

namespace DA
{
//===================================================
// DATableNavigationService
//===================================================

DATableNavigationService::DATableNavigationService(const DATablePageContext& ctx,
                                                   const DATableSelectionService& sel,
                                                   const std::function< void(int, int, bool, bool) >& locate,
                                                   QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mLocate(locate)
{
}

/**
 * @brief 打开定位对话框（007 T1.7）
 *
 * 懒建复用；setup 重置初值（不回显残留）；exec 模态，确定后解析请求。
 * 无数据页静默返回。
 */
void DATableNavigationService::showGotoDialog()
{
    DADataTableModel* model = mCtx.model();
    if (!model || mCtx.dataRowCount() <= 0 || mCtx.dataColumnCount() <= 0) {
        return;
    }
    if (!mGotoDialog) {
        mGotoDialog = new DADialogGotoCell(mCtx.hostWidget());
    }
    QStringList columnNames;  // 重名列保留重复项（不做去重）
    for (int c = 0; c < mCtx.dataColumnCount(); ++c) {
        columnNames.append(model->headerData(c, Qt::Horizontal).toString());
    }
    const QModelIndex cur        = mSel.currentOrSelectionTopLeftIndex();
    const int currentRowPosition = cur.isValid() ? mCtx.toActualRow(cur.row()) + 1 : 1;
    const QString currentColName = cur.isValid() ? model->headerData(cur.column(), Qt::Horizontal).toString() : QString();
    const bool hasHiddenColumns  = (mCtx.viewStateStore() && !mCtx.viewStateStore()->hiddenColumns().isEmpty());
    mGotoDialog->setup(mCtx.dataRowCount(), columnNames, currentRowPosition, currentColName, hasHiddenColumns);
    if (mGotoDialog->exec() != QDialog::Accepted) {
        return;
    }
    resolveGotoRequest(mGotoDialog->request());
}

/**
 * @brief 解析定位请求并调定位单入口（007 T1.7）
 *
 * 行解析 → 列解析 → 只调定位宿主桥（不自己写滚动/定位）。返回 false =
 * 未解析成功（已给用户可见提示、未改选区）。行列口径只用 dataRowCount()/
 * dataColumnCount()；currentIndex() 的行是滑动窗逻辑行，经 toActualRow 换算。
 * @param req 定位请求
 * @return 是否定位成功
 */
bool DATableNavigationService::resolveGotoRequest(const DADialogGotoCell::Request& req)
{
    DADataTableModel* model = mCtx.model();
    if (!model || mCtx.dataRowCount() <= 0 || mCtx.dataColumnCount() <= 0) {
        return false;
    }
    // ① 行解析
    int actualRow = 0;
    switch (req.rowKind) {
    case DADialogGotoCell::Request::RowKind::FirstRow:
        actualRow = 0;
        break;
    case DADialogGotoCell::Request::RowKind::LastRow:
        actualRow = mCtx.dataRowCount() - 1;
        break;
    case DADialogGotoCell::Request::RowKind::Position:
        if (req.rowPosition < 1 || req.rowPosition > mCtx.dataRowCount()) {
            daWarning << tr("Row number is out of range");  // cn:行号超出范围
            return false;
        }
        actualRow = req.rowPosition - 1;
        break;
    case DADialogGotoCell::Request::RowKind::None: {
        // 保持当前行（当前项无效 → 第 0 行）
        const QModelIndex cur = mSel.currentOrSelectionTopLeftIndex();
        actualRow = cur.isValid() ? mCtx.toActualRow(cur.row()) : 0;
        break;
    }
    case DADialogGotoCell::Request::RowKind::IndexLabel: {
        DAPyIndex index                    = mCtx.df().index();
        DAPyIndex::IndexLabelLocations loc = index.findLabelLocations(DA::PY::toPyObject(req.indexLabel));
        if (!loc.found) {  // 数值索引：字符串→整数/浮点再试一次（同一个查询路径）
            bool ok = false;
            const long long asInt = req.indexLabel.toLongLong(&ok);
            if (ok) {
                loc = index.findLabelLocations(pybind11::int_(asInt));
            } else {
                const double asDouble = req.indexLabel.toDouble(&ok);
                if (ok) {
                    loc = index.findLabelLocations(pybind11::float_(asDouble));
                }
            }
        }
        if (!loc.found) {
            daWarning << tr("Index label not found: %1").arg(req.indexLabel);  // cn:未找到索引标签：%1
            return false;
        }
        if (loc.count > 1) {
            daInfo << tr("There are %1 rows with this index label; jumped to the first one").arg(loc.count);  // cn:有 %1 行使用该索引标签，已跳到第一个
        }
        actualRow = static_cast< int >(loc.firstPos);
        break;
    }
    }
    // ② 列解析
    int col = 0;
    switch (req.columnKind) {
    case DADialogGotoCell::Request::ColumnKind::FirstColumn:
        col = 0;
        break;
    case DADialogGotoCell::Request::ColumnKind::LastColumn:
        col = mCtx.dataColumnCount() - 1;
        break;
    case DADialogGotoCell::Request::ColumnKind::Name: {
        int found    = -1;  // 重名取第一个（数据列序）
        int nameHits = 0;
        for (int c = 0; c < mCtx.dataColumnCount(); ++c) {
            if (model->headerData(c, Qt::Horizontal).toString() == req.columnName) {
                if (found < 0) {
                    found = c;
                }
                ++nameHits;
            }
        }
        if (found < 0) {
            daWarning << tr("Column not found: %1").arg(req.columnName);  // cn:未找到列名：%1
            return false;
        }
        if (nameHits > 1) {
            daInfo << tr("There are %1 columns with this name; used the first one").arg(nameHits);  // cn:有 %1 个同名列，已取第一个
        }
        col = found;
        break;
    }
    case DADialogGotoCell::Request::ColumnKind::None: {
        // 保持当前列（当前项无效 → 第 0 列）
        const QModelIndex cur = mSel.currentOrSelectionTopLeftIndex();
        col = cur.isValid() ? cur.column() : 0;
        break;
    }
    }
    // ③ 收尾：定位单入口（keepHidden 透传）
    if (mLocate) {
        mLocate(actualRow, col, /*select=*/true, /*keepHidden=*/req.keepHidden);
    }
    return true;
}
}  // end of namespace DA
