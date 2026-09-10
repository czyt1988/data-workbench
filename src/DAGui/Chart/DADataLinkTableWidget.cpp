#include "DADataLinkTableWidget.h"
#include "DADataLinkTableTab.h"
// Qt
#include <QTabWidget>
#include <QVBoxLayout>
#include <QTimer>
// DA
#include "DAFigureWidget.h"
#include "DAChartOperateWidget.h"
#include "DADataProbeMarker.h"

namespace DA
{

DADataLinkTableWidget::DADataLinkTableWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        DA::DADataLinkTableTab* tab = qobject_cast< DA::DADataLinkTableTab* >(m_tabWidget->widget(index));
        if (tab) {
            for (auto it = m_tabs.begin(); it != m_tabs.end(); ++it) {
                if (it.value() == tab) {
                    m_tabs.erase(it);
                    break;
                }
            }
            m_tabWidget->removeTab(index);
            tab->deleteLater();
        }
    });
}

DADataLinkTableWidget::~DADataLinkTableWidget()
{
}

void DADataLinkTableWidget::setChartOperateWidget(DA::DAChartOperateWidget* chartOpt)
{
    m_chartOpt = chartOpt;
    if (!chartOpt) {
        return;
    }
    connect(chartOpt, &DA::DAChartOperateWidget::figureCreated,
            this, &DADataLinkTableWidget::onFigureCreated);
    connect(chartOpt, &DA::DAChartOperateWidget::figureRemoving,
            this, &DADataLinkTableWidget::onFigureRemoving);

    const QList< DA::DAFigureWidget* > figs = chartOpt->getFigureList();
    for (DA::DAFigureWidget* fig : figs) {
        onFigureCreated(fig);
    }
}

void DADataLinkTableWidget::onFigureCreated(DA::DAFigureWidget* figWidget)
{
    if (!figWidget) {
        return;
    }
    // 探针创建 -> 对应 tab 加列并请求前置
    connect(figWidget, &DA::DAFigureWidget::dataProbeCreated, this,
            [this, figWidget](DA::DADataProbeMarker* probe) { onProbeCreated(figWidget, probe); }, Qt::UniqueConnection);
}

void DADataLinkTableWidget::onFigureRemoving(DA::DAFigureWidget* figWidget)
{
    if (!figWidget) {
        return;
    }
    disconnect(figWidget, &DA::DAFigureWidget::dataProbeCreated, this, nullptr);
    auto it = m_tabs.find(figWidget);
    if (it == m_tabs.end()) {
        return;
    }
    DA::DADataLinkTableTab* tab = it.value();
    m_tabs.erase(it);
    int idx = findTabIndex(figWidget);
    if (idx >= 0) {
        m_tabWidget->removeTab(idx);
    }
    tab->deleteLater();
}

void DADataLinkTableWidget::refreshFigure(DA::DAFigureWidget* figWidget)
{
    if (!figWidget) {
        return;
    }
    if (DA::DADataLinkTableTab* tab = ensureTab(figWidget)) {
        tab->rebuildFromProbes();
    }
}

int DADataLinkTableWidget::findTabIndex(DA::DAFigureWidget* figWidget) const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        DA::DADataLinkTableTab* tab = qobject_cast< DA::DADataLinkTableTab* >(m_tabWidget->widget(i));
        if (tab && tab->figureWidget() == figWidget) {
            return i;
        }
    }
    return -1;
}

DA::DADataLinkTableTab* DADataLinkTableWidget::ensureTab(DA::DAFigureWidget* figWidget)
{
    auto it = m_tabs.find(figWidget);
    if (it != m_tabs.end()) {
        return it.value();
    }
    DA::DADataLinkTableTab* tab = new DA::DADataLinkTableTab(figWidget);
    QString figName;
    if (m_chartOpt) {
        figName = m_chartOpt->getFigureName(figWidget);
    }
    if (figName.isEmpty()) {
        figName = DADataLinkTableWidget::tr("Figure");  // cn:绘图
    }
    m_tabWidget->addTab(tab, figName);
    m_tabs[ figWidget ] = tab;
    m_tabWidget->setCurrentWidget(tab);
    return tab;
}

void DADataLinkTableWidget::onProbeCreated(DA::DAFigureWidget* figWidget, DA::DADataProbeMarker* probe)
{
    DA::DADataLinkTableTab* tab = ensureTab(figWidget);
    if (tab) {
        tab->onProbeCreated(probe);
    }
    Q_EMIT requestRaise();
}

}  // namespace DA
