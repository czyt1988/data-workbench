#include "DADataLinkTableTab.h"
#include "DADataLinkTableModel.h"
#include "DADataLinkTableDelegate.h"
// Qt
#include <QTreeView>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSet>
#include <QTimer>
// DA
#include "DAFigureWidget.h"
#include "DADataProbeMarker.h"
#include "DAChartWidget.h"
// qwt
#include "qwt_plot.h"
#include "qwt_axis.h"
#include "qwt_plot_item.h"
#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_styling.h"
#include "qwt_plot_item_info.h"
#include "qwt_figure.h"
#include "qwt_scale_draw.h"
#include "qwt_text.h"

namespace DA
{

DADataLinkTableTab::DADataLinkTableTab(DA::DAFigureWidget* figWidget, QWidget* parent)
    : QWidget(parent), m_figureWidget(figWidget)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_treeView = new QTreeView(this);
    m_treeView->setHeaderHidden(false);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setAlternatingRowColors(true);

    m_model    = new DA::DADataLinkTableModel(this);
    m_delegate = new DA::DADataLinkTableDelegate(this);
    m_treeView->setModel(m_model);
    m_treeView->setItemDelegate(m_delegate);

    m_treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView->header(), &QHeaderView::customContextMenuRequested,
            this, &DADataLinkTableTab::onHeaderContextMenuRequested);

    connect(m_model, &QAbstractItemModel::rowsInserted, this, [this]() {
        m_treeView->expandAll();
    });

    layout->addWidget(m_treeView);

    m_headerMenu    = new QMenu(this);
    m_actRename     = m_headerMenu->addAction(tr("Rename"));  // cn:重命名
    m_actRename->setIcon(QIcon(":/DAGui/icon/rename-column.svg"));
    m_headerMenu->addSeparator();
    m_actRefresh    = m_headerMenu->addAction(tr("Refresh"));  // cn:刷新
    m_actRefresh->setIcon(QIcon(":/DAGui/icon/refresh.svg"));
    m_actRefreshAll = m_headerMenu->addAction(tr("Refresh All"));  // cn:刷新全部
    m_actRefreshAll->setIcon(QIcon(":/DAGui/icon/refresh-all.svg"));
    m_headerMenu->addSeparator();
    m_actHide       = m_headerMenu->addAction(tr("Hide"));  // cn:隐藏
    m_actHide->setIcon(QIcon(":/DAGui/icon/hide-column.svg"));
    m_subMenuShow   = new QMenu(tr("Show"), m_headerMenu);  // cn:显示
    m_subMenuShow->menuAction()->setIcon(QIcon(":/DAGui/icon/show-column.svg"));
    m_headerMenu->addMenu(m_subMenuShow);
    m_actHideAll    = m_headerMenu->addAction(tr("Hide All"));  // cn:全部隐藏
    m_actHideAll->setIcon(QIcon(":/DAGui/icon/hide-all-columns.svg"));
    m_actShowAll    = m_headerMenu->addAction(tr("Show All"));  // cn:全部显示
    m_actShowAll->setIcon(QIcon(":/DAGui/icon/show-all-columns.svg"));
    m_headerMenu->addSeparator();
    m_actDelete     = m_headerMenu->addAction(tr("Delete"));  // cn:删除
    m_actDelete->setIcon(QIcon(":/DAGui/icon/delete-column.svg"));
    m_actDeleteAll  = m_headerMenu->addAction(tr("Delete All"));  // cn:全部删除
    m_actDeleteAll->setIcon(QIcon(":/DAGui/icon/delete-all-columns.svg"));
    connect(m_actRename, &QAction::triggered, this, &DADataLinkTableTab::onRenameColumn);
    connect(m_actHide, &QAction::triggered, this, &DADataLinkTableTab::onHideColumn);
    connect(m_actRefresh, &QAction::triggered, this, &DADataLinkTableTab::onRefreshColumn);
    connect(m_actRefreshAll, &QAction::triggered, this, &DADataLinkTableTab::onRefreshAllColumns);
    connect(m_actHideAll, &QAction::triggered, this, &DADataLinkTableTab::onHideAllColumns);
    connect(m_actShowAll, &QAction::triggered, this, &DADataLinkTableTab::onShowAllColumns);
    connect(m_actDelete, &QAction::triggered, this, &DADataLinkTableTab::onDeleteColumn);
    connect(m_actDeleteAll, &QAction::triggered, this, &DADataLinkTableTab::onDeleteAllColumns);

    connectPlots();
    // 初次构建：扫描探针 + 曲线行
    rebuildFromProbes();
    initRows();
}

