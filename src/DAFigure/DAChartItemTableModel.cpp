﻿#include "DAChartItemTableModel.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_text.h"
#include "DAChartUtil.h"
namespace DA
{
class DAChartItemTableModelPrivate
{
    DA_IMPL_PUBLIC(DAChartItemTableModel)
public:
    QList< QwtPlotItem* > m_items;
    QMap< QwtPlotItem*, int > m_itemsRowCount;
    QMap< QwtPlotItem*, QColor > m_itemsColor;          ///< 记录item的颜色，以免频繁读取
    QMap< QwtPlotItem*, int > m_itemsColumnStartIndex;  ///< 记录数据开始的那一列的索引
    int m_rowCount;
    bool m_enableBkColor;  ///< 是否允许背景色
    int m_bkAlpha;         ///< 背景透明度
    QMap< int, QPair< QwtPlotItem*, int > > m_columnMap;
    DAChartItemTableModelPrivate(DAChartItemTableModel* d)
        : q_ptr(d), m_rowCount(0), m_enableBkColor(true), m_bkAlpha(30)
    {
    }
};

DAChartItemTableModel::DAChartItemTableModel(QObject* p)
    : QAbstractTableModel(p), d_ptr(new DAChartItemTableModelPrivate(this))
{
}

DAChartItemTableModel::~DAChartItemTableModel()
{
}

void DAChartItemTableModel::setPlotItems(const QList< QwtPlotItem* >& items)
{
    beginResetModel();
    d_ptr->m_items = items;
    updateRowCount();
    updateColumnCount();
    updateItemColor();
    endResetModel();
}

const QList< QwtPlotItem* >& DAChartItemTableModel::getPlotItems() const
{
    return d_ptr->m_items;
}

void DAChartItemTableModel::clear()
{
    beginResetModel();
    d_ptr->m_rowCount = 0;
    d_ptr->m_items.clear();
    d_ptr->m_itemsRowCount.clear();
    d_ptr->m_itemsColor.clear();
    d_ptr->m_itemsColumnStartIndex.clear();
    d_ptr->m_columnMap.clear();
    endResetModel();
}

int DAChartItemTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return d_ptr->m_rowCount;
}

int DAChartItemTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return d_ptr->m_columnMap.size();
}

QVariant DAChartItemTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (Qt::Horizontal == orientation) {  //说明是水平表头
        int index         = 0;
        QwtPlotItem* item = getItemFromCol(section, &index);
        if (!item)
            return QVariant();
        QString name = QStringLiteral("%1\n%2").arg(item->title().text()).arg(getItemDimDescribe(item, index));
        return name;
    } else {
        return (section + 1);
    }
    return QVariant();
}

QVariant DAChartItemTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::TextAlignmentRole) {  //返回的是对其方式
        return int(Qt::AlignRight | Qt::AlignVCenter);
    } else if (role == Qt::DisplayRole) {  //返回的是现实内容
        QwtPlotItem* item = getItemFromCol(index.column());
        if (!item)
            return QVariant();

        int col = d_ptr->m_itemsColumnStartIndex.value(item, 0);
        col     = index.column() - col;

        if (index.row() >= getItemRowCount(item))
            return QVariant();
        return getItemData(index.row(), col, item);
    } else if (role == Qt::BackgroundColorRole) {
        if (!d_ptr->m_enableBkColor)
            return QVariant();
        QwtPlotItem* item = getItemFromCol(index.column());
        if (!item)
            return QVariant();
        return getItemColor(item);
    }
    return QVariant();
}

bool DAChartItemTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;
    if (Qt::EditRole != role)
        return false;
    QwtPlotItem* item = getItemFromCol(index.column());
    if (nullptr == item)
        return false;
    int col = getItemsColumnStartIndex(item);
    if (col < 0)
        return false;
    col = index.column() - col;
    return setPlotItemData(index.row(), col, item, value);
}

Qt::ItemFlags DAChartItemTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

