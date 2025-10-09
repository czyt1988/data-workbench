#ifndef DAABSTRACTCACHEWINDOWTABLEMODEL_H
#define DAABSTRACTCACHEWINDOWTABLEMODEL_H
#include "DAGuiAPI.h"
#include <QAbstractTableModel>

namespace DA
{

/**
 * @brief 这是一个有缓存窗的模型，模型的显示行数固定在缓存窗的大小，这个模型适合超多行数据的显示
 *
 * @note DAAbstractCacheWindowTableModel不需要重写@sa rowCount @sa headerData @sa data 函数
 * 但你要重写@sa actualRowCount @sa actualHeaderData @sa actualData 函数
 */
class DAGUI_API DAAbstractCacheWindowTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	DAAbstractCacheWindowTableModel(QObject* parent = nullptr);
	~DAAbstractCacheWindowTableModel();
    // 滑动窗模式
    //  设置滑动窗模式的起始行
	virtual void setCacheWindowStartRow(int startRow);
	int getCacheWindowStartRow() const;
	// windows size决定了显示的行数
	void setCacheWindowSize(int s);
	int getCacheWindowSize() const;
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    // 滑动窗模式需要基础的接口

	virtual Qt::ItemFlags actualFlags(int actualRow, int actualColumn) const;
	// 获取真实的行数,相当于普通模型的rowCount
	virtual int actualRowCount() const = 0;
	//  获取真实的HeaderData,相当于普通模型的headerData
	virtual QVariant actualHeaderData(int actualSection, Qt::Orientation orientation, int role = Qt::DisplayRole) const = 0;
	//  获取真实的HeaderData,相当于普通模型的headerData
	virtual QVariant actualData(int actualRow, int actualColumn, int role = Qt::DisplayRole) const = 0;
	// 设置数据
	virtual bool setActualData(int actualRow, int actualColumn, const QVariant& value, int role = Qt::EditRole);

protected:
	int mCacheWindowSize { 2000 };  // 默认窗口大小
	int mWindowStartRow { 0 };      // 当前窗口起始行
};
}

#endif  // DAABSTRACTCACHEWINDOWTABLEMODEL_H