DADataLinkTableTab::~DADataLinkTableTab()
{
    disconnectPlots();
}

DA::DAFigureWidget* DADataLinkTableTab::figureWidget() const
{
    return m_figureWidget;
}

void DADataLinkTableTab::connectPlots()
{
    if (!m_figureWidget) {
        return;
    }
    QwtFigure* figure = m_figureWidget->figure();
    if (!figure) {
        return;
    }
    const QList< QwtPlot* > plots = figure->allAxes();
    for (QwtPlot* plot : plots) {
        connect(plot, &QwtPlot::itemAttached, this, &DADataLinkTableTab::onItemAttached, Qt::UniqueConnection);
    }
    // 新子图加入时也连接
    connect(figure, &QwtFigure::axesAdded, this, [this](QwtPlot* plot) {
        if (plot) {
            connect(plot, &QwtPlot::itemAttached, this, &DADataLinkTableTab::onItemAttached, Qt::UniqueConnection);
        }
    });
}

void DADataLinkTableTab::disconnectPlots()
{
    if (!m_figureWidget) {
        return;
    }
    QwtFigure* figure = m_figureWidget->figure();
    if (figure) {
        disconnect(figure, &QwtFigure::axesAdded, this, nullptr);
        const QList< QwtPlot* > plots = figure->allAxes();
        for (QwtPlot* plot : plots) {
            disconnect(plot, &QwtPlot::itemAttached, this, &DADataLinkTableTab::onItemAttached);
        }
    }
}

void DADataLinkTableTab::initRows()
{
    if (!m_figureWidget) {
        return;
    }
    QwtFigure* figure = m_figureWidget->figure();
    if (!figure) {
        return;
    }
    const QList< QwtPlot* > plots = figure->allAxes();
    for (QwtPlot* plot : plots) {
        QString plotTitle = plot->title().text();
        if (plotTitle.isEmpty()) {
            plotTitle = tr("Chart %1").arg(plot->plotId());  // cn:绘图 %1
        }
        QStandardItem* plotRow = m_model->ensurePlotRow(plot, plotTitle);

        QwtPlotItemList items = QwtPlotItemInfo::seriesItems(plot);
        for (QwtPlotItem* item : items) {
            QString curveTitle = item->title().text();
            if (curveTitle.isEmpty()) {
                curveTitle = tr("Curve");  // cn:曲线
            }
            QColor color = QwtPlotStyling::color(item);
            m_model->ensureCurveRow(plotRow, item, curveTitle, color);
        }
    }
    m_model->updateVisibility();
}

QList< DADataLinkTableTab::ProbeGroup > DADataLinkTableTab::scanProbeGroups() const
{
    QList< ProbeGroup > groups;
    if (!m_figureWidget) {
        return groups;
    }
    QwtFigure* figure = m_figureWidget->figure();
    if (!figure) {
        return groups;
    }
    const QList< QwtPlot* > plots = figure->allAxes();
    for (QwtPlot* plot : plots) {
        const QwtPlotItemList& items = plot->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() != DA::DADataProbeMarker::Rtti_DataProbeMarker) {
                continue;
            }
            auto* probe = static_cast< DA::DADataProbeMarker* >(item);
            const QString name = probe->probeName().text();
            if (name.isEmpty()) {
                continue;
            }
            bool found = false;
            for (ProbeGroup& g : groups) {
                if (g.name == name) {
                    g.probes.append(probe);
                    found = true;
                    break;
                }
            }
            if (!found) {
                ProbeGroup g;
                g.name  = name;
                g.xValue = probe->probeValue();
                g.probes.append(probe);
                groups.append(g);
            }
        }
    }
    // 按 x 值排序，保证列顺序稳定（创建顺序即 x 递增的常见情形下无变化）
    std::sort(groups.begin(), groups.end(),
              [](const ProbeGroup& a, const ProbeGroup& b) { return a.xValue < b.xValue; });
    return groups;
}

int DADataLinkTableTab::columnOfProbeGroup(const QString& name) const
{
    const int colCount = m_model->columnCount();
    for (int c = 1; c < colCount; ++c) {
        if (auto* header = m_model->horizontalHeaderItem(c)) {
            if (header->text() == name) {
                return c;
            }
        }
    }
    return -1;
}

void DADataLinkTableTab::rebuildFromProbes()
{
    if (!m_figureWidget) {
        return;
    }
    m_rebuilding = true;
    // 清掉旧列（保留第 0 列曲线列）
    for (int col = m_model->columnCount() - 1; col >= 1; --col) {
        m_model->removeProbeColumn(col);
    }
    const QList< ProbeGroup > groups = scanProbeGroups();
    for (const ProbeGroup& g : groups) {
        int col = m_model->addProbeColumn(g.xValue, formatXValueForHeader(g.xValue), g.name);
        fillColumn(col, g);
    }
    m_model->updateVisibility();
    m_rebuilding = false;
}

