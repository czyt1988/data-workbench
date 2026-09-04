#include "DADataManagerTreeWidget.h"
#include "ui_DADataManagerTreeWidget.h"
// Qt
#include <QTimer>
#include <QCompleter>
#include <QStringListModel>
#include <QItemSelectionModel>
#include <QHeaderView>
// stl
#include <algorithm>
// DA
#include "Models/DADataManagerTreeModel.h"
#include "DADataManager.h"
#include "Models/DADataManagerTreeFilterProxyModel.h"
#include "pandas/DAPyDataFrame.h"
namespace DA
{

class DADataManagerTreeWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManagerTreeWidget)
public:
    PrivateData(DADataManagerTreeWidget* p);
    void init();
    QStandardItem* getCurrentSelectItem() const;

public:
    DADataManagerTreeModel* model { nullptr };
    DADataManagerTreeFilterProxyModel* proxyModel { nullptr };
    QCompleter* completer { nullptr };
    QTimer* filterTimer { nullptr };
    QTimer* completerDebounceTimer { nullptr };
};

DADataManagerTreeWidget::PrivateData::PrivateData(DADataManagerTreeWidget* p) : q_ptr(p)
{
}

void DADataManagerTreeWidget::PrivateData::init()
{
    Ui::DADataManagerTreeWidget* ui = q_ptr->ui;
    // 创建模型和代理模型
    model = new DADataManagerTreeModel(q_ptr);
    model->setExpandDataframeToSeries(true);
    // 数据集重命名：启用模型编辑位，编辑经 F2 / ribbon 重命名按钮 / 右键菜单触发
    // （双击保留给"打开数据窗口"，避免双击同时触发编辑）
    model->setEnableEdit(true);
    // 属性列：名称+属性两列，dataframe 显示尺寸（行x列），series 留空
    model->setColumnStyle(DADataManagerTreeModel::ColumnWithNameProperty);
    proxyModel = new DADataManagerTreeFilterProxyModel(q_ptr);
    proxyModel->setSourceModel(model);
    ui->treeView->setModel(proxyModel);
    ui->treeView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    //  设置列宽：显示水平表头，拖动两列间分隔线可调节列宽；
    //  名称列 Interactive 可交互拖拽，属性列（最后一列）填满剩余空间
    QHeaderView* header = ui->treeView->header();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->treeView->setColumnWidth(0, 150);
    ui->treeView->setDragEnabled(true);                          // 启用拖曳
    ui->treeView->setAcceptDrops(false);                         // 树本身不接收拖放
    ui->treeView->setDragDropMode(QAbstractItemView::DragOnly);  // 只允许拖曳，不允许放置
    ui->treeView->setDefaultDropAction(Qt::CopyAction);

    // 初始化补全器
    // 创建补全器
    completer = new QCompleter(q_ptr);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);  // 包含匹配

    // 设置补全模型
    QStringListModel* completerModel = new QStringListModel(q_ptr);
    completer->setModel(completerModel);

    // 设置到ComboBox
    ui->comboBoxFilter->setCompleter(completer);

    // 初始化过滤定时器（用于延迟过滤，提高性能）
    filterTimer = new QTimer(q_ptr);
    filterTimer->setSingleShot(true);
    filterTimer->setInterval(300);  // 300ms延迟

    // 补全器去抖定时器，避免 rowsInserted 频繁触发全量刷新
    completerDebounceTimer = new QTimer(q_ptr);
    completerDebounceTimer->setSingleShot(true);
    completerDebounceTimer->setInterval(50);  // 50ms去抖
}

QStandardItem* DADataManagerTreeWidget::PrivateData::getCurrentSelectItem() const
{
    Ui::DADataManagerTreeWidget* ui = q_ptr->ui;
    auto index                      = ui->treeView->currentIndex();
    if (!index.isValid()) {
        return nullptr;
    }
    // 属性列（第二列）选中时映射回名称列
    if (index.column() != 0) {
        index = index.siblingAtColumn(0);
    }
    index = proxyModel->mapToSource(index);
    return model->itemFromIndex(index);
}
//===============================================================
// name
//===============================================================