void DAChartItemTableModel::enableBackgroundColor(bool enable, int alpha)
{
    beginResetModel();
    d_ptr->m_enableBkColor = enable;
    d_ptr->m_bkAlpha       = alpha;
    if (d_ptr->m_enableBkColor) {
        updateItemColor();
    }
    endResetModel();
}

double DAChartItemTableModel::nan()
{
    return std::numeric_limits< double >::quiet_NaN();
}
///
/// \brief 数据开始的那一列列号
/// \param item
/// \return 如果没有对应的item返回-1
///
int DAChartItemTableModel::getItemsColumnStartIndex(QwtPlotItem* item) const
{
    return d_ptr->m_itemsColumnStartIndex.value(item, -1);
}
///
/// \brief 获取item对应的列范围
/// \param startCol
/// \param endCol
///
void DAChartItemTableModel::getItemColumnRange(QwtPlotItem* item, int* startCol, int* endCol) const
{
    int startIndex = getItemsColumnStartIndex(item);
    if (startCol)
        *startCol = startIndex;
    if (endCol) {
        int dim2 = calcItemDataColumnCount(item);
        *endCol  = startIndex + dim2 - 1;
    }
}
///
/// \brief 计算QwtPlotMultiBarChart的最大维度
/// \param p
/// \return QwtSetSample.set的最大尺寸+1，其中加1是对应QwtSetSample.value
///
int DAChartItemTableModel::calcPlotMultiBarChartDim(const QwtPlotMultiBarChart* p)
{
    int maxDim     = 0;
    const int size = p->dataSize();
    for (int i = 0; i < size; ++i) {
        int s = p->sample(i).set.size();
        if (s > maxDim) {
            maxDim = s;
        }
    }
    return maxDim + 1;
}
///
/// \brief 更新行数
///
void DAChartItemTableModel::updateRowCount()
{
    d_ptr->m_rowCount = 0;
    d_ptr->m_itemsRowCount.clear();
    for (auto i = d_ptr->m_items.begin(); i != d_ptr->m_items.end(); ++i) {
        int dataCount = calcItemDataRowCount(*i);
        if (dataCount > d_ptr->m_rowCount) {
            d_ptr->m_rowCount = dataCount;
        }
        d_ptr->m_itemsRowCount[ *i ] = dataCount;
    }
}
///
/// \brief 更新列数
///
void DAChartItemTableModel::updateColumnCount()
{
    d_ptr->m_columnMap.clear();
    d_ptr->m_itemsColumnStartIndex.clear();
    for (auto i = d_ptr->m_items.begin(); i != d_ptr->m_items.end(); ++i) {
        int dim = calcItemDataColumnCount(*i);
        if (dim > 0) {
            int startIndex                       = d_ptr->m_columnMap.size();
            d_ptr->m_itemsColumnStartIndex[ *i ] = startIndex;
            for (int d = 0; d < dim; ++d) {
                d_ptr->m_columnMap[ startIndex + d ] = qMakePair< QwtPlotItem*, int >(*i, d);
            }
        }
    }
}
///
/// \brief 更新颜色
///
void DAChartItemTableModel::updateItemColor()
{
    d_ptr->m_itemsColor.clear();
    for (auto i = d_ptr->m_items.begin(); i != d_ptr->m_items.end(); ++i) {
        QColor c = DAChartUtil::getPlotItemColor(*i);
        if (!c.isValid()) {
            c = Qt::black;
        }
        if (d_ptr->m_bkAlpha < 255) {
            c.setAlpha(d_ptr->m_bkAlpha);
        }
        d_ptr->m_itemsColor[ *i ] = c;
    }
}

