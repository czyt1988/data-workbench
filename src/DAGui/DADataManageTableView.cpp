#include "DADataManageTableView.h"
#include <QHeaderView>
#include <QDebug>
#include "DADataManagerTableModel.h"
#include "DADataManager.h"

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

namespace DA
{

//===================================================
// DADataManageTableView
//===================================================
DADataManageTableView::DADataManageTableView(QWidget* parent) : QTableView(parent)
{
    setModel(new DADataManagerTableModel(this));
    init();
}

DADataManageTableView::DADataManageTableView(DADataManager* dmgr, QWidget* parent) : QTableView(parent)
{
    setModel(new DADataManagerTableModel(dmgr, this));
    init();
}
/**
 * @brief 设置datamanager
 * @param dmgr
 */
void DADataManageTableView::setDataManager(DADataManager* dmgr)
{
    DADataManagerTableModel* m = qobject_cast< DADataManagerTableModel* >(model());
    if (m) {
        m->setDataManager(dmgr);
    }
}

void DADataManageTableView::init()
{
    setShowGrid(false);
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    //允许编辑
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    horizontalHeader()->setStretchLastSection(true);
    QFontMetrics fm = fontMetrics();
    //高度为行高的1.2
    verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.2);
    verticalHeader()->hide();
    //
    connect(this, &DADataManageTableView::doubleClicked, this, &DADataManageTableView::onTableViewDoubleClicked);
}
/**
 * @brief 获取一个选中的数据
 * @return
 */
DAData DADataManageTableView::getOneSelectData() const
{
    QItemSelectionModel* selectModel = selectionModel();
    if (nullptr == selectModel) {
        //说明没有任何选中
        return DAData();
    }
    if (!selectModel->hasSelection()) {
        //说明没有任何选中
        return DAData();
    }
    QModelIndexList sels = selectModel->selectedRows();
    if (sels.isEmpty()) {
        //说明没有任何选中
        return DAData();
    }
    QVariant v = sels.last().data(DA_ROLE_DADATAMANAGERTABLEMODEL_DATA);
    if (!v.isValid()) {
        //说明没有任何选中
        qWarning() << tr("The item is selected in the data management table, "
                         "but the corresponding data cannot be obtained");  //在数据管理表中选中了条目，但无法获取对应数据
        return DAData();
    }
    DAData d = v.value< DA::DAData >();
    return d;
}

/**
 * @brief 获取所有选中的数据
 * @return
 */
QList< DAData > DADataManageTableView::getSelectDatas() const
{
    QList< DAData > res;
    QItemSelectionModel* selectModel = selectionModel();
    if (nullptr == selectModel) {
        //说明没有任何选中
        return res;
    }
    if (!selectModel->hasSelection()) {
        //说明没有任何选中
        return res;
    }
    QModelIndexList sels = selectModel->selectedRows();
    if (sels.isEmpty()) {
        //说明没有任何选中
        return res;
    }
    for (const QModelIndex& index : qAsConst(sels)) {

        QVariant v = index.data(DA_ROLE_DADATAMANAGERTABLEMODEL_DATA);
        if (!v.isValid()) {
            //说明没有任何选中
            continue;
        }
        DAData d = v.value< DA::DAData >();
        res.append(d);
    }
    return res;
}

void DADataManageTableView::onTableViewDoubleClicked(const QModelIndex& index)
{
    QVariant v = index.data(DA_ROLE_DADATAMANAGERTABLEMODEL_DATA);
    if (!v.isValid()) {
        return;
    }
    DAData d = v.value< DA::DAData >();
    emit dataDbClicked(d);
}

}