DADataManagerTreeWidget::DADataManagerTreeWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADataManagerTreeWidget)
{
    ui->setupUi(this);
    d_ptr->init();
    connect(ui->toolButtonExpand, &QToolButton::clicked, this, &DADataManagerTreeWidget::onToolButtonExpandClicked);
    connect(ui->toolButtonCollapse, &QToolButton::clicked, this, &DADataManagerTreeWidget::onToolButtonCollapseClicked);
    connect(ui->comboBoxFilter, &QComboBox::editTextChanged, this, &DADataManagerTreeWidget::onComboBoxEditTextChanged);
    // 连接模型变化信号，通过去抖定时器更新补全器，避免批量操作时频繁全量刷新
    connect(d_ptr->model, &DADataManagerTreeModel::rowsInserted, d_ptr->completerDebounceTimer, QOverload<>::of(&QTimer::start));
    connect(d_ptr->model, &DADataManagerTreeModel::rowsRemoved, d_ptr->completerDebounceTimer, QOverload<>::of(&QTimer::start));
    connect(d_ptr->model, &DADataManagerTreeModel::dataChanged, d_ptr->completerDebounceTimer, QOverload<>::of(&QTimer::start));
    connect(d_ptr->completerDebounceTimer, &QTimer::timeout, this, &DADataManagerTreeWidget::updateCompleterModel);
    connect(d_ptr->filterTimer, &QTimer::timeout, this, &DADataManagerTreeWidget::applyFilter);

    connect(ui->treeView, &QTreeView::doubleClicked, this, &DADataManagerTreeWidget::onTreeViewDoubleClicked);
    connect(ui->treeView, &QTreeView::clicked, this, &DADataManagerTreeWidget::onTreeViewClicked);
}

DADataManagerTreeWidget::~DADataManagerTreeWidget()
{
    delete ui;
}

QTreeView* DADataManagerTreeWidget::getTreeView() const
{
    return ui->treeView;
}

DADataManagerTreeModel* DADataManagerTreeWidget::getModel() const
{
    return d_ptr->model;
}

DADataManagerTreeFilterProxyModel* DADataManagerTreeWidget::getProxyModel() const
{
    return d_ptr->proxyModel;
}

void DADataManagerTreeWidget::setDataManager(DADataManager* dataMgr)
{
    DA_D(d);
    // 断开旧 dataMgr 的批量信号连接
    DADataManager* oldMgr = d->model->getDataManager();
    if (oldMgr && oldMgr != dataMgr) {
        disconnect(oldMgr, &DADataManager::datasBatchAdded, this, &DADataManagerTreeWidget::onDatasBatchAdded);
    }
    d->model->setDataManager(dataMgr);
    if (dataMgr) {
        connect(dataMgr, &DADataManager::datasBatchAdded, this, &DADataManagerTreeWidget::onDatasBatchAdded);
    }
    // 更新补全器
    updateCompleterModel();
}

DADataManager* DADataManagerTreeWidget::getDataManager() const
{
    return d_ptr->model->getDataManager();
}

void DADataManagerTreeWidget::setExpandDataframeToSeries(bool on)
{
    DA_D(d);
    d->model->setExpandDataframeToSeries(on);
}

bool DADataManagerTreeWidget::isExpandDataframeToSeries() const
{
    DA_DC(d);
    return d->model->isExpandDataframeToSeries();
}

void DADataManagerTreeWidget::setColumnStyle(DADataManagerTreeModel::ColumnStyle style)
{
    DA_D(d);
    d->model->setColumnStyle(style);
}

DADataManagerTreeModel::ColumnStyle DADataManagerTreeWidget::getColumnStyle() const
{
    DA_DC(d);
    return d->model->getColumnStyle();
}

/**
 * @brief 获取当前选中的数据
 * @return 如果选中的是series，返回的是series对应dataframe的DAData
 */
DAData DADataManagerTreeWidget::getCurrentSelectData() const
{
    DA_DC(d);
    auto item = d->getCurrentSelectItem();
    if (!item) {
        return DAData();
    }
    return d->model->itemToData(item);
}

/**
 * @brief 获取所有选择的数据
 * @return
 */
QList< DAData > DADataManagerTreeWidget::getAllSelectDatas() const
{
    DA_DC(d);
    QItemSelectionModel* selectionModel = ui->treeView->selectionModel();
    if (!selectionModel) {
        return QList< DAData >();
    }
    // 获取所有选中的索引
    QList< DAData > res;
    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    for (const QModelIndex& index : selectedIndexes) {
        // 只处理第一级（父节点是无效索引的根节点）
        if (index.parent().isValid()) {
            // 不是第一级节点
            continue;
        }

        // 获取对应的QStandardItem
        const QModelIndex srcIndex = d->proxyModel->mapToSource(index);
        QStandardItem* item        = d->model->itemFromIndex(srcIndex);
        if (item && DADataManagerTreeModel::isDataframeItem(item)) {
            DAData data = DADataManagerTreeModel::itemToData(item);
            res.append(data);
        }
    }
    return res;
}

/**
 * @brief 当前是否选中了dataframe，选中了dataframe下面的series会返回false
 * @return
 */
bool DADataManagerTreeWidget::isSelectDataframe() const
{
    DA_DC(d);
    auto item = d->getCurrentSelectItem();
    if (!item) {
        return false;
    }
    return d->model->isDataframeItem(item);
}

/**
 * @brief 当前是否选中了dataframe下面的series
 * @return
 */
bool DADataManagerTreeWidget::isSelectDataframeSeries() const
{
    DA_DC(d);
    auto item = d->getCurrentSelectItem();
    if (!item) {
        return false;
    }
    return d->model->isDataframeSeriesItem(item);
}

/**
 * @brief 获取当前选中的series的名字
 * @return
 */
