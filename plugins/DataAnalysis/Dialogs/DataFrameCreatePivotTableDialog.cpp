#include "DataFrameCreatePivotTableDialog.h"
#include "ui_DataFrameCreatePivotTableDialog.h"
#include <QStandardItemModel>


class DataFrameCreatePivotTableDialog::PrivateData
{
    DA_DECLARE_PUBLIC(DataFrameCreatePivotTableDialog)
public:
    PrivateData(DataFrameCreatePivotTableDialog* p);

public:
    DA::DAPyDataFrame mDataframe;
    QStandardItemModel* mModel { nullptr };
};

/**
 * @brief 构造函数
 * @param p 对话框指针
 */
DataFrameCreatePivotTableDialog::PrivateData::PrivateData(DataFrameCreatePivotTableDialog* p) : q_ptr(p)
{
}

//===============================================================
// DADialogCreatePivotTable
//===============================================================

/**
 * @brief 构造函数
 *
 * @param parent 父窗口
 */
DataFrameCreatePivotTableDialog::DataFrameCreatePivotTableDialog(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DataFrameCreatePivotTableDialog)
{
    ui->setupUi(this);
    this->initPivotTableAggfunc();
    connect(ui->tableViewParameter, &QTableView::clicked, this, &DataFrameCreatePivotTableDialog::onTableItemClicked);
}

/**
 * @brief 析构函数
 */
DataFrameCreatePivotTableDialog::~DataFrameCreatePivotTableDialog()
{
    delete ui;
}

/**
 * @brief 获取数据帧
 * @return 数据帧对象
 */
DA::DAPyDataFrame DataFrameCreatePivotTableDialog::getDataFrame() const
{
    return d_ptr->mDataframe;
}

/**
 * @brief 设置数据帧
 * @param df 数据帧
 */