///
/// \brief 通过列号获取item
/// \param col 列的序号
/// \return 如果没有，返回nullptr
///
QwtPlotItem* DAChartItemTableModel::getItemFromCol(int col, int* dataColumnDim) const
{
    QPair< QwtPlotItem*, int > pair = d_ptr->m_columnMap.value(col, qMakePair< QwtPlotItem*, int >(nullptr, 0));
    if (dataColumnDim) {
        *dataColumnDim = pair.second;
    }
    return pair.first;
}
///
/// \brief 通过列号获取item的名字
/// \param col 列的序号
/// \return item的名字
///
QString DAChartItemTableModel::getItemNameFromCol(int col) const
{
    QwtPlotItem* item = getItemFromCol(col);
    if (item) {
        return item->title().text();
    }
    return QString();
}
///
/// \brief 获取最大的行数
/// 重载此函数，可以让表格显示的行数
/// \param item
/// \return
///
int DAChartItemTableModel::calcItemDataRowCount(QwtPlotItem* item) const
{
    return DAChartUtil::getItemDataSize(item);
}

///
/// \brief 获取曲线的维度
///
/// \param item
/// \return
///
int DAChartItemTableModel::calcItemDataColumnCount(QwtPlotItem* item) const
{
    return getItemColumnCount(item);
}

double DAChartItemTableModel::getItemData(int row, int col, QwtPlotItem* item) const
{
    if (nullptr == item || col < 0) {
        return nan();
    }
#if QwtPlotItemDataModel_Use_Dynamic_Cast
    //读取数据
    if (const QwtSeriesStore< QPointF >* p = dynamic_cast< const QwtSeriesStore< QPointF >* >(item)) {
        if (row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).x();
            case 1:
                return p->sample(row).y();
            }
        }
    } else if (const QwtSeriesStore< QwtIntervalSample >* p = dynamic_cast< const QwtSeriesStore< QwtIntervalSample >* >(item)) {
        if (row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).value;
            case 1:
                return p->sample(row).interval.minValue();
            case 2:
                return p->sample(row).interval.maxValue();
            }
        }
    } else if (const QwtSeriesStore< QwtSetSample >* p = dynamic_cast< const QwtSeriesStore< QwtSetSample >* >(item)) {
        if (row < p->dataSize()) {
            const QwtSetSample& s = p->sample(row);
            if (0 == col) {
                return s.value;
            } else {
                int setIndex = col - 1;
                if (setIndex >= 0 && setIndex < s.set.size()) {
                    return s.set.value(setIndex);
                }
            }
        }
    } else if (const QwtSeriesStore< QwtPoint3D >* p = dynamic_cast< const QwtSeriesStore< QwtPoint3D >* >(item)) {
        if (row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).x();
            case 1:
                return p->sample(row).y();
            case 2:
                return p->sample(row).z();
            }
        }
    } else if (const QwtSeriesStore< QwtOHLCSample >* p = dynamic_cast< const QwtSeriesStore< QwtOHLCSample >* >(item)) {
        if (row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).time;
            case 1:
                return p->sample(row).open;
            case 2:
                return p->sample(row).high;
            case 3:
                return p->sample(row).low;
            case 4:
                return p->sample(row).close;
            }
        }
    }
