#ifndef DADATAFRAMETOVECTORPOINT_H
#define DADATAFRAMETOVECTORPOINT_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAData.h"
namespace Ui
{
class DADataframeToVectorPoint;
}

namespace DA
{
class DAPySeriesTableModule;
/**
 * @brief Dataframe To VectorPoint
 */
class DAGUI_API DADataframeToVectorPoint : public QWidget
{
    Q_OBJECT

public:
    explicit DADataframeToVectorPoint(QWidget* parent = nullptr);
    ~DADataframeToVectorPoint();
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
    Ui::DADataframeToVectorPoint* ui;
    DAData _currentData;
    DAPySeriesTableModule* _model;
    bool _xNeedInsert0 { false };
};
}

#endif  // DADATAFRAMETOVECTORPOINT_H
