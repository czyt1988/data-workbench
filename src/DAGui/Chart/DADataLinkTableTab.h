#ifndef DADATALINKTABLETAB_H
#define DADATALINKTABLETAB_H
#include <QWidget>
#include <QPointer>
#include <QList>
#include "DAGuiAPI.h"

class QTreeView;
class QMenu;
class QAction;
class QwtPlotItem;
namespace DA
{
class DADataLinkTableModel;
class DADataLinkTableDelegate;
class DAFigureWidget;
class DADataProbeMarker;
}

/**
 * @brief 数据联动表的单个 tab（对应一个 DAFigureWidget）
 *
 * 纯视图架构：表格内容完全从 figure 上的 DADataProbeMarker 探针图元自动推导，
 * 不持有独立的持久化状态——
 * - 列 = 一组同名探针（探针名即列头，探针值即 x 值）
 * - 探针经 QwtPlot::itemAttached 信号增删（探针模式创建/undo/redo/指针工具删除）时列自动增删
 * - 工程加载时探针由图表序列化恢复，本表随之自动重建
 */
namespace DA
{
class DAGUI_API DADataLinkTableTab : public QWidget
{
    Q_OBJECT
public:
    explicit DADataLinkTableTab(DA::DAFigureWidget* figWidget, QWidget* parent = nullptr);
    ~DADataLinkTableTab() override;

    DA::DAFigureWidget* figureWidget() const;

    // 从探针图元全量重建表格（tab 创建、工程加载完成、探针外部变更等场景）
    void rebuildFromProbes();

public Q_SLOTS:
    // 探针模式下创建了一组探针（DAFigureWidget::dataProbeCreated）
    void onProbeCreated(DA::DADataProbeMarker* probe);

private slots:
    void onHeaderContextMenuRequested(const QPoint& pos);
    void onRenameColumn();
    void onHideColumn();
    void onDeleteColumn();
    void onRefreshColumn();
    void onRefreshAllColumns();
    void onHideAllColumns();
    void onShowAllColumns();
    void onDeleteAllColumns();
    void onItemAttached(QwtPlotItem* item, bool on);

private:
    void initRows();
    void connectPlots();
    void disconnectPlots();
    void syncRowsVisibility();
    // 一组同名探针：同一探针名在 figure 各子图上的所有实例
    struct ProbeGroup
    {
        QString name;                            ///< 探针名（列头）
        double xValue { 0.0 };                   ///< 探针 x 值
        QList< DA::DADataProbeMarker* > probes;  ///< 跨子图的探针实例
    };
    // 扫描 figure 全部探针，按名称分组（列顺序按 x 值排序）
    QList< ProbeGroup > scanProbeGroups() const;
    // 按探针名定位已有列（不存在返回 -1）
    int columnOfProbeGroup(const QString& name) const;
    // 用探针组的捕获数据回填列
    void fillColumn(int col, const ProbeGroup& group);
    QString formatXValueForHeader(double xValue) const;
    // 删除一组探针（走 undo 栈）
    void removeProbeGroup(const ProbeGroup& group);

private:
    QPointer< DA::DAFigureWidget > m_figureWidget;
    QTreeView* m_treeView { nullptr };
    DA::DADataLinkTableModel* m_model { nullptr };
    DA::DADataLinkTableDelegate* m_delegate { nullptr };
    QMenu* m_headerMenu { nullptr };
    QAction* m_actRename { nullptr };
    QAction* m_actHide { nullptr };
    QMenu* m_subMenuShow { nullptr };
    QAction* m_actDelete { nullptr };
    QAction* m_actRefresh { nullptr };
    QAction* m_actRefreshAll { nullptr };
    QAction* m_actHideAll { nullptr };
    QAction* m_actShowAll { nullptr };
    QAction* m_actDeleteAll { nullptr };
    int m_contextMenuColumn { -1 };
    bool m_rebuilding { false };  // 重建过程抑制 itemAttached 引发的递归刷新
};
}  // namespace DA

#endif  // DADATALINKTABLETAB_H
