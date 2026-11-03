#ifndef DADIALOGSTATSCHARTGUIDE_H
#define DADIALOGSTATSCHARTGUIDE_H
#include "DAGuiAPI.h"
#include "DAFigureAPI.h"
#include "DAData.h"
#include <QDialog>
#include <QJsonObject>

namespace Ui
{
class DADialogStatsChartGuide;
}

class QListWidgetItem;
namespace DA
{
class DADataManager;
class DAAbstractStatsChartAddWidget;
class DAFigureWidget;
class DAChartWidget;

/**
 * @brief 统计绘图引导对话框
 *
 * 左侧列表 + 右侧页面栈：列表内容与页面创建方式来自
 * DAChartAddItemWidgetFactory 注册表的 Stats 家族。
 *
 * 使用方式：
 * @code
 * DADialogStatsChartGuide dlg(parent);
 * dlg.setDataManager(dmgr);
 * dlg.setFigureWidget(fig);
 * dlg.setChartWidget(chart);
 * dlg.setCurrentChartType(DA::DAChartTypes::StatsHistplot);
 * dlg.show();
 * @endcode
 */
class DAGUI_API DADialogStatsChartGuide : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADialogStatsChartGuide)
public:
    explicit DADialogStatsChartGuide(QWidget* parent = nullptr);
    ~DADialogStatsChartGuide();

    // 设置 data manager，会传递给子 widget
    void setDataManager(DADataManager* dmgr);

    // 设置目标 Figure 窗口
    void setFigureWidget(DAFigureWidget* fig);

    // 设置目标 Chart 窗口
    void setChartWidget(DAChartWidget* chart);

    // 获取当前的统计绘图类型
    DA::DAChartTypes getCurrentChartType() const;

    // 设置当前的统计绘图类型（切换到对应页面）
    void setCurrentChartType(DA::DAChartTypes t);

    // 获取当前显示的统计绘图设置 widget
    DAAbstractStatsChartAddWidget* getCurrentStatsChartAddWidget() const;

    // 根据类型获取对应的统计绘图设置 widget（页面未创建时按需创建）
    DAAbstractStatsChartAddWidget* getStatsChartAddWidget(DA::DAChartTypes chartType);

Q_SIGNALS:
    /**
     * @brief 用户确认绘图参数后发射此信号
     * @param params 绘图参数 JSON
     * @param data 数据源，随信号一起传递
     * @param fig 目标 Figure 窗口
     * @param chart 目标 Chart 窗口
     */
    void plotRequested(const QJsonObject& params, const DA::DAData& data, DA::DAFigureWidget* fig, DA::DAChartWidget* chart);

private Q_SLOTS:
    void onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onAccepted();

private:
    void initListWidget();
    // 按注册表创建并缓存页面，未注册或不是统计类型时返回 nullptr
    QWidget* ensurePage(DA::DAChartTypes type);
    // 切换到指定类型的页面
    void selectPage(DA::DAChartTypes type);
    void ensureWidgetDataManager(DAAbstractStatsChartAddWidget* w);
    void propagateFigureChart(DAAbstractStatsChartAddWidget* w);

private:
    Ui::DADialogStatsChartGuide* ui;
    DADataManager* mDataMgr { nullptr };
    DAFigureWidget* mFigureWidget { nullptr };
    DAChartWidget* mChartWidget { nullptr };
};

}  // namespace DA

#endif  // DADIALOGSTATSCHARTGUIDE_H