QString DADataManagerTreeWidget::getCurrentSelectSeriesName() const
{
    DA_DC(d);
    auto item = d->getCurrentSelectItem();
    if (!item) {
        return QString();
    }
    if (!(d->model->isDataframeSeriesItem(item))) {
        return QString();
    }
    return item->text();
}

/**
 * @brief 展开所有
 */
void DADataManagerTreeWidget::expandAll()
{
    ui->treeView->expandAll();
}

void DADataManagerTreeWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

/**
 * @brief 展开树形所有节点
 */
void DADataManagerTreeWidget::onToolButtonExpandClicked()
{
    ui->treeView->expandAll();
}

/**
 * @brief 收起树形说有节点
 */
void DADataManagerTreeWidget::onToolButtonCollapseClicked()
{
    ui->treeView->collapseAll();
}

/**
 * @brief DADataManagerTreeWidget::onComboBoxEditTextChanged
 * @param text
 */
void DADataManagerTreeWidget::onComboBoxEditTextChanged(const QString& text)
{
    // 使用定时器延迟过滤，避免频繁刷新
    DA_D(d);
    d->filterTimer->start();
}

void DADataManagerTreeWidget::updateCompleterModel()
{
    DA_D(d);
    DADataManager* dmgr = getDataManager();
    if (!dmgr) {
        return;
    }
    QStringList names;
    const QList< DAData > datas = dmgr->getAllDatas();
    for (const DAData& data : datas) {
        if (!data.isDataFrame()) {
            continue;
        }
        names.append(data.getName());
        DAPyDataFrame df = data.toDataFrame();
        names += df.columns();
    }
    names.removeDuplicates();  // 原地去重，保留原顺序
    QStringListModel* model = qobject_cast< QStringListModel* >(d->completer->model());
    if (model) {
        model->setStringList(names);
    }
}

void DADataManagerTreeWidget::applyFilter()
{
    updateFilter(ui->comboBoxFilter->currentText());
}

void DADataManagerTreeWidget::onTreeViewDoubleClicked(const QModelIndex& index)
{

    DA_D(d);
    // 属性列（第二列）双击等同于名称列双击
    QModelIndex proxyIndex = (index.column() == 0) ? index : index.siblingAtColumn(0);
    if (proxyIndex.parent().isValid()) {
        // 子节点双击
        const QModelIndex srcIndex = d->proxyModel->mapToSource(proxyIndex);
        QStandardItem* item        = d->model->itemFromIndex(srcIndex);
        if (item) {
            DAData data = DADataManagerTreeModel::itemToData(item);
            if (!data.isNull()) {
                Q_EMIT dataSeriesDbClicked(data, item->text());
            }
        }
        return;
    } else {
        // 根节点双击不处理
        const QModelIndex srcIndex = d->proxyModel->mapToSource(proxyIndex);
        QStandardItem* item        = d->model->itemFromIndex(srcIndex);
        if (item) {
            DAData data = DADataManagerTreeModel::itemToData(item);
            if (!data.isNull()) {
                Q_EMIT dataDbClicked(data);
            }
        }
    }
}

/**
 * @brief 树形单击
 *
 * 仅子节点(dataframe.series)单击触发 dataSeriesClicked，
 * 根节点(dataframe)单击不触发，避免误拾取整个 dataframe。
 */
void DADataManagerTreeWidget::onTreeViewClicked(const QModelIndex& index)
{
    // 属性列（第二列）点击等同于名称列点击
    QModelIndex proxyIndex = (index.column() == 0) ? index : index.siblingAtColumn(0);
    if (!proxyIndex.parent().isValid()) {
        return;
    }
    DA_D(d);
    const QModelIndex srcIndex = d->proxyModel->mapToSource(proxyIndex);
    QStandardItem* item        = d->model->itemFromIndex(srcIndex);
    if (!item) {
        return;
    }
    DAData data = DADataManagerTreeModel::itemToData(item);
    if (data.isNull()) {
        return;
    }
    Q_EMIT dataSeriesClicked(data, item->text());
}

void DADataManagerTreeWidget::updateFilter(const QString& text)
{
    DA_D(d);
    d->proxyModel->setFilterText(text);

    // 如果过滤文本不为空，展开所有匹配的项
    if (!text.isEmpty()) {
        ui->treeView->expandAll();
    }
}

QStringList DADataManagerTreeWidget::collectAllDataNames() const
{
    QStringList names;

    DA_DC(d);
    if (!d->model) {
        return names;
    }

    // 从模型中获取所有数据名称
    names = d->model->getAllDataframeNames();

    // 添加搜索提示
    names.prepend(tr("Search..."));  // cn:搜索

    return names;
}

void DADataManagerTreeWidget::onDatasBatchAdded()
{
    // 批量添加完成，立即停止去抖定时器并同步刷新
    d_ptr->completerDebounceTimer->stop();
    updateCompleterModel();
}

}  // end DA
