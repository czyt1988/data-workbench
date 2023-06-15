#include "DAPySeriesTableModule.h"
#include "pandas/DAPySeries.h"

namespace DA
{

class DAPySeriesTableModulePrivate
{
public:
    DA_IMPL_PUBLIC(DAPySeriesTableModule)
    DAPySeriesTableModulePrivate(DAPySeriesTableModule* p);

    int columnCount() const;

    int rowCount() const;

public:
    //! 这里都用map来管理，因为不同列有可能是不同属性，目前还没有做一个抽象能囊括所有序列，因此还是需要每种类型添加一个索引
    //! 注意，这里要用qmap，不能用qhash，因为取最大索引是通过qmap的自动排序实现的
    QMap< int, DAPySeries > mSeries;                                    ///< 序列
    QMap< int, DAAutoincrementSeries< double > > mAutoincrementSeries;  ///< 自增序列
    QMap< int, QString > mHeader;                                       ///< 表头
};

DAPySeriesTableModulePrivate::DAPySeriesTableModulePrivate(DAPySeriesTableModule* p) : q_ptr(p)
{
}

/**
 * @brief 获取列数
 * @return
 */
int DAPySeriesTableModulePrivate::columnCount() const
{
    int col = 0;
    if (!mSeries.isEmpty()) {
        auto i = mSeries.end();
        --i;
        col = i.key();
    }
    if (!mAutoincrementSeries.isEmpty()) {
        int v  = 0;
        auto i = mAutoincrementSeries.end();
        --i;
        v = i.key();
        if (v > col) {
            col = v;
        }
    }
    if (mHeader.isEmpty()) {
        int v  = 0;
        auto i = mHeader.end();
        --i;
        v = i.key();
        if (v > col) {
            col = v;
        }
    }
    return col;
}

int DAPySeriesTableModulePrivate::rowCount() const
{
    int r = 0;
    for (auto i = mSeries.begin(); i != mSeries.end(); ++i) {
        int s = static_cast< int >(i.value().size());
        if (r < s) {
            r = s;
        }
    }
    if (r < 15) {
        return 15;
    }
    return r;
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
    if (role != Qt::DisplayRole || d_ptr->mSeries.empty()) {
        return QVariant();
    }
    //剩下都是DisplayRole
    if (Qt::Horizontal == orientation) {  //说明是水平表头
        auto ite = d_ptr->mHeader.find(section);
        if (ite != d_ptr->mHeader.end()) {
            return ite.value();
        }
        //如果没有，查看是否是series，series有名称，显示名称
        auto iteSer = d_ptr->mSeries.find(section);
        if (iteSer != d_ptr->mSeries.end()) {
            return iteSer.value().name();
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
    return d_ptr->columnCount();
}

int DAPySeriesTableModule::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return d_ptr->rowCount();
}

Qt::ItemFlags DAPySeriesTableModule::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant DAPySeriesTableModule::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || d_ptr->mSeries.empty()) {
        return QVariant();
    }
    if (role == Qt::TextAlignmentRole) {
        //返回的是对其方式
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::DisplayRole) {
        const int col = index.column();
        const int row = index.row();
        auto ite      = d_ptr->mSeries.find(col);
        if (ite != d_ptr->mSeries.end()) {
            //说明是序列列
            const DAPySeries& ser = ite.value();
            int ss                = static_cast< int >(ser.size());
            if (row < ss) {
                return ser[ row ];
            }
            return QVariant();
        }
        //没有在序列找到，找自增
        auto iteInc = d_ptr->mAutoincrementSeries.find(col);
        if (iteInc != d_ptr->mAutoincrementSeries.end()) {
            //说明是序列列
            const DAAutoincrementSeries< double >& serInc = iteInc.value();
            return serInc[ row ];
        }
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
    beginResetModel();
    int c               = d_ptr->columnCount();
    d_ptr->mSeries[ c ] = s;
    endResetModel();
}

/**
 * @brief 插入自增series
 * @param s
 */
void DAPySeriesTableModule::appendSeries(const DAAutoincrementSeries< double >& s)
{
    beginResetModel();
    int c                            = d_ptr->columnCount();
    d_ptr->mAutoincrementSeries[ c ] = s;
    endResetModel();
}

/**
 * @brief 插入series，index可以任意范围
 *
 * 例如[s0,s1],insertSeries(3,s2),结果是[s0,s1,[],s2]
 * 例如[],insertSeries(1,s1),结果是[[],s1]
 * @param index
 * @param s
 */
void DAPySeriesTableModule::insertSeries(int c, const DAPySeries& s)
{
    //先看看要刷新哪里
    beginResetModel();
    d_ptr->mSeries[ c ] = s;
    //看看这个位置是否有自增序列
    if (d_ptr->mAutoincrementSeries.contains(c)) {
        d_ptr->mAutoincrementSeries.remove(c);
    }
    endResetModel();
}

/**
 * @brief 插入DAAutoincrementSeries，index可以任意范围
 *
 * 例如[s0,s1],insertSeries(3,s2),结果是[s0,s1,[],s2]
 * 例如[],insertSeries(1,s1),结果是[[],s1]
 * @param index
 * @param s
 */
void DAPySeriesTableModule::insertSeries(int c, const DAAutoincrementSeries< double >& s)
{
    beginResetModel();
    d_ptr->mAutoincrementSeries[ c ] = s;
    //看看这个位置是否有series
    if (d_ptr->mSeries.contains(c)) {
        d_ptr->mSeries.remove(c);
    }
    endResetModel();
}

/**
 * @brief 把series设置到对应位置，如果有，则替换
 * @param c
 * @param s
 */
void DAPySeriesTableModule::setSeriesAt(int c, const DAPySeries& s)
{
    insertSeries(c, s);
}

/**
 * @brief 把DAAutoincrementSeries设置到对应位置，如果有，则替换
 * @param c
 * @param s
 */
void DAPySeriesTableModule::setSeriesAt(int c, const DAAutoincrementSeries< double >& s)
{
    insertSeries(c, s);
}

/**
 * @brief 设置表头
 * @param c
 * @param head
 */
void DAPySeriesTableModule::setColumnHeader(int c, const QString& head)
{
    d_ptr->mHeader[ c ] = head;
    emit headerDataChanged(Qt::Horizontal, c, c);
}

/**
 * @brief 获取header
 * @param c
 * @return 如果没有返回QString()
 */
QString DAPySeriesTableModule::getColumnHeader(int c) const
{
    return d_ptr->mHeader.value(c);
}

/**
 * @brief 清除
 */
void DAPySeriesTableModule::clearData()
{
    beginResetModel();
    d_ptr->mSeries.clear();
    d_ptr->mAutoincrementSeries.clear();
    endResetModel();
}

/**
 * @brief 设置表头，如果不设置，则返回的是series的名字作为表头
 * @param head
 */
void DAPySeriesTableModule::setHeaderLabel(const QStringList& head)
{
    for (int i = 0; i < head.size(); ++i) {
        d_ptr->mHeader[ i ] = head[ i ];
    }
    emit headerDataChanged(Qt::Horizontal, 0, head.size() - 1);
}

/**
 * @brief 获取当前维护的series
 * @return
 */
int DAPySeriesTableModule::getSeriesCount() const
{
    return d_ptr->mSeries.size();
}

/**
 * @brief 获取所有设置的表头，注意，如果跳着设置，表头返回的是连续的，不会因为中间没有设置而补充QString()
 * @return
 */
QList< QString > DAPySeriesTableModule::getSettingHeaderLabels() const
{
    return d_ptr->mHeader.values();
}

}
