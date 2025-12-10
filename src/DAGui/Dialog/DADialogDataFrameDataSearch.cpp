#include "DADialogDataFrameDataSearch.h"
#include "ui_DADialogDataFrameDataSearch.h"
#include "DAWaitCursorScoped.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPyScripts.h"
#include "DAPyDataFrameTableView.h"
#include "DADataTableView.h"
namespace DA
{
DADialogDataFrameDataSearch::DADialogDataFrameDataSearch(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataFrameDataSearch)
{
    ui->setupUi(this);
    connect(ui->lineEditFindItem, &QLineEdit::textChanged, this, &DADialogDataFrameDataSearch::onLineEditTextChanged);
    connect(ui->pushButtonNext, &QPushButton::clicked, this, &DADialogDataFrameDataSearch::onPushButtonNextClicked);
}

DADialogDataFrameDataSearch::~DADialogDataFrameDataSearch()
{
    delete ui;
}

QString DADialogDataFrameDataSearch::getSearchText() const
{
    return ui->lineEditFindItem->text();
}

QList< QPair< int, int > > DADialogDataFrameDataSearch::getItemCoor() const
{
    return mMatches;
}

void DADialogDataFrameDataSearch::onPushButtonNextClicked()
{
    // 直接在此函数上操作
    if (mIsNeedResearch) {
        searchData();
        mIsNeedResearch = false;
    }
    if (mMatches.empty()) {
        ui->labelLocation->setText(tr("can not find item"));  // cn:无法找到条目
        return;
    }
    if (mIndex >= mMatches.size()) {
        mIndex = 0;
    }
    QPair< int, int > cellloc = mMatches[ mIndex ];
    mDataTableView->selectActualCell(cellloc.first, cellloc.second);
    ui->labelLocation->setText(tr("Found at column %1,line %2")
                                   .arg(mDataTableView->actualColumnName(cellloc.second))
                                   .arg(mDataTableView->actualRowName(cellloc.first)));
    ++mIndex;
}

DADataTableView* DADialogDataFrameDataSearch::getDataTableView() const
{
    return mDataTableView;
}

void DADialogDataFrameDataSearch::setDataTableView(DADataTableView* v)
{
    if (mDataTableView == v) {
        return;
    }
    mDataTableView = v;
    // 搜索内容发生改变时，标记重新搜索
    mIsNeedResearch = true;
}

void DADialogDataFrameDataSearch::searchData()
{
    DA_WAIT_CURSOR_SCOPED();
    DAPyDataFrame df           = mDataTableView->getData().toDataFrame();
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    mMatches                   = pydf.searchData(df, getSearchText());
    mIndex                     = 0;
}

void DADialogDataFrameDataSearch::onLineEditTextChanged(const QString& t)
{
    Q_UNUSED(t);
    // 搜索内容发生改变时，标记重新搜索
    mIsNeedResearch = true;
}

}  // end DA
