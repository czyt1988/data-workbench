#include "DataFrameDataSearchDialog.h"
#include "ui_DataFrameDataSearchDialog.h"
#include "DAWaitCursorScoped.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPyScripts.h"
#include "DAPyDataFrameTableView.h"
#include "DADataTableView.h"
#include "DAWaitCursorScoped.h"

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DataFrameDataSearchDialog::DataFrameDataSearchDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DataFrameDataSearchDialog)
{
    ui->setupUi(this);
    connect(ui->lineEditFindItem, &QLineEdit::textChanged, this, &DataFrameDataSearchDialog::onLineEditTextChanged);
    connect(ui->pushButtonNext, &QPushButton::clicked, this, &DataFrameDataSearchDialog::onPushButtonNextClicked);
}

/**
 * @brief 析构函数
 */
DataFrameDataSearchDialog::~DataFrameDataSearchDialog()
{
    delete ui;
}

/**
 * @brief 获取搜索文本
 * @return 搜索文本
 */
QString DataFrameDataSearchDialog::getSearchText() const
{
    return ui->lineEditFindItem->text();
}

/**
 * @brief 获取匹配项的坐标列表
 * @return 坐标列表
 */
QList< QPair< int, int > > DataFrameDataSearchDialog::getItemCoor() const
{
    return mMatches;
}

/**
 * @brief 点击下一个匹配项
 */
void DataFrameDataSearchDialog::onPushButtonNextClicked()
{
    // 直接在此函数上操作
    if (mIsNeedResearch) {
        searchData();
        mIsNeedResearch = false;
    }
    if (mMatches.empty()) {
        ui->labelLocation->setText(tr("Cannot find item"));  // cn:无法找到条目
        return;
    }
    if (mIndex >= mMatches.size()) {
        mIndex = 0;
    }
    QPair< int, int > cellloc = mMatches[ mIndex ];
    mDataTableView->selectActualCell(cellloc.first, cellloc.second);
    ui->labelLocation->setText(tr("Found at column %1, line %2")
                                   .arg(mDataTableView->actualColumnName(cellloc.second))
                                   .arg(mDataTableView->actualRowName(cellloc.first)));  // cn:在第%2行、第%1列找到
    ++mIndex;
}

/**
 * @brief 获取数据表格视图
 * @return 数据表格视图指针
 */
DA::DADataTableView* DataFrameDataSearchDialog::getDataTableView() const
{
    return mDataTableView;
}

/**
 * @brief 设置数据表格视图
 * @param v 数据表格视图指针
 */
void DataFrameDataSearchDialog::setDataTableView(DA::DADataTableView* v)
{
    if (mDataTableView == v) {
        return;
    }
    mDataTableView = v;
    // 搜索内容发生改变时，标记重新搜索
    mIsNeedResearch = true;
}

/**
 * @brief 执行搜索
 */
void DataFrameDataSearchDialog::searchData()
{
    DA_WAIT_CURSOR_SCOPED_NS();
    DA::DAPyDataFrame df           = mDataTableView->getData().toDataFrame();
    DA::DAPyScriptsDataFrame& pydf = DA::DAPyScripts::getDataFrame();
    mMatches                       = pydf.searchData(df, getSearchText());
    mIndex                         = 0;
}

/**
 * @brief 搜索文本变化时的槽函数
 * @param t 变化后的文本
 */
void DataFrameDataSearchDialog::onLineEditTextChanged(const QString& t)
{
    Q_UNUSED(t);
    // 搜索内容发生改变时，标记重新搜索
    mIsNeedResearch = true;
}
