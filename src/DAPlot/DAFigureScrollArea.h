#ifndef DAFIGURESCROLLAREA_H
#define DAFIGURESCROLLAREA_H
#include "DAPlotAPI.h"
#include <QScrollArea>
class QImFigureWidget;
namespace DA
{
/**
 * @brief 用于展示绘图的滚动区域
 */
class DAPLOT_API DAFigureScrollArea : public QScrollArea
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureScrollArea)
public:
    explicit DAFigureScrollArea(QWidget* parent = nullptr);
    ~DAFigureScrollArea();
    // 获取绘图窗口
    QImFigureWidget* figure() const;
    // id
    QString getFigureId() const;
    void setFigureId(const QString& id);

private:
    void init();
};
}
#endif  // DAFIGURESCROLLAREA_H
