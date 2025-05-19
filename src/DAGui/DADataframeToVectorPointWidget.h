#ifndef DADATAFRAMETOVECTORPOINTWIDGET_H
#define DADATAFRAMETOVECTORPOINTWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAData.h"
namespace Ui
{
class DADataframeToVectorPointWidget;
}

namespace DA
{
class DAPySeriesTableModel;
/**
 * @brief Dataframe To VectorPoint
 */
class DAGUI_API DADataframeToVectorPointWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DADataframeToVectorPointWidget(QWidget* parent = nullptr);
    ~DADataframeToVectorPointWidget();
    //设置datafram
    void setCurrentData(const DAData& d);
    DAData getCurrentData() const;
    //获取为vector pointf
    bool getToVectorPointF(QVector< QPointF >& res);

    //刷新x，y两个列选择listwidget
    void updateDataframeColumnList();
private slots:
    void onListWidgetXCurrentTextChanged(const QString& n);
    void onListWidgetYCurrentTextChanged(const QString& n);

private:
    Ui::DADataframeToVectorPointWidget* ui;
    DAData _currentData;
    DAPySeriesTableModel* _model;
};
}

#endif  // DADATAFRAMETOVECTORPOINTWIDGET_H