void DADataLinkTableTab::fillColumn(int col, const ProbeGroup& group)
{
    if (!m_figureWidget) {
        return;
    }
    QwtFigure* figure = m_figureWidget->figure();
    if (!figure) {
        return;
    }
    const QList< QwtPlot* > plots = figure->allAxes();
    for (QwtPlot* plot : plots) {
        QString plotTitle = plot->title().text();
        if (plotTitle.isEmpty()) {
            plotTitle = tr("Chart %1").arg(plot->plotId());  // cn:绘图 %1
        }
        QStandardItem* plotRow = m_model->ensurePlotRow(plot, plotTitle);
        // 先清空该列已有数据
        QwtPlotItemList items = QwtPlotItemInfo::seriesItems(plot);
        for (QwtPlotItem* item : items) {
            QString curveTitle = item->title().text();
            if (curveTitle.isEmpty()) {
                curveTitle = tr("Curve");  // cn:曲线
            }
            QColor color = QwtPlotStyling::color(item);
            QStandardItem* curveRow = m_model->ensureCurveRow(plotRow, item, curveTitle, color);
            m_model->setCellData(curveRow, col, QString());
        }
        // 找属于该列的探针（本子图上的实例）
        DA::DADataProbeMarker* probe = nullptr;
        for (DA::DADataProbeMarker* p : group.probes) {
            if (p->plot() == plot) {
                probe = p;
                break;
            }
        }
        if (!probe) {
            continue;
        }
        probe->captureData(true);
        const QList< DA::DADataProbeMarker::CapturedData > cds = probe->capturedData();
        for (const auto& cd : cds) {
            if (!cd.item) {
                continue;
            }
            QString curveTitle = cd.item->title().text();
            if (curveTitle.isEmpty()) {
                curveTitle = tr("Curve");  // cn:曲线
            }
            QColor color = QwtPlotStyling::color(cd.item);
            QStandardItem* curveRow = m_model->ensureCurveRow(plotRow, cd.item, curveTitle, color);
            m_model->setCellData(curveRow, col, QString::number(cd.point.y(), 'g', 6));
        }
    }
    m_model->updateVisibility();
}

void DADataLinkTableTab::onProbeCreated(DA::DADataProbeMarker* probe)
{
    if (m_rebuilding || !probe) {
        return;
    }
    const QString name = probe->probeName().text();
    if (name.isEmpty()) {
        return;
    }
    int col = columnOfProbeGroup(name);
    if (col < 0) {
        col = m_model->addProbeColumn(probe->probeValue(), formatXValueForHeader(probe->probeValue()), name);
    }
    const QList< ProbeGroup > groups = scanProbeGroups();
    for (const ProbeGroup& g : groups) {
        if (g.name == name) {
            fillColumn(col, g);
            break;
        }
    }
}

void DADataLinkTableTab::onItemAttached(QwtPlotItem* item, bool on)
{
    if (m_rebuilding) {
        return;
    }
    if (item->rtti() == DA::DADataProbeMarker::Rtti_DataProbeMarker) {
        // 探针增删（探针模式/undo/redo/指针工具删除）-> 列级同步，延时合并批量探针组的逐个事件
        QTimer::singleShot(0, this, [this]() { rebuildFromProbes(); });
        return;
    }
    if (on) {
        if (!QwtPlotItemInfo::isSeriesItem(item)) {
            return;
        }
        QwtPlot* plot = item->plot();
        if (!plot) {
            return;
        }
        QString plotTitle = plot->title().text();
        if (plotTitle.isEmpty()) {
            plotTitle = tr("Chart %1").arg(plot->plotId());  // cn:绘图 %1
        }
        QStandardItem* plotRow = m_model->ensurePlotRow(plot, plotTitle);
        QString curveTitle = item->title().text();
        if (curveTitle.isEmpty()) {
            curveTitle = tr("Curve");  // cn:曲线
        }
        QColor color = QwtPlotStyling::color(item);
        m_model->ensureCurveRow(plotRow, item, curveTitle, color);
        syncRowsVisibility();
    } else {
        m_model->removeCurveRow(item);
    }
}

