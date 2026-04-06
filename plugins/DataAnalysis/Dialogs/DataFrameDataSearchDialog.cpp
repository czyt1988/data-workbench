#include "DataFrameDataSearchDialog.h"
#include "ui_DataFrameDataSearchDialog.h"
#include "DAWaitCursorScoped.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPyScripts.h"
#include "DAPyDataFrameTableView.h"
#include "DADataTableView.h"
#include "DAWaitCursorScoped.h"

DataFrameDataSearchDialog::DataFrameDataSearchDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataFrameDataSearchDialog)
{
    ui->setupUi(this);
    connect(ui->lineEditFindItem, &QLineEdit::textChanged, this, &DataFrameDataSearchDialog::onLineEditTextChanged);
    connect(ui->pushButtonNext, &QPushButton::clicked, this, &DataFrameDataSearchDialog::onPushButtonNextClicked);
}

DataFrameDataSearchDialog::~DataFrameDataSearchDialog()
{
    delete ui;
}

QString DataFrameDataSearchDialog::getSearchText() const
{
    return ui->lineEditFindItem->text();
}

QList< QPair< int, int > > DataFrameDataSearchDialog::getItemCoor() const
{
    return mMatches;
}

void DataFrameDataSearchDialog::onPushButtonNextClicked()
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

DA::DADataTableView* DataFrameDataSearchDialog::getDataTableView() const
{
    return mDataTableView;
}

void DataFrameDataSearchDialog::setDataTableView(DA::DADataTableView* v)
{
    if (mDataTableView == v) {
        return;
    }
    mDataTableView = v;
    // 搜索内容发生改变时，标记重新搜索
    mIsNeedResearch = true;
}

void DataFrameDataSearchDialog::searchData()
{
    DA_WAIT_CURSOR_SCOPED_NS();
    DA::DAPyDataFrame df           = mDataTableView->getData().toDataFrame();
    DA::DAPyScriptsDataFrame& pydf = DA::DAPyScripts::getDataFrame();
    mMatches                       = pydf.searchData(df, getSearchText());
    mIndex                         = 0;
}

void DataFrameDataSearchDialog::onLineEditTextChanged(const QString& t)
{
    Q_UNUSED(t);
    // 搜索内容发生改变时，标记重新搜索
    mIsNeedResearch = true;
}
