#ifndef DAPLOTNODESETTINGWIDGET_H
#define DAPLOTNODESETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
#include "DAGuiAPI.h"

namespace Ui
{
class DAPlotNodeSettingWidget;
}
namespace QIM
{
class QImPlotNode;
}
namespace DA
{
/**
 * @brief 图表设置窗口
 */
class DAGUI_API DAPlotNodeSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAPlotNodeSettingWidget(QWidget* parent = nullptr);
    ~DAPlotNodeSettingWidget();
    // 设置chart
    void setPlotNode(QIM::QImPlotNode* w);
    QIM::QImPlotNode* getPlotNode() const;
    // 更新ui
    void updateUI();

private slots:
    // 标题内容设置
    void onTitleTextChanged(const QString& t);
    void onTitleFontChanged(const QFont& f);
    void onTitleColorChanged(const QColor& c);
    // footer内容设置
    void onFooterTextChanged(const QString& t);
    void onFooterFontChanged(const QFont& f);
    void onFooterColorChanged(const QColor& c);

private:
    Ui::DAPlotNodeSettingWidget* ui;
    QPointer< QIM::QImPlotNode > mChartPlot;
};
}  // end DA
#endif  // DAPLOTNODESETTINGWIDGET_H
