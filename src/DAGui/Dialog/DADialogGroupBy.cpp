#include "DADialogGroupBy.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAWaitCursorScoped.h"
#include <QListWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDialogButtonBox>

namespace DA
{

/**
 * @brief 构造
 * @param df 数据（只读；聚合产出新表，不改原表）
 * @param parent 父窗口
 */
DADialogGroupBy::DADialogGroupBy(const DAPyDataFrame& df, QWidget* parent) : QDialog(parent), mDf(df)
{
    setupUI();
    setWindowTitle(tr("Group By"));  // cn:分组聚合
    runAggregate();
}

DADialogGroupBy::~DADialogGroupBy()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）：左=键/值/函数选择，右=状态+预览
 */
void DADialogGroupBy::setupUI()
{
    setModal(true);
    const QList< QString > cols = mDf.columns();
    QWidget* leftPanel          = new QWidget(this);
    QVBoxLayout* leftLay        = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    QGroupBox* keyGroup         = new QGroupBox(tr("Group keys (multi-select)"), leftPanel);  // cn:分组键（多选）
    QVBoxLayout* keyLay         = new QVBoxLayout(keyGroup);
    keyLay->setContentsMargins(4, 4, 4, 4);
    mListKeys                   = new QListWidget(keyGroup);
    mListKeys->setObjectName(QStringLiteral("listGroupKeys"));
    mListKeys->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const QString& c : cols) {
        mListKeys->addItem(c);
    }
    keyLay->addWidget(mListKeys);
    leftLay->addWidget(keyGroup, 3);
    QGroupBox* valueGroup   = new QGroupBox(tr("Aggregate columns (multi-select)"), leftPanel);  // cn:聚合列（多选）
    QVBoxLayout* valueLay   = new QVBoxLayout(valueGroup);
    valueLay->setContentsMargins(4, 4, 4, 4);
    mListValues             = new QListWidget(valueGroup);
    mListValues->setObjectName(QStringLiteral("listGroupValues"));
    mListValues->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const QString& c : cols) {
        mListValues->addItem(c);
    }
    valueLay->addWidget(mListValues);
    leftLay->addWidget(valueGroup, 3);
    QGroupBox* funcGroup  = new QGroupBox(tr("Aggregations (column / function)"), leftPanel);  // cn:聚合设置（列 / 函数）
    QVBoxLayout* funcLay  = new QVBoxLayout(funcGroup);
    funcLay->setContentsMargins(4, 4, 4, 4);
    mTableFuncs           = new QTableWidget(funcGroup);
    mTableFuncs->setObjectName(QStringLiteral("tableGroupFuncs"));
    mTableFuncs->setColumnCount(2);
    mTableFuncs->setHorizontalHeaderLabels(QStringList() << tr("Column") << tr("Function"));  // cn:列 / 函数
    mTableFuncs->verticalHeader()->setVisible(false);
    mTableFuncs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTableFuncs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    funcLay->addWidget(mTableFuncs);
    leftLay->addWidget(funcGroup, 2);

    QWidget* rightPanel   = new QWidget(this);
    QVBoxLayout* rightLay = new QVBoxLayout(rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    mLabelStatus = new QLabel(rightPanel);
    mLabelStatus->setWordWrap(true);
    rightLay->addWidget(mLabelStatus);
    QGroupBox* previewGroup = new QGroupBox(tr("Preview (first %1 rows)").arg(c_previewRows), rightPanel);  // cn:预览（前 %1 行）
    QVBoxLayout* previewLay = new QVBoxLayout(previewGroup);
    previewLay->setContentsMargins(4, 4, 4, 4);
    mTablePreview = new QTableWidget(previewGroup);
    mTablePreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTablePreview->verticalHeader()->setVisible(false);
    previewLay->addWidget(mTablePreview);
    rightLay->addWidget(previewGroup, 1);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtonOk                 = buttons->button(QDialogButtonBox::Ok);
    mButtonOk->setText(tr("OK"));                                      // cn:确定
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    mButtonOk->setEnabled(false);
    QHBoxLayout* bodyLay = new QHBoxLayout();
    bodyLay->addWidget(leftPanel, 2);
    bodyLay->addWidget(rightPanel, 3);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->addLayout(bodyLay, 1);
    mainLay->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(300);
    connect(mDebounceTimer, &QTimer::timeout, this, &DADialogGroupBy::runAggregate);
    connect(mListKeys, &QListWidget::itemSelectionChanged, this, &DADialogGroupBy::onInputChanged);
    connect(mListValues, &QListWidget::itemSelectionChanged, this, &DADialogGroupBy::onValueSelectionChanged);

    resize(880, 520);
}

void DADialogGroupBy::onValueSelectionChanged()
{
    rebuildFuncTable();
    onInputChanged();
}

void DADialogGroupBy::onInputChanged()
{
    mDebounceTimer->start();
}

/**
 * @brief 按当前选中的聚合列重建「列名+函数」表（保留既有函数选择，默认 mean）
 */