void DADataLinkTableTab::syncRowsVisibility()
{
    for (int r = 0; r < m_model->rowCount(); ++r) {
        QModelIndex plotIndex = m_model->index(r, 0);
        if (!m_model->data(plotIndex, DA::DADataLinkTableModel::RoleIsPlotNode).toBool()) {
            continue;
        }
        for (int c = 0; c < m_model->rowCount(plotIndex); ++c) {
            QModelIndex curveIndex = m_model->index(c, 0, plotIndex);
            auto* curveItem = reinterpret_cast< QwtPlotItem* >(
                m_model->data(curveIndex, DA::DADataLinkTableModel::RolePlotItem).value< quintptr >());
            if (!curveItem) {
                continue;
            }
            bool hide = !curveItem->isVisible();
            m_treeView->setRowHidden(c, plotIndex, hide);
        }
    }
}

void DADataLinkTableTab::onHeaderContextMenuRequested(const QPoint& pos)
{
    int col = m_treeView->header()->logicalIndexAt(pos);
    if (col < 0) {
        return;
    }
    m_contextMenuColumn = col;
    bool singleCol       = (col > 0);
    m_actRename->setEnabled(singleCol);
    m_actHide->setEnabled(singleCol);
    m_actDelete->setEnabled(singleCol);
    m_actRefresh->setEnabled(singleCol);

    m_subMenuShow->clear();
    int colCount = m_model->columnCount();
    for (int c = 1; c < colCount; ++c) {
        if (m_treeView->isColumnHidden(c)) {
            auto* header = m_model->horizontalHeaderItem(c);
            QString name = header ? header->text() : tr("Column %1").arg(c);  // cn:列 %1
            QAction* act = m_subMenuShow->addAction(name);
            connect(act, &QAction::triggered, this, [this, c]() {
                m_treeView->setColumnHidden(c, false);
                // 同步探针可见性
                const QList< ProbeGroup > groups = scanProbeGroups();
                QSet< QwtPlot* > dirtyPlots;
                for (const ProbeGroup& g : groups) {
                    if (g.name == m_model->columnLetter(c)) {
                        for (DA::DADataProbeMarker* probe : g.probes) {
                            probe->setVisible(true);
                            if (probe->plot()) {
                                dirtyPlots.insert(probe->plot());
                            }
                        }
                        break;
                    }
                }
                for (QwtPlot* plot : dirtyPlots) {
                    plot->replot();
                }
            });
        }
    }
    m_subMenuShow->menuAction()->setEnabled(!m_subMenuShow->isEmpty());

    m_headerMenu->exec(m_treeView->header()->mapToGlobal(pos));
}

void DADataLinkTableTab::onRenameColumn()
{
    if (m_contextMenuColumn <= 0) {
        return;
    }
    auto* header = m_model->horizontalHeaderItem(m_contextMenuColumn);
    if (!header) {
        return;
    }
    bool ok = false;
    QString newName = QInputDialog::getText(this, tr("Rename"),  // cn:重命名
                                            tr("New name:"),      // cn:新名称:
                                            QLineEdit::Normal, header->text(), &ok);
    if (ok && !newName.isEmpty()) {
        const QString oldName = header->text();
        m_model->renameColumn(m_contextMenuColumn, newName);
        // 同步这组探针的名称（徽章文字跟随表头）
        const QList< ProbeGroup > groups = scanProbeGroups();
        for (const ProbeGroup& g : groups) {
            if (g.name != oldName) {
                continue;
            }
            QSet< QwtPlot* > dirtyPlots;
            for (DA::DADataProbeMarker* probe : g.probes) {
                probe->setProbeName(QwtText(newName));
                if (probe->plot()) {
                    dirtyPlots.insert(probe->plot());
                }
            }
            for (QwtPlot* plot : dirtyPlots) {
                plot->replot();
            }
            break;
        }
    }
}

void DADataLinkTableTab::onHideColumn()
{
    if (m_contextMenuColumn <= 0) {
        return;
    }
    m_treeView->setColumnHidden(m_contextMenuColumn, true);
    const QString name = m_model->columnLetter(m_contextMenuColumn);
    const QList< ProbeGroup > groups = scanProbeGroups();
    QSet< QwtPlot* > dirtyPlots;
    for (const ProbeGroup& g : groups) {
        if (g.name != name) {
            continue;
        }
        for (DA::DADataProbeMarker* probe : g.probes) {
            probe->setVisible(false);
            if (probe->plot()) {
                dirtyPlots.insert(probe->plot());
            }
        }
        break;
    }
    for (QwtPlot* plot : dirtyPlots) {
        plot->replot();
    }
}

