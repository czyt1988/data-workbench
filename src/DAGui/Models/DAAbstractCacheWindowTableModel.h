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
 *
 * @note 行号口径（重要）：模型对外的窗口接口（rowCount/data/headerData）用的是
 * 「窗口内逻辑行」，而 actual* 接口与所有 notify* 通知用的是「数据源绝对行」。
 * 数据操作（插删行列、粘贴、定位、样式键）一律使用绝对行，逻辑行只允许出现在
 * 视图内部，两者的换算只经 getCacheWindowStartRow() 一处。
 */
class DAGUI_API DAAbstractCacheWindowTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DAAbstractCacheWindowTableModel(QObject* parent = nullptr);
    ~DAAbstractCacheWindowTableModel();
    // 滑动窗模式
    //  设置滑动窗模式的起始行，自动夹取到[0, maxCacheWindowStartRow()]
    virtual void setCacheWindowStartRow(int startRow);
    int getCacheWindowStartRow() const;
    // 窗口起始行的合法上界：超过它，窗口尾部就会越过数据末尾
    virtual int maxCacheWindowStartRow() const;
    // 窗口尾部用于新增数据的额外空行数量，参与maxCacheWindowStartRow的计算
    virtual int getExtraRowCount() const;
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
    // 以下 notify* 的行列参数一律为「数据源绝对行/列」，与 actualData/setActualData 同口径
    void notifyRowChanged(int row);
    void notifyColumnChanged(int col);
    // virtual：派生模型可能持有cell级缓存（如DADataTableModel的块缓存），
    // 需在此类通知中先行失效；其余notify*已经过虚函数cacheShape()给派生类失效机会
    virtual void notifyDataChanged(int row, int col);
    virtual void notifyDataChanged(int rowStart, int colStart, int rowEnd, int colEnd);
    // 行将移除
    void notifyRowsRemoved(const QList< int >& r);
    // 行将插入
    void notifyRowsInserted(const QList< int >& r);
    // 行将移除
    void notifyColumnsRemoved(const QList< int >& c);
    // 行将插入
    void notifyColumnsInserted(const QList< int >& c);

Q_SIGNALS:
    // 窗口起始行发生变化（视图据此同步滚动条与位置状态）
    void cacheWindowChanged(int oldStartRow, int newStartRow);
    // 结构变化（插删行列）后发出：行列数可能已变，消费方需重新确认窗口与滚动范围
    void tableStructureChanged();

protected:
    virtual void cacheShape();
    // 整窗刷新：结构变化（插删行列）后窗口内映射整体位移，按整窗通知
    void refreshWindow();

protected:
    int mCacheWindowSize { 20000 };  // 默认窗口大小，Qt的MVC性能足够，这里主要是避免大表会触发全行遍历表头求表头宽度导致的性能问题
    int mWindowStartRow { 0 };       // 当前窗口起始行
};
}

#endif  // DAABSTRACTCACHEWINDOWTABLEMODEL_H
