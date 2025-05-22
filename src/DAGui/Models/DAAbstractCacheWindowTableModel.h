#ifndef DAABSTRACTCACHEWINDOWTABLEMODEL_H
#define DAABSTRACTCACHEWINDOWTABLEMODEL_H
#include "DAGuiAPI.h"
#include <QAbstractTableModel>

namespace DA
{

/**
 * @brief 这是一个有缓存窗的模型，模型的显示行数固定在缓存窗的大小，这个模型适合超多行数据的显示
 */
class DAGUI_API DAAbstractCacheWindowTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DAAbstractCacheWindowTableModel(QObject* parent = nullptr);
    ~DAAbstractCacheWindowTableModel();
    /// @group 滑动窗模式
    /// @{
    // 设置滑动窗模式的起始行
    virtual void setCacheWindowStartRow(int startRow);
    int getCacheWindowStartRow() const;
    // windows size决定了显示的行数
    void setCacheWindowSize(int s);
    int getCacheWindowSize() const;
    // 获取真实的行数
    virtual int getActualRowCount() const = 0;
    /// @}
protected:
    int mCacheWindowSize { 2000 };  // 默认窗口大小
    int mWindowStartRow { 0 };      // 当前窗口起始行
};
}

#endif  // DAABSTRACTCACHEWINDOWTABLEMODEL_H