void DADialogGroupBy::rebuildFuncTable()
{
    // 先记住现有下拉的当前选择
    for (int r = 0; r < mTableFuncs->rowCount(); ++r) {
        QTableWidgetItem* nameItem = mTableFuncs->item(r, 0);
        QComboBox* combo           = qobject_cast< QComboBox* >(mTableFuncs->cellWidget(r, 1));
        if (nameItem && combo) {
            mFuncChoice[ nameItem->text() ] = combo->currentText();
        }
    }
    const QModelIndexList sel = mListValues->selectionModel()->selectedIndexes();
    QStringList values;
    for (const QModelIndex& i : sel) {
        values.append(i.data(Qt::DisplayRole).toString());
    }
    mTableFuncs->clearContents();
    mTableFuncs->setRowCount(values.size());
    for (int r = 0; r < values.size(); ++r) {
        mTableFuncs->setItem(r, 0, new QTableWidgetItem(values.at(r)));
        QComboBox* combo = new QComboBox(mTableFuncs);
        // 聚合函数集合与透视表对齐（da_create_pivot_table：mean/sum/size）
        combo->addItem(QStringLiteral("mean"));
        combo->addItem(QStringLiteral("sum"));
        combo->addItem(QStringLiteral("size"));
        const QString prev = mFuncChoice.value(values.at(r), QStringLiteral("mean"));
        combo->setCurrentText(prev);
        connect(combo, &QComboBox::currentTextChanged, this, [ this ](const QString& ) {
            onInputChanged();
        });
        mTableFuncs->setCellWidget(r, 1, combo);
    }
}

void DADialogGroupBy::setStatusError(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#CE6043;"));  // 橙红（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(false);
}

void DADialogGroupBy::setStatusOk(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#669E8B;"));  // 绿（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(true);
}

/**
 * @brief 全表聚合 + 预览（与产出同一算子 da_groupby_aggregate，所见即所得）
 *
 * 大表聚合可能耗时（验收 §5.4：100 万行可接受 + 忙等待提示），全程忙等待光标；
 * 结果缓存在 mResult，接受后页面直接取用，不重复聚合。
 */
void DADialogGroupBy::runAggregate()
{
    // 失败/未算态用显式 none（DAPyDataFrame() 默认构造是空 DataFrame，isNone()==false）
    mResult     = DAPyDataFrame(pybind11::none());
    mResultRows = 0;
    mTablePreview->clear();
    mTablePreview->setRowCount(0);
    mTablePreview->setColumnCount(0);
    if (mDf.isNone()) {
        setStatusError(tr("No data"));  // cn:无数据
        return;
    }
    // 分组键（多选，按列表顺序）
    QStringList keys;
    for (int r = 0; r < mListKeys->count(); ++r) {
        if (mListKeys->item(r)->isSelected()) {
            keys.append(mListKeys->item(r)->text());
        }
    }
    if (keys.isEmpty()) {
        setStatusError(tr("Please select at least one group key"));  // cn:请至少选择一个分组键
        return;
    }
    // 聚合列 + 每列函数（函数表行序 = 选择序）
    QStringList values, funcs;
    for (int r = 0; r < mTableFuncs->rowCount(); ++r) {
        QTableWidgetItem* nameItem = mTableFuncs->item(r, 0);
        QComboBox* combo           = qobject_cast< QComboBox* >(mTableFuncs->cellWidget(r, 1));
        if (nameItem && combo) {
            values.append(nameItem->text());
            funcs.append(combo->currentText());
        }
    }
    if (values.isEmpty()) {
        setStatusError(tr("Please select at least one aggregation column"));  // cn:请至少选择一个聚合列
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QString err;
    {
        DAWaitCursorScoped waitCursor;  // 全表聚合忙等待（验收 §5.4）
        mResult = py.groupbyAggregate(mDf, keys, values, funcs, &err);
    }
    if (mResult.isNone()) {
        setStatusError(tr("Aggregation failed: %1").arg(err));  // cn:聚合失败：%1
        return;
    }
    const auto shape = mResult.shape();
    mResultRows      = static_cast< int >(shape.first);
    const int ncols  = static_cast< int >(shape.second);
    // 预览前 N 行
    const QList< QString > resCols = mResult.columns();
    const int nrows                = qMin(mResultRows, c_previewRows);
    mTablePreview->setColumnCount(ncols);
    mTablePreview->setRowCount(nrows);
    mTablePreview->setHorizontalHeaderLabels(resCols);
    for (int c = 0; c < ncols; ++c) {
        for (int r = 0; r < nrows; ++r) {
            mTablePreview->setItem(r, c, new QTableWidgetItem(mResult.iat(r, c).toString()));
        }
    }
    mTablePreview->resizeColumnsToContents();
    setStatusOk(tr("OK — %1 group(s) (result rows)").arg(mResultRows));  // cn:通过 — 共 %1 组（结果行数）
}

DAPyDataFrame DADialogGroupBy::result() const
{
    return mResult;
}

QStringList DADialogGroupBy::groupKeys() const
{
    QStringList keys;
    for (int r = 0; r < mListKeys->count(); ++r) {
        if (mListKeys->item(r)->isSelected()) {
            keys.append(mListKeys->item(r)->text());
        }
    }
    return keys;
}

int DADialogGroupBy::resultRowCount() const
{
    return mResultRows;
}

}  // end of namespace DA
