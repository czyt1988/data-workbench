#ifndef DAPYSERIESTABLEMODEL_H
#define DAPYSERIESTABLEMODEL_H
#include "DAGuiAPI.h"
#include "DAAbstractCacheWindowTableModel.h"
#include <QAbstractTableModel>
#include "DAData.h"
#include "DAAutoincrementSeries.hpp"
namespace DA
{

/**
 * @brief 用于显示一系列series
 */
class DAGUI_API DAPySeriesTableModel : public DAAbstractCacheWindowTableModel
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPySeriesTableModel)
public:
	DAPySeriesTableModel(QObject* parent = nullptr);
	~DAPySeriesTableModel();

public:
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual Qt::ItemFlags actualFlags(int actualRow, int actualColumn) const override;
	virtual QVariant actualHeaderData(int actualSection, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	virtual int actualRowCount() const override;
	virtual QVariant actualData(int actualRow, int actualColumn, int role = Qt::DisplayRole) const override;

public:
	void setSeries(const QList< DAPySeries >& series);
	QList< DAPySeries > getSeries() const;
	// 设置滑动窗模式的起始行
	virtual void setCacheWindowStartRow(int startRow) override;
	// 追加series
	void appendSeries(const DAPySeries& s);
	void appendSeries(const DAAutoincrementSeries< double >& s);
	// 插入series，index如果超出范围，会append，例如[s0,s1],insertSeries(3,s2),结果是[s0,s1,s2]
	void insertSeries(int c, const DAPySeries& s);
	void insertSeries(int c, const DAAutoincrementSeries< double >& s);
	// 把series设置到对应位置，如果有，则替换
	void setSeriesAt(int c, const DAPySeries& s);
	void setSeriesAt(int c, const DAAutoincrementSeries< double >& s);
	// 设置表头的名称
	void setColumnHeader(int c, const QString& head);
	QString getColumnHeader(int c) const;
	// 清除
	void clearData();
	// 设置表头，如果不设置，则返回的是series的名字作为表头
	void setHeaderLabel(const QStringList& head);
	// 获取当前维护的series
	int getSeriesCount() const;
	// 获取所有表头
	QList< QString > getSettingHeaderLabels() const;
	//
};
}

#endif  // DAPYSERIESTABLEMODEL_H