#else
    int rtti = item->rtti();
    switch (rtti) {
    case QwtPlotItem::Rtti_PlotCurve: {
        const QwtPlotCurve* p = static_cast< const QwtPlotCurve* >(item);
        if ((size_t)row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).x();
            case 1:
                return p->sample(row).y();
            }
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotBarChart: {
        const QwtPlotBarChart* p = static_cast< const QwtPlotBarChart* >(item);
        if ((size_t)row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).x();
            case 1:
                return p->sample(row).y();
            }
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotSpectroCurve: {
        const QwtPlotSpectroCurve* p = static_cast< const QwtPlotSpectroCurve* >(item);
        if ((size_t)row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).x();
            case 1:
                return p->sample(row).y();
            case 2:
                return p->sample(row).z();
            }
        }
        break;
    }

    case QwtPlotItem::Rtti_PlotIntervalCurve: {
        const QwtPlotIntervalCurve* p = static_cast< const QwtPlotIntervalCurve* >(item);
        if ((size_t)row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).value;
            case 1:
                return p->sample(row).interval.minValue();
            case 2:
                return p->sample(row).interval.maxValue();
            }
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotHistogram: {
        const QwtPlotHistogram* p = static_cast< const QwtPlotHistogram* >(item);
        if ((size_t)row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).value;
            case 1:
                return p->sample(row).interval.minValue();
            case 2:
                return p->sample(row).interval.maxValue();
            }
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotTradingCurve: {
        const QwtPlotTradingCurve* p = static_cast< const QwtPlotTradingCurve* >(item);
        if ((size_t)row < p->dataSize()) {
            switch (col) {
            case 0:
                return p->sample(row).time;
            case 1:
                return p->sample(row).open;
            case 2:
                return p->sample(row).high;
            case 3:
                return p->sample(row).low;
            case 4:
                return p->sample(row).close;
            }
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotMultiBarChart: {
        const QwtPlotMultiBarChart* p = static_cast< const QwtPlotMultiBarChart* >(item);
        if ((size_t)row < p->dataSize()) {
            const QwtSetSample& s = p->sample(row);
            if (0 == col) {
                return s.value;
            } else {
                int setIndex = col - 1;
                if (setIndex >= 0 && setIndex < s.set.size()) {
                    return s.set.value(setIndex);
                }
            }
        }
        break;
    }
    default:
        break;
    }

#endif

    return nan();
}
///
/// \brief 获取绘图数据维度的描述
/// \param item
/// \param index
/// \return
///
QString DAChartItemTableModel::getItemDimDescribe(QwtPlotItem* item, int index) const
{
#if QwtPlotItemDataModel_Use_Dynamic_Cast
    if (const QwtSeriesStore< QPointF >* p = dynamic_cast< const QwtSeriesStore< QPointF >* >(item)) {
        Q_UNUSED(p);
        switch (index) {
        case 0:
            return tr("x");
        case 1:
            return tr("y");
        default:
            return QString();
        }
    } else if (const QwtSeriesStore< QwtIntervalSample >* p = dynamic_cast< const QwtSeriesStore< QwtIntervalSample >* >(item)) {
        Q_UNUSED(p);
        switch (index) {
        case 0:
            return tr("value");
        case 1:
            return tr("min");
        case 2:
            return tr("max");
        default:
            return QString();
        }
    } else if (const QwtSeriesStore< QwtSetSample >* p = dynamic_cast< const QwtSeriesStore< QwtSetSample >* >(item)) {
        Q_UNUSED(p);
        if (0 == index) {
            return tr("value");
        } else {
            return tr("set %1").arg(index);
        }
    } else if (const QwtSeriesStore< QwtPoint3D >* p = dynamic_cast< const QwtSeriesStore< QwtPoint3D >* >(item)) {
        Q_UNUSED(p);
        switch (index) {
        case 0:
            return tr("x");
        case 1:
            return tr("y");
        case 2:
            return tr("z");
        default:
            return QString();
        }
    } else if (const QwtSeriesStore< QwtOHLCSample >* p = dynamic_cast< const QwtSeriesStore< QwtOHLCSample >* >(item)) {

        switch (index) {
        case 0:
            return tr("time");
        case 1:
            return tr("open");
        case 2:
            return tr("high");
        case 3:
            return tr("low");
        case 4:
            return tr("close");
        }
    }
#else
    int rtti = item->rtti();
    switch (rtti) {
    case QwtPlotItem::Rtti_PlotCurve:
    case QwtPlotItem::Rtti_PlotBarChart: {
        switch (index) {
        case 0:
            return tr("x");
        case 1:
            return tr("y");
        default:
            return QString();
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotSpectroCurve: {
        switch (index) {
        case 0:
            return tr("x");
        case 1:
            return tr("y");
        case 2:
            return tr("z");
        default:
            return QString();
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotIntervalCurve:
    case QwtPlotItem::Rtti_PlotHistogram: {
        switch (index) {
        case 0:
            return tr("value");
        case 1:
            return tr("min");
        case 2:
            return tr("max");
        default:
            return QString();
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotTradingCurve: {
        switch (index) {
        case 0:
            return tr("time");
        case 1:
            return tr("open");
        case 2:
            return tr("high");
        case 3:
            return tr("low");
        case 4:
            return tr("close");
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotMultiBarChart: {
        if (0 == index) {
            return tr("value");
        } else {
            return tr("set %1").arg(index);
        }
        break;
    }
    default:
        break;
    }
#endif
    return QString();
}

void DAChartItemTableModel::setSeriesPointFValue(QPointF& p, int col, double val)
{
    switch (col) {
    case 0:
        p.rx() = val;
        break;
    case 1:
        p.ry() = val;
        break;
    default:
        break;
    }
}

void DAChartItemTableModel::setSeriesPoint3dValue(QwtPoint3D& p, int col, double val)
{
    switch (col) {
    case 0:
        p.setX(val);
        break;
    case 1:
        p.setY(val);
        break;
    case 2:
        p.setZ(val);
        break;
    default:
        break;
    }
}

void DAChartItemTableModel::setSeriesIntervalValue(QwtIntervalSample& p, int col, double val)
{
    switch (col) {
    case 0:
        p.value = val;
        break;
    case 1:
        p.interval.setMinValue(val);
        break;
    case 2:
        p.interval.setMaxValue(val);
        break;
    default:
        break;
    }
}

void DAChartItemTableModel::setSeriesSetsampleValue(QwtSetSample& p, int col, double val)
{
    if (0 == col) {
        p.value = val;
    } else {
        if (col - 1 < p.set.size()) {
            p.set[ col - 1 ] = val;
        }
    }
}

void DAChartItemTableModel::setSeriesOHLCsampleValue(QwtOHLCSample& p, int col, double val)
{
    switch (col) {
    case 0:
        p.time = val;
        break;
    case 1:
        p.open = val;
        break;
    case 2:
        p.high = val;
        break;
    case 3:
        p.low = val;
        break;
    case 4:
        p.close = val;
        break;
    default:
        break;
    }
}

///
/// \brief 依据显示规则设置数据
/// \param row 实际数据的行号
/// \param col 实际数据的列号，如果是表格列要减去这个数据的第一个维度的列数
/// \param item 数据指针
/// \param var 设置的值
/// \return 成功设置返回true
///
bool DAChartItemTableModel::setPlotItemData(int row, int col, QwtPlotItem* item, const QVariant& var)
{
    if (row < 0 || col < 0 || !var.isValid() || nullptr == item)
        return false;
    bool isOK = false;
    double d  = var.toDouble(&isOK);
    if (!isOK)
        return false;

    int rtti = item->rtti();
    switch (rtti) {
    case QwtPlotItem::Rtti_PlotCurve: {
        if (col > 1)
            return false;
        QwtPlotCurve* p = static_cast< QwtPlotCurve* >(item);
        QVector< QPointF > xys;
        DAChartUtil::getXYDatas(xys, p);
        if (row < xys.size()) {
            setSeriesPointFValue(xys[ row ], col, d);
            p->setSamples(xys);
            return true;
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotBarChart: {
        QwtPlotBarChart* p = static_cast< QwtPlotBarChart* >(item);
        if (col > 1)
            return false;
        QVector< QPointF > xys;
        DAChartUtil::getXYDatas(xys, p);
        if (row < xys.size()) {
            setSeriesPointFValue(xys[ row ], col, d);
            p->setSamples(xys);
            return true;
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotSpectroCurve: {
        QwtPlotSpectroCurve* p = static_cast< QwtPlotSpectroCurve* >(item);
        if (col > 2)
            return false;
        QVector< QwtPoint3D > xyzs;
        DAChartUtil::getXYZDatas(xyzs, p);
        if (row < xyzs.size()) {
            setSeriesPoint3dValue(xyzs[ row ], col, d);
            p->setSamples(xyzs);
            return true;
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotIntervalCurve: {
        QwtPlotIntervalCurve* p = static_cast< QwtPlotIntervalCurve* >(item);
        if (col > 2)
            return false;
        QVector< QwtIntervalSample > samples;
        DAChartUtil::getIntervalSampleDatas(samples, p);
        if (row < samples.size()) {
            setSeriesIntervalValue(samples[ row ], col, d);
            p->setSamples(samples);
            return true;
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotHistogram: {
        QwtPlotHistogram* p = static_cast< QwtPlotHistogram* >(item);
        if (col > 2)
            return false;
        QVector< QwtIntervalSample > samples;
        DAChartUtil::getIntervalSampleDatas(samples, p);
        if (row < samples.size()) {
            setSeriesIntervalValue(samples[ row ], col, d);
            p->setSamples(samples);
            return true;
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotTradingCurve: {
        QwtPlotTradingCurve* p = static_cast< QwtPlotTradingCurve* >(item);
        if (col > 4)
            return false;
        QVector< QwtOHLCSample > samples;
        DAChartUtil::getSeriesData< QwtOHLCSample >(samples, p);
        if (row < samples.size()) {
            setSeriesOHLCsampleValue(samples[ row ], col, d);
            p->setSamples(samples);
            return true;
        }
        break;
    }
    case QwtPlotItem::Rtti_PlotMultiBarChart: {
        QwtPlotMultiBarChart* p = static_cast< QwtPlotMultiBarChart* >(item);
        QVector< QwtSetSample > samples;
        DAChartUtil::getSeriesData< QwtSetSample >(samples, p);
        if (row < samples.size()) {
            setSeriesSetsampleValue(samples[ row ], col, d);
            p->setSamples(samples);
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

///
/// \brief 获取item的颜色
/// \param item
/// \return QColor
///
QColor DAChartItemTableModel::getItemColor(QwtPlotItem* item) const
{
    return d_ptr->m_itemsColor.value(item, QColor());
}

int DAChartItemTableModel::getItemRowCount(QwtPlotItem* item) const
{
    return d_ptr->m_itemsRowCount.value(item, 0);
}

///
/// \brief QwtPlotItemDataModel::getItemColumnCount
/// \param item
/// \return
///
int DAChartItemTableModel::getItemColumnCount(QwtPlotItem* item)
{
#if QwtPlotItemDataModel_Use_Dynamic_Cast
    if (const QwtSeriesStore< QPointF >* p = dynamic_cast< const QwtSeriesStore< QPointF >* >(item)) {
        Q_UNUSED(p);
        return 2;
    } else if (const QwtSeriesStore< QwtIntervalSample >* p = dynamic_cast< const QwtSeriesStore< QwtIntervalSample >* >(item)) {
        Q_UNUSED(p);
        return 3;
    } else if (const QwtSeriesStore< QwtSetSample >* p = dynamic_cast< const QwtSeriesStore< QwtSetSample >* >(item)) {
        int maxDim     = 0;
        const int size = p->dataSize();
        for (int i = 0; i < size; ++i) {
            int s = p->sample(i).set.size();
            if (s > maxDim) {
                maxDim = s;
            }
        }
        return maxDim + 1;
    } else if (const QwtSeriesStore< QwtPoint3D >* p = dynamic_cast< const QwtSeriesStore< QwtPoint3D >* >(item)) {
        Q_UNUSED(p);
        return 3;
    } else if (const QwtSeriesStore< QwtOHLCSample >* p = dynamic_cast< const QwtSeriesStore< QwtOHLCSample >* >(item)) {
        Q_UNUSED(p);
        return 5;
    }
#else
    int rtti = item->rtti();
    switch (rtti) {
    case QwtPlotItem::Rtti_PlotCurve:
    case QwtPlotItem::Rtti_PlotBarChart:
        return 2;
    case QwtPlotItem::Rtti_PlotSpectroCurve:
    case QwtPlotItem::Rtti_PlotIntervalCurve:
    case QwtPlotItem::Rtti_PlotHistogram:
        return 3;
    case QwtPlotItem::Rtti_PlotTradingCurve:
        return 5;
    case QwtPlotItem::Rtti_PlotMultiBarChart: {
        const QwtPlotMultiBarChart* p = static_cast< const QwtPlotMultiBarChart* >(item);
        return calcPlotMultiBarChartDim(p);
    }
    default:
        return 0;
    }
#endif
}
}  // End Of Namespace DA
