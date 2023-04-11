#include "DAPySeriesTableModule.h"
#include "pandas/DAPySeries.h"
namespace DA
{

class DAPySeriesTableModulePrivate
{
public:
    DA_IMPL_PUBLIC(DAPySeriesTableModule)
    DAPySeriesTableModulePrivate(DAPySeriesTableModule* p);

public:
    QList< DAPySeries > _series;
    QStringList _header;
};

DAPySeriesTableModulePrivate::DAPySeriesTableModulePrivate(DAPySeriesTableModule* p) : q_ptr(p)
{
}
//===================================================
// DAPySeriesTableModule
//===================================================
DAPySeriesTableModule::DAPySeriesTableModule(QObject* parent)
    : QAbstractTableModel(parent), d_ptr(new DAPySeriesTableModulePrivate(this))
{
}

DAPySeriesTableModule::~DAPySeriesTableModule()
{
}

QVariant DAPySeriesTableModule::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || d_ptr->_series.empty()) {
        return QVariant();
    }
    //剩下都是DisplayRole
    if (Qt::Horizontal == orientation) {  //说明是水平表头

        if (section < d_ptr->_header.size()) {
            return d_ptr->_header[ section ];
        }
        if (section < d_ptr->_series.size()) {
            return d_ptr->_series[ section ].name();
        }
        return QVariant();
    } else {
        return (section + 1);
    }
    return QVariant();
}

int DAPySeriesTableModule::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return d_ptr->_series.size();
}

int DAPySeriesTableModule::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    std::size_t r = 0;
    for (const DAPySeries& ser : qAsConst(d_ptr->_series)) {
        std::size_t s = ser.size();
        if (s > r) {
            r = s;
        }
    }
    return (int)r;
}

Qt::ItemFlags DAPySeriesTableModule::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant DAPySeriesTableModule::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || d_ptr->_series.empty()) {
        return QVariant();
    }
    if (role == Qt::TextAlignmentRole) {
        //返回的是对其方式
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::DisplayRole) {
        int colcnt = d_ptr->_series.size();
        if (index.column() >= colcnt) {
            return QVariant();
        }
        const DAPySeries& ser = d_ptr->_series.at(index.column());
        int rowcnt            = (int)ser.size();
        if (index.row() >= rowcnt) {
            return QVariant();
        }
        return ser[ index.row() ];
    } else if (role == Qt::BackgroundRole) {
        //背景颜色
        return QVariant();
    }
    return QVariant();
}

bool DAPySeriesTableModule::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}

/**
 * @brief 插入series
 * @param s
 */
void DAPySeriesTableModule::appendSeries(const DAPySeries& s)
{
    beginInsertColumns(QModelIndex(), d_ptr->_series.size(), d_ptr->_series.size());
    d_ptr->_series.append(s);
    endInsertColumns();
}

/**
 * @brief 插入series，index如果超出范围，会append
 *
 * 例如[s0,s1],insertSeries(3,s2),结果是[s0,s1,s2]
 * 例如[],insertSeries(1,s1),结果是[s1]
 * @param index
 * @param s
 */
void DAPySeriesTableModule::insertSeries(int c, const DAPySeries& s)
{
    //先看看要刷新哪里
    int oldsize = d_ptr->_series.size();
    if (c >= oldsize) {
        beginInsertColumns(QModelIndex(), oldsize, oldsize);
    } else if (c >= 0) {
        beginInsertColumns(QModelIndex(), c, c);
    } else {  //-1
        beginInsertColumns(QModelIndex(), 0, 0);
    }
    d_ptr->_series.insert(c, s);
    endInsertColumns();
}

/**
 * @brief 把series设置到对应位置，如果有，则替换
 * @param c
 * @param s
 */
void DAPySeriesTableModule::setSeriesAt(int c, const DAPySeries& s)
{
    if (c < 0) {
        beginInsertColumns(QModelIndex(), 0, 0);
        d_ptr->_series.prepend(s);
        endInsertColumns();
    } else if (c < d_ptr->_series.size()) {
        const DAPySeries& oldseries = d_ptr->_series[ c ];
        int oldrow                  = (int)oldseries.size();
        d_ptr->_series[ c ]         = s;
        int newrow                  = (int)s.size();
        emit dataChanged(index(0, c), index(std::max(oldrow, newrow), c));
    } else if (c >= d_ptr->_series.size()) {
        beginInsertColumns(QModelIndex(), d_ptr->_series.size(), d_ptr->_series.size());
        d_ptr->_series.append(s);
        endInsertColumns();
    }
}

/**
 * @brief 清除
 */
void DAPySeriesTableModule::clear()
{
    beginResetModel();
    d_ptr->_series.clear();
    endResetModel();
}

/**
 * @brief 设置表头，如果不设置，则返回的是series的名字作为表头
 * @param head
 */
void DAPySeriesTableModule::setHeaderLabel(const QStringList& head)
{
    d_ptr->_header = head;
}

/**
 * @brief 获取当前维护的series
 * @return
 */
int DAPySeriesTableModule::getSeriesCount() const
{
    return d_ptr->_series.size();
}

QStringList& DAPySeriesTableModule::headerLabel()
{
    return d_ptr->_header;
}

const QStringList& DAPySeriesTableModule::headerLabel() const
{
    return d_ptr->_header;
}

}
