#ifndef DADATATABLEMODEL_H
#define DADATATABLEMODEL_H
#include "DAGuiAPI.h"
#include <QAbstractTableModel>
#include "DAAbstractCacheWindowTableModel.h"
#include "DAData.h"

class QUndoStack;
namespace DA
{
/**
 * @brief 针对DAData的model
 *
 * DAData会共享数据，在外部修改任意一个DAData都会改变所有共享对象的内容
 *
 * 如果不希望共享，可以使用DAPyDataFrameTableModel
 */
class DADataTableModel : public DAAbstractCacheWindowTableModel
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADataTableModel)
public:
    DADataTableModel(QUndoStack* stack, QObject* parent = nullptr);
    ~DADataTableModel();

public:
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QVariant actualHeaderData(int actualSection, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual int actualRowCount() const override;
    virtual QVariant actualData(int actualRow, int actualColumn, int role = Qt::DisplayRole) const override;
    // 设置数据
    virtual bool setActualData(int actualRow, int actualColumn, const QVariant& value, int role = Qt::EditRole) override;

public:
    // 设置数据
    void setData(const DAData& data);
    DAData getData() const;
    // 设置使用缓存模式，缓存模式不会频繁调用dataframe，在setdataframe时把常用的参数缓存
    void setUseCacheMode(bool on = true);
    // 设置滑动窗模式的起始行
    virtual void setCacheWindowStartRow(int startRow) override;
    // 刷新
    void refreshData();
    // 超出模型实际数据行数的额外空行数量
    void setExtraRowCount(int v);
    int getExtraRowCount() const;
    // 超出模型实际数据列数的额外空列数量
    void setExtraColumnCount(int v);
    int getExtraColumnCount() const;
    // 最小显示的行数量
    void setMinShowRowCount(int v);
    int getMinShowRowCount() const;
    // 最小显示的列数量
    void setMinShowColumnCount(int v);
    int getMinShowColumnCount() const;

protected:
    // 缓存
    void cacheShape() override;
};
}
#endif  // DADATATABLEMODEL_H
