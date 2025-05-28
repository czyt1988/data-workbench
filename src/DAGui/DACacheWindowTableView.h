#ifndef DACACHEWINDOWTABLEVIEW_H
#define DACACHEWINDOWTABLEVIEW_H
#include "DAGuiAPI.h"
#include <QTableView>
namespace DA
{
class DAAbstractCacheWindowTableModel;
/**
 * @brief 针对DAAbstractCacheWindowTableModel的view
 */
class DAGUI_API DACacheWindowTableView : public QTableView
{
    Q_OBJECT
public:
    DACacheWindowTableView(QWidget* parent = nullptr);
    ~DACacheWindowTableView();
    // 获取缓存窗口model
    DAAbstractCacheWindowTableModel* getCacheModel() const;
    // 显示行
    void showActualRow(int actualRow);
private Q_SLOTS:
    virtual void verticalScrollBarValueChanged(int v);
};
}

#endif  // DACACHEWINDOWTABLEVIEW_H
