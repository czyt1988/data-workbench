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

private:
    Ui::DADataframeToVectorPoint* ui;
    DAData _currentData;
};
}

#endif  // DADATAFRAMETOVECTORPOINT_H
