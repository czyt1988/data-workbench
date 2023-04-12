#ifndef DACHARTADDXYSERIESWIDGET_H
#define DACHARTADDXYSERIESWIDGET_H

#include <QWidget>

namespace Ui
{
class DAChartAddXYSeriesWidget;
}
namespace DA
{

class DADataManager;
class DAChartAddXYSeriesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartAddXYSeriesWidget(QWidget* parent = nullptr);
    ~DAChartAddXYSeriesWidget();
    //
    void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
    //获取为vector pointf
    bool getToVectorPointF(QVector< QPointF >& res);

private:
    Ui::DAChartAddXYSeriesWidget* ui;
};
}
#endif  // DACHARTADDXYSERIESWIDGET_H
