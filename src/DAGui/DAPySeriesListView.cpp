#include "DAPySeriesListView.h"
// stl
#include <algorithm>
// Qt
#include <QStandardItemModel>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
// DA
#include "MimeData/DAMimeDataForData.h"
#include "MimeData/DAMimeDataFormats.h"
#include "Models/DADataManagerTreeModel.h"
namespace DA
{

class DAPySeriesListView::PrivateData
{
    DA_DECLARE_PUBLIC(DAPySeriesListView)
public:
    PrivateData(DAPySeriesListView* p);
    void addSeries(const DAData& df, const QStringList& series);
    QStandardItem* createDataframeSeriesItem(const DAData& data, const QString& name);
    DAData itemToData(QStandardItem* item);

public:
    QStandardItemModel* listModel { nullptr };
    AcceptMode acceptMode { AcceptMultDataframeMultSeries };
    QList< QPair< DAData, QStringList > > series;  // 真实数据
    DADataManager* dataMgr { nullptr };
};

DAPySeriesListView::PrivateData::PrivateData(DAPySeriesListView* p) : q_ptr(p)
{
    listModel = new QStandardItemModel(p);
    q_ptr->setModel(listModel);
}

void DAPySeriesListView::PrivateData::addSeries(const DAData& df, const QStringList& series)
{
}

QStandardItem* DAPySeriesListView::PrivateData::createDataframeSeriesItem(const DAData& data, const QString& name)
{
    return DADataManagerTreeModel::createDataFrameSeriesItem(name, data);
}

DAData DAPySeriesListView::PrivateData::itemToData(QStandardItem* item)
{
    return DADataManagerTreeModel::itemToData(item);
}

//===============================================================
// DAPySeriesListView
//===============================================================

DAPySeriesListView::DAPySeriesListView(QWidget* par) : QListView(par), DA_PIMPL_CONSTRUCT
{
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DropOnly);
}

DAPySeriesListView::~DAPySeriesListView()
{
}

void DAPySeriesListView::setDataManager(DADataManager* dataMgr)
{
    d_ptr->dataMgr = dataMgr;
}

DADataManager* DAPySeriesListView::getDataManager() const
{
    return d_ptr->dataMgr;
}

void DAPySeriesListView::setAcceptMode(AcceptMode mode)
{
    d_ptr->acceptMode = mode;
}

DAPySeriesListView::AcceptMode DAPySeriesListView::getAcceptMode() const
{
    return d_ptr->acceptMode;
}

/**
 * @brief 添加序列
 * @param data 序列对应的数据，对应dataframe的data
 * @param name 序列名字
 */
void DAPySeriesListView::addSeries(const DAData& dataframeData, const QString& name)
{
    DA_D(d);
    if (!d->dataMgr) {
        d->dataMgr = dataframeData.getDataManager();
    }
    // 对于只接收一个的情况，需要先清除
    if (d->acceptMode == AcceptOneSeries) {
        d->series.clear();  // 加入之前，先清除掉内容
        d->listModel->clear();
    }
    // 1. 先确保数据区有这一条
    auto it = std::find_if(d->series.begin(), d->series.end(), [ & ](const auto& p) { return p.first == dataframeData; });
    if (it == d->series.end()) {
        it = d->series.insert(d->series.end(), { dataframeData, {} });
    }

    if (it->second.contains(name)) {  // 已经存在，直接返回
        return;
    }

    it->second.append(name);

    QStandardItem* item = d->createDataframeSeriesItem(dataframeData, name);
    d->listModel->appendRow(item);
    Q_EMIT seriesChanged();
}

/**
 * @brief 移除已经添加的序列
 * @param dataframeData
 * @param name
 */
void DAPySeriesListView::removeSeries(const DAData& dataframeData, const QString& name)
{
    DA_D(d);
    auto it = std::find_if(d->series.begin(), d->series.end(), [ & ](const auto& p) { return p.first == dataframeData; });
    if (it == d->series.end()) {
        return;
    }
    // 找到data后删除name
    it->second.removeAll(name);

    // 调整完数据调整model
    const int rowcnt = d->listModel->rowCount();
    for (int i = rowcnt - 1; i >= 0; --i) {
        QStandardItem* item = d->listModel->item(i);
        if (item->text() == name) {
            DAData data = d->itemToData(item);
            if (data == dataframeData) {
                d->listModel->removeRow(i);
            }
        }
    }
}

/**
 * @brief 移除当前选中
 */
void DAPySeriesListView::removeCurrentSelect()
{
    DA_D(d);
    auto index = currentIndex();
    if (!index.isValid()) {
        return;
    }
    QStandardItem* item = d->listModel->itemFromIndex(index);
    if (!item) {
        return;
    }
    DAData data = d->itemToData(item);
    removeSeries(data, item->text());
}

QList< QPair< DAData, QStringList > > DAPySeriesListView::getSeries() const
{
    return d_ptr->series;
}

void DAPySeriesListView::dragEnterEvent(QDragEnterEvent* event)
{
    if (acceptMimeData(event->mimeData())) {
        // 根据设定模式判断
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DAPySeriesListView::dragMoveEvent(QDragMoveEvent* event)
{
    if (acceptMimeData(event->mimeData())) {
        // 根据设定模式判断
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DAPySeriesListView::dropEvent(QDropEvent* event)
{
    const QMimeData* mime                         = event->mimeData();
    const DAMimeDataForMultDataSeries* seriesMime = qobject_cast< const DAMimeDataForMultDataSeries* >(mime);
    if (!seriesMime) {
        event->ignore();
        return;
    }
    const QList< QPair< DAData, QStringList > >& series = seriesMime->getDADatas();
    for (const auto& p : series) {
        for (const QString& name : p.second) {
            addSeries(p.first, name);
        }
    }
    event->acceptProposedAction();
}

bool DAPySeriesListView::acceptMimeData(const QMimeData* mime) const
{
    if (!mime->hasFormat(DAMIMEDATA_FORMAT_MULT_DADATAS_SERIES)) {
        return false;
    }
    const DAMimeDataForMultDataSeries* seriesMime = qobject_cast< const DAMimeDataForMultDataSeries* >(mime);
    if (!seriesMime) {
        return false;
    }
    return acceptSeries(seriesMime);
}

bool DAPySeriesListView::acceptSeries(const DAMimeDataForMultDataSeries* mime) const
{
    DA_DC(d);
    const QList< QPair< DAData, QStringList > >& datas = mime->getDADatas();
    if (datas.empty()) {
        return false;
    }
    switch (d->acceptMode) {
    case AcceptOneSeries:
        return (datas.size() == 1);
        break;
    case AcceptOneDataframeMultSeries: {
        // 通常情况下，DAMimeDataForMultDataSeries的getDADatas不会出现多个不同的data
        auto firstValue = datas.first();
        return std::all_of(datas.begin(), datas.end(), [ firstValue ](const auto& v) {
            return (v.first == firstValue.first);
        });
    }
    default:
        break;
    }
    return true;
}


}  // end namespace DA
