#ifndef DAABSTRACTCHARTADDWIDGETBASE_H
#define DAABSTRACTCHARTADDWIDGETBASE_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAData.h"
namespace DA
{
class DADataManager;

/**
 * @brief 图表添加页的公共基类
 *
 * 统一持有数据管理器，并提供「数据管理器改变」「当前数据改变」两个信号。
 * 二维（DAAbstractChartAddItemWidget）、三维（DAAbstractChart3DAddItemWidget）
 * 与统计（DAAbstractStatsChartAddWidget）三个家族都从它派生，
 * 避免每个家族各自重复实现数据管理器成员与信号。
 */
class DAGUI_API DAAbstractChartAddWidgetBase : public QWidget
{
    Q_OBJECT
public:
    DAAbstractChartAddWidgetBase(QWidget* par = nullptr);
    ~DAAbstractChartAddWidgetBase();
    // 设置数据管理器，值改变时发出 dataManagerChanged
    virtual void setDataManager(DADataManager* dmgr);
    // 获取数据管理器
    DADataManager* getDataManager() const;

Q_SIGNALS:
    // 数据管理器发生改变
    void dataManagerChanged(DADataManager* dmgr);
    // 当前数据发生改变
    void currentDataChanged(const DAData& d);

private:
    DADataManager* mDataManager { nullptr };
};
}  // end DA

#endif  // DAABSTRACTCHARTADDWIDGETBASE_H