void DADataLinkTableTab::onDeleteColumn()
{
    if (m_contextMenuColumn <= 0) {
        return;
    }
    int ret = QMessageBox::question(this, tr("Delete"),  // cn:删除
                                    tr("Are you sure to delete this column?"),  // cn:确定删除这一列吗？
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    const QString name = m_model->columnLetter(m_contextMenuColumn);
    const QList< ProbeGroup > groups = scanProbeGroups();
    for (const ProbeGroup& g : groups) {
        if (g.name == name) {
            removeProbeGroup(g);
            break;
        }
    }
    // 探针 detach 会触发 itemAttached -> rebuildFromProbes 完成列移除
    m_contextMenuColumn = -1;
}

void DADataLinkTableTab::onHideAllColumns()
{
    int colCount = m_model->columnCount();
    for (int col = 1; col < colCount; ++col) {
        m_treeView->setColumnHidden(col, true);
    }
    const QList< ProbeGroup > groups = scanProbeGroups();
    QSet< QwtPlot* > dirtyPlots;
    for (const ProbeGroup& g : groups) {
        for (DA::DADataProbeMarker* probe : g.probes) {
            probe->setVisible(false);
            if (probe->plot()) {
                dirtyPlots.insert(probe->plot());
            }
        }
    }
    for (QwtPlot* plot : dirtyPlots) {
        plot->replot();
    }
}

void DADataLinkTableTab::onShowAllColumns()
{
    int colCount = m_model->columnCount();
    for (int col = 1; col < colCount; ++col) {
        m_treeView->setColumnHidden(col, false);
    }
    const QList< ProbeGroup > groups = scanProbeGroups();
    QSet< QwtPlot* > dirtyPlots;
    for (const ProbeGroup& g : groups) {
        for (DA::DADataProbeMarker* probe : g.probes) {
            probe->setVisible(true);
            if (probe->plot()) {
                dirtyPlots.insert(probe->plot());
            }
        }
    }
    for (QwtPlot* plot : dirtyPlots) {
        plot->replot();
    }
}

void DADataLinkTableTab::onDeleteAllColumns()
{
    int ret = QMessageBox::question(this, tr("Delete All"),  // cn:全部删除
                                    tr("Are you sure to delete all columns? This cannot be undone."),  // cn:确定删除所有列吗？此操作不可撤销。
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    const QList< ProbeGroup > groups = scanProbeGroups();
    for (const ProbeGroup& g : groups) {
        removeProbeGroup(g);
    }
    m_contextMenuColumn = -1;
}

void DADataLinkTableTab::removeProbeGroup(const ProbeGroup& group)
{
    if (!m_figureWidget) {
        return;
    }
    // 逐个探针走 undo 栈删除（detach 由命令驱动）
    for (DA::DADataProbeMarker* probe : group.probes) {
        QwtPlot* plot = probe->plot();
        if (!plot) {
            continue;
        }
        if (DA::DAChartWidget* chart = qobject_cast< DA::DAChartWidget* >(plot)) {
            m_figureWidget->removeItem_(chart, probe);
        } else {
            probe->detach();
            delete probe;
            plot->replot();
        }
    }
}

QString DADataLinkTableTab::formatXValueForHeader(double xValue) const
{
    if (!m_figureWidget) {
        return QString::number(xValue, 'g', 6);
    }
    QwtFigure* figure = m_figureWidget->figure();
    if (!figure) {
        return QString::number(xValue, 'g', 6);
    }
    const QList< QwtPlot* > plots = figure->allAxes();
    for (QwtPlot* plot : plots) {
        QwtAxisId xAxisId = plot->visibleXAxisId();
        if (!plot->isAxisVisible(xAxisId)) {
            continue;
        }
        const QwtScaleDraw* sd = plot->axisScaleDraw(xAxisId);
        if (sd) {
            QwtText text = sd->label(xValue);
            QString str  = text.text();
            if (!str.isEmpty()) {
                return str;
            }
        }
    }
    return QString::number(xValue, 'g', 6);
}

void DADataLinkTableTab::onRefreshColumn()
{
    if (m_contextMenuColumn <= 0) {
        return;
    }
    const QString name = m_model->columnLetter(m_contextMenuColumn);
    const QList< ProbeGroup > groups = scanProbeGroups();
    for (const ProbeGroup& g : groups) {
        if (g.name == name) {
            fillColumn(m_contextMenuColumn, g);
            break;
        }
    }
}

void DADataLinkTableTab::onRefreshAllColumns()
{
    const QList< ProbeGroup > groups = scanProbeGroups();
    for (const ProbeGroup& g : groups) {
        int col = columnOfProbeGroup(g.name);
        if (col > 0) {
            fillColumn(col, g);
        }
    }
}

}  // namespace DA