void DataFrameCreatePivotTableDialog::setDataframe(const DA::DAPyDataFrame& df)
{
    d_ptr->mDataframe = df;

    // 准备数据模型
    if (!d_ptr->mModel) {
        d_ptr->mModel = new QStandardItemModel(this);
    }
    QStandardItemModel* m = d_ptr->mModel;
    QStringList para      = df.columns();
    m->clear();

    // 设置tableview表头内容
    QStringList headers;
    headers << tr("Value") << tr("Index") << tr("Columns");  // cn:值、索引、列

    // 添加数据
    for (int row = 0; row < para.size(); ++row) {
        QStandardItem* vitem = new QStandardItem(para[ row ]);
        QStandardItem* iitem = new QStandardItem(para[ row ]);
        QStandardItem* citem = new QStandardItem(para[ row ]);

        vitem->setCheckable(true);
        vitem->setCheckState(Qt::Unchecked);

        iitem->setCheckable(true);
        iitem->setCheckState(Qt::Unchecked);

        citem->setCheckable(true);
        citem->setCheckState(Qt::Unchecked);

        m->setItem(row, 0, vitem);  // "Value" 列
        m->setItem(row, 1, iitem);  // "Index" 列
        m->setItem(row, 2, citem);  // "Columns" 列
    }

    // 添加tableview表头
    m->setHorizontalHeaderLabels(headers);

    // 利用 setModel() 方法将数据模型与 QTableView 绑定
    ui->tableViewParameter->setModel(m);
    ui->tableViewParameter->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 监听复选框状态变化，实现**一行互斥**
    connect(m, &QStandardItemModel::itemChanged, [ m ](QStandardItem* item) {
        int row = item->row();
        int col = item->column();

        if (item->checkState() == Qt::Checked) {
            // 如果当前列被选中，禁用本行的其他列
            for (int c = 0; c < 3; ++c) {
                if (c == col)
                    continue;
                if (QStandardItem* otherItem = m->item(row, c); otherItem) {
                    otherItem->setCheckState(Qt::Unchecked);
                    otherItem->setFlags(otherItem->flags() & ~Qt::ItemIsEnabled);
                }
            }
        } else {
            // 如果当前列取消勾选，则恢复本行所有列的可选状态
            for (int c = 0; c < 3; ++c) {
                if (QStandardItem* resetItem = m->item(row, c); resetItem) {
                    resetItem->setFlags(resetItem->flags() | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                }
            }
        }
    });
}

/**
 * @brief 获取数据透视表的值列
 * @return 值列名列表
 */
QStringList DataFrameCreatePivotTableDialog::getPivotTableValue() const
{
    QStandardItemModel* model = qobject_cast< QStandardItemModel* >(ui->tableViewParameter->model());

    // 存储选中的文本值
    QStringList values;
    if (!model) {
        return values;
    }

    int rows = model->rowCount();

    for (int i = 0; i < rows; ++i) {
        QStandardItem* item = model->item(i, 0);
        if (item && item->checkState() == Qt::Checked) {
            values << item->text();
        }
    }
    return values;
}

/**
 * @brief 获取数据透视表的索引列
 * @return 索引列名列表
 */
QStringList DataFrameCreatePivotTableDialog::getPivotTableIndex() const
{
    QStandardItemModel* model = qobject_cast< QStandardItemModel* >(ui->tableViewParameter->model());
    // 存储选中的文本值
    QStringList indexs;

    if (!model) {
        return indexs;
    }

    int rows = model->rowCount();

    for (int i = 0; i < rows; ++i) {
        QStandardItem* item = model->item(i, 1);
        if (item && item->checkState() == Qt::Checked) {
            indexs << item->text();
        }
    }
    return indexs;
}

/**
 * @brief 获取数据透视表的列
 * @return 列名列表
 */
QStringList DataFrameCreatePivotTableDialog::getPivotTableColumn() const
{
    QStandardItemModel* model = qobject_cast< QStandardItemModel* >(ui->tableViewParameter->model());

    if (!model)
        return QStringList();

    int rows = model->rowCount();
    // 存储选中的文本值
    QStringList columns;

    for (int i = 0; i < rows; ++i) {
        QStandardItem* item = model->item(i, 2);
        if (item && item->checkState() == Qt::Checked) {
            columns << item->text();
        }
    }
    return columns;
}

/**
 * @brief 初始化聚合函数下拉框
 */
void DataFrameCreatePivotTableDialog::initPivotTableAggfunc()
{
    ui->comboBoxAggfunc->addItem(tr("mean"), QStringLiteral("mean"));      // cn:均值
    ui->comboBoxAggfunc->addItem(tr("sum"), QStringLiteral("sum"));        // cn:求和
    ui->comboBoxAggfunc->addItem(tr("count"), QStringLiteral("count"));    // cn:计数
    ui->comboBoxAggfunc->addItem(tr("size"), QStringLiteral("size"));      // cn:大小
    ui->comboBoxAggfunc->addItem(tr("min"), QStringLiteral("min"));        // cn:最小值
    ui->comboBoxAggfunc->addItem(tr("max"), QStringLiteral("max"));        // cn:最大值
    ui->comboBoxAggfunc->addItem(tr("median"), QStringLiteral("median"));  // cn:中位数
    ui->comboBoxAggfunc->addItem(tr("std"), QStringLiteral("std"));        // cn:标准差
    ui->comboBoxAggfunc->addItem(tr("var"), QStringLiteral("var"));        // cn:方差
    ui->comboBoxAggfunc->addItem(tr("first"), QStringLiteral("first"));    // cn:第一个
    ui->comboBoxAggfunc->addItem(tr("last"), QStringLiteral("last"));      // cn:最后一个
    ui->comboBoxAggfunc->addItem(tr("prod"), QStringLiteral("prod"));      // cn:乘积
}

/**
 * @brief 获取聚合函数
 *
 * @todo 这里不要使用combox的文本作为聚合函数的名字，应该使用combox的data，因为文本有可能进行翻译，翻译后就会导致错误，
 * 应该如下操作:
 * @code
 * ui->comboBoxAggfunc->addItem(tr("menu"),QString("menu"));//第一个是可翻译的文本，第二个是设置到item的QVariant的data内容，
 * @endcode
 *
 * @return
 */
QString DataFrameCreatePivotTableDialog::getPivotTableAggfunc() const
{
    QVariant data = ui->comboBoxAggfunc->itemData(ui->comboBoxAggfunc->currentIndex());
    return data.toString();
}

/**
 * @brief 获取是否启用分组名称
 * @return true表示启用，false表示不启用
 */
bool DataFrameCreatePivotTableDialog::isEnableMarginsName() const
{
    return ui->checkBoxMargins->isChecked();
}

/**
 * @brief 设置是否启用分组
 * @param on 是否启用
 */
void DataFrameCreatePivotTableDialog::setEnableMargins(bool on)
{
    ui->checkBoxMargins->setChecked(on);
}

/**
 * @brief
 *
 * @todo MarginsName不应该让用户输入，而是让用户选择，可以使用带checkbox的combox,这个去github上或者哪里搜索一下有没有现成的，这个combox把列名列举出来，再加上一个all，让用户自己勾选就行
 * @return
 */
QString DataFrameCreatePivotTableDialog::getMarginsName() const
{
    return ui->lineEditMarginsName->text();
}

/**
 * @brief 设置分组名称
 * @param s 分组名称
 */
void DataFrameCreatePivotTableDialog::setMarginsName(QString& s)
{
    ui->lineEditMarginsName->setText(s);
}

/**
 * @brief 获取是否启用排序
 * @return true表示启用，false表示不启用
 */
bool DataFrameCreatePivotTableDialog::isEnableSort() const
{
    return ui->checkBoxSort->isChecked();
}

/**
 * @brief 设置是否启用排序
 * @param on 是否启用
 */
void DataFrameCreatePivotTableDialog::setEnableSort(bool on)
{
    ui->checkBoxSort->setChecked(on);
}

/**
 * @brief 表格项点击事件
 * @param index 点击的索引
 */
void DataFrameCreatePivotTableDialog::onTableItemClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    QStandardItem* item = d_ptr->mModel->itemFromIndex(index);
    if (item && item->isCheckable()) {
        // 切换复选框状态
        Qt::CheckState newState = (item->checkState() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
        item->setCheckState(newState);
    }
}
