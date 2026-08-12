#ifndef DACHARTSERIESSELECTWIDGET_H
#define DACHARTSERIESSELECTWIDGET_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include <QWidget>
#include "DAPySeriesListView.h"

class QToolButton;

namespace Ui
{
class DAChartSeriesSelectWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesPickerWidget;

/**
 * @brief 数据序列选择复合组件
 *
 * 由 DAPySeriesListView + 添加/移除两个 QToolButton 组成，点击"添加"按钮弹出
 * DAChartSeriesPickerWidget 浮动窗口供用户拾取序列，封装 DAChartAddXYSeriesWidget 中
 * 的 X/Y 选择组合逻辑，供所有"新增绘图"窗口复用。
 *
 * 典型用法：
 * @code
 * auto* w = new DAChartSeriesSelectWidget("Open", parent);
 * w->setAcceptMode(DAPySeriesListView::AcceptOneSeries);
 * w->setDataManager(mgr);
 * connect(w, &DAChartSeriesSelectWidget::seriesChanged, ...);
 * // 读取：
 * QPair<DAData, QString> s = w->getCurrentSeries();
 * @endcode
 *
 * @note 本组件不含"自增序列"功能，需要自增的调用方应在组件外部管理自增 groupbox，
 *       并在自增启用时调用 @ref setEnabled(false) 禁用本组件。
 */
class DAGUI_API DAChartSeriesSelectWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造
     * @param parent 父窗口
     *
     * @note 此构造签名符合 Qt promoted widget 惯例（仅 QWidget*），便于在 .ui 文件中
     *       通过 promoted widget 引入。角色标签默认为空，需通过 @ref setRoleLabel 设置。
     *       如需编程式构造并直接指定 roleLabel，使用 @ref setRoleLabel 在构造后调用。
     */
    explicit DAChartSeriesSelectWidget(QWidget* parent = nullptr);
    ~DAChartSeriesSelectWidget();

    // 设置序列接受模式，默认 AcceptOneSeries
    void setAcceptMode(DAPySeriesListView::AcceptMode mode);
    DAPySeriesListView::AcceptMode getAcceptMode() const;
    // 设置数据管理器
    void setDataManager(DADataManager* mgr);
    DADataManager* getDataManager() const;
    // 设置角色标签文本
    void setRoleLabel(const QString& label);
    QString getRoleLabel() const;

    // 单序列场景：获取当前选中的第一个序列（data + seriesName）
    QPair< DAData, QString > getCurrentSeries() const;
    // 多序列场景：获取所有选中的序列
    QList< QPair< DAData, QStringList > > getSeries() const;
    // 编程式添加序列
    void addSeries(const DAData& dataframeData, const QString& name);
    // 移除 listView 当前选中项
    void removeCurrentSelect();

Q_SIGNALS:
    // 序列变化信号（添加/移除时触发）
    void seriesChanged();

private Q_SLOTS:
    void onAddClicked();
    void onRemoveClicked();

private:
    void ensurePicker();
    void hideParentGuideDialog();
    void showParentGuideDialog();

private:
    Ui::DAChartSeriesSelectWidget* ui;
    DAChartSeriesPickerWidget* mPicker { nullptr };
    DADataManager* mDataMgr { nullptr };
    QString mRoleLabel;
};
}  // namespace DA
#endif  // DACHARTSERIESSELECTWIDGET_H
