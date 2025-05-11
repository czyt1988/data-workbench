#include "DAPySeriesTableModule.h"
#include "pandas/DAPySeries.h"

namespace DA
{

class DAPySeriesTableModule::PrivateData
{
	DA_DECLARE_PUBLIC(DAPySeriesTableModule)
public:
	PrivateData(DAPySeriesTableModule* p);

	int columnCount() const;

	int rowCount() const;

public:
	//! 这里都用map来管理，因为不同列有可能是不同属性，目前还没有做一个抽象能囊括所有序列，因此还是需要每种类型添加一个索引
	//! 注意，这里要用qmap，不能用qhash，因为取最大索引是通过qmap的自动排序实现的
	QMap< int, DAPySeries > seriesMap;                                    ///< 序列
	QMap< int, DAAutoincrementSeries< double > > autoincrementSeriesMap;  ///< 自增序列
	QMap< int, QString > headerLabelMap;                                  ///< 表头
};

DAPySeriesTableModule::PrivateData::PrivateData(DAPySeriesTableModule* p) : q_ptr(p)
{
}

/**
 * @brief 获取列数
 * @return
 */
int DAPySeriesTableModule::PrivateData::columnCount() const
{
	int col = -1;  // 这样在没有数据的时候可以返回0
	if (!seriesMap.isEmpty()) {
		col = seriesMap.lastKey();
	}
	if (!autoincrementSeriesMap.isEmpty()) {
		int v = autoincrementSeriesMap.lastKey();
		if (v > col) {
			col = v;
		}
	}
	if (!headerLabelMap.isEmpty()) {
		int v = headerLabelMap.lastKey();
		if (v > col) {
			col = v;
		}
	}
	return col + 1;
}

int DAPySeriesTableModule::PrivateData::rowCount() const
{
	int r = 0;
	for (auto i = seriesMap.begin(); i != seriesMap.end(); ++i) {
		const DAPySeries& ser = i.value();
		if (!ser.isNone()) {
			int s = static_cast< int >(ser.size());
			if (r < s) {
				r = s;
			}
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
DAPySeriesTableModule::DAPySeriesTableModule(QObject* parent) : QAbstractTableModel(parent), DA_PIMPL_CONSTRUCT
{
}

DAPySeriesTableModule::~DAPySeriesTableModule()
{
}

QVariant DAPySeriesTableModule::headerData(int section, Qt::Orientation orientation, int role) const
{
	DA_DC(d);
	if (role != Qt::DisplayRole || d->seriesMap.empty()) {
		return QVariant();
	}
	// 剩下都是DisplayRole
	if (Qt::Horizontal == orientation) {  // 说明是水平表头
		auto ite = d->headerLabelMap.find(section);
		if (ite != d->headerLabelMap.end()) {
			return ite.value();
		}
		// 如果没有，查看是否是series，series有名称，显示名称
		auto iteSer = d->seriesMap.find(section);
		if (iteSer != d->seriesMap.end()) {
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
	DA_DC(d);
	if (!index.isValid() || d->seriesMap.empty()) {
		return QVariant();
	}
	if (role == Qt::TextAlignmentRole) {
		// 返回的是对其方式
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	} else if (role == Qt::DisplayRole) {
		const int col = index.column();
		const int row = index.row();
		auto ite      = d->seriesMap.find(col);
		if (ite != d->seriesMap.end()) {
			// 说明是序列列
			try {
				const DAPySeries& ser = ite.value();
				if (ser.isNone()) {
					return QVariant();
				}
				int ss = static_cast< int >(ser.size());
				if (row < ss) {
					return ser[ row ];
				}
				return QVariant();
			} catch (const std::exception& e) {
				qCritical() << e.what();
				return QVariant();
			}
		}
		// 没有在序列找到，找自增
		auto iteInc = d->autoincrementSeriesMap.find(col);
		if (iteInc != d->autoincrementSeriesMap.end()) {
			// 说明是序列列
			const DAAutoincrementSeries< double >& serInc = iteInc.value();
			return serInc[ row ];
		}
	} else if (role == Qt::BackgroundRole) {
		// 背景颜色
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
 * @brief 设置序列
 * @param s
 */
void DAPySeriesTableModule::setSeries(const QList< DAPySeries >& series)
{
	beginResetModel();
	d_ptr->seriesMap.clear();
	int c = 0;
	for (const DAPySeries& s : series) {
		d_ptr->seriesMap[ c ] = s;
		++c;
	}
	endResetModel();
}

/**
 * @brief 获取series
 *
 */
QList< DAPySeries > DAPySeriesTableModule::getSeries() const
{
    return d_ptr->seriesMap.values();
}

/**
 * @brief 插入series
 * @param s
 */
void DAPySeriesTableModule::appendSeries(const DAPySeries& s)
{
	beginResetModel();
	int c                 = d_ptr->columnCount();
	d_ptr->seriesMap[ c ] = s;
	endResetModel();
}

/**
 * @brief 插入自增series
 * @param s
 */
void DAPySeriesTableModule::appendSeries(const DAAutoincrementSeries< double >& s)
{
	beginResetModel();
	int c                              = d_ptr->columnCount();
	d_ptr->autoincrementSeriesMap[ c ] = s;
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
	// 先看看要刷新哪里
	beginResetModel();
	d_ptr->seriesMap[ c ] = s;
	// 看看这个位置是否有自增序列
	if (d_ptr->autoincrementSeriesMap.contains(c)) {
		d_ptr->autoincrementSeriesMap.remove(c);
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
	d_ptr->autoincrementSeriesMap[ c ] = s;
	// 看看这个位置是否有series
	if (d_ptr->seriesMap.contains(c)) {
		d_ptr->seriesMap.remove(c);
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
	d_ptr->headerLabelMap[ c ] = head;
	emit headerDataChanged(Qt::Horizontal, c, c);
}

/**
 * @brief 获取header
 * @param c
 * @return 如果没有返回QString()
 */
QString DAPySeriesTableModule::getColumnHeader(int c) const
{
    return d_ptr->headerLabelMap.value(c);
}

/**
 * @brief 清除
 */
void DAPySeriesTableModule::clearData()
{
	beginResetModel();
	d_ptr->seriesMap.clear();
	d_ptr->autoincrementSeriesMap.clear();
	endResetModel();
}

/**
 * @brief 设置表头，如果不设置，则返回的是series的名字作为表头
 * @param head
 */
void DAPySeriesTableModule::setHeaderLabel(const QStringList& head)
{
	for (int i = 0; i < head.size(); ++i) {
		d_ptr->headerLabelMap[ i ] = head[ i ];
	}
	emit headerDataChanged(Qt::Horizontal, 0, head.size() - 1);
}

/**
 * @brief 获取当前维护的series
 * @return
 */
int DAPySeriesTableModule::getSeriesCount() const
{
    return d_ptr->seriesMap.size();
}

/**
 * @brief 获取所有设置的表头，注意，如果跳着设置，表头返回的是连续的，不会因为中间没有设置而补充QString()
 * @return
 */
QList< QString > DAPySeriesTableModule::getSettingHeaderLabels() const
{
    return d_ptr->headerLabelMap.values();
}
}
