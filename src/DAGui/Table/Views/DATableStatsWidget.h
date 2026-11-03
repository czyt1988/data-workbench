#ifndef DATABLESTATSWIDGET_H
#define DATABLESTATSWIDGET_H
#include "DAGuiAPI.h"
#include <QPointer>
#include <QString>
#include <QWidget>

class QLabel;
class QTimer;

namespace DA
{
class DADataOperateOfDataFrameWidget;

/**
 * @brief 状态栏选区统计部件（007 T1.7）
 *
 * 绑定当前数据页 → 选区变化去抖 350ms → 按档位请求 computeColumnStats →
 * 状态栏一行文本 + 明细 tooltip。只读：不改数据、不入撤销栈、不写工程、
 * 不改选区；显示档位由返回值（std::optional 有无）决定，C++ 不判类型。
 */
class DAGUI_API DATableStatsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DATableStatsWidget(QWidget* parent = nullptr);
    ~DATableStatsWidget();

    // 绑定数据页（nullptr = 解绑并隐藏）；同页重复调用只做立即刷新
    void setPage(DADataOperateOfDataFrameWidget* page);
    DADataOperateOfDataFrameWidget* page() const;
    // 立即刷新（去抖到期、页面切换、测试）
    void refreshNow();
    // 状态栏那一行的完整文本（未省略）
    QString statsText() const;
    // 明细 tooltip 文本（供单测断言）
    QString detailsTooltipText() const;

protected:
    void resizeEvent(QResizeEvent* e) override;

private Q_SLOTS:
    void onSelectionChanged();

private:
    void rebuildDisplay();

private:
    QPointer< DADataOperateOfDataFrameWidget > mPage;
    QTimer* mDebounceTimer { nullptr };
    QLabel* mTextLabel { nullptr };
    QString m_statsText;        ///< 未省略完整文本
    QString m_detailsTooltip;  ///< 明细 tooltip（富文本）
};
}  // end of namespace DA
#endif  // DATABLESTATSWIDGET_H
