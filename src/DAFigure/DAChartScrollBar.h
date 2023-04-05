#ifndef DACHARTSCROLLBAR_H
#define DACHARTSCROLLBAR_H
#include "DAFigureAPI.h"
#include <QScrollBar>
namespace DA
{
class DAFIGURE_API DAChartScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    DAChartScrollBar(QWidget* parent = NULL);
    DAChartScrollBar(Qt::Orientation, QWidget* parent = NULL);
    DAChartScrollBar(double minBase, double maxBase, Qt::Orientation o, QWidget* parent = NULL);

    void setInverted(bool);
    bool isInverted() const;

    double minBaseValue() const;
    double maxBaseValue() const;

    double minSliderValue() const;
    double maxSliderValue() const;

    int extent() const;

signals:
    void sliderMoved(Qt::Orientation, double, double);
    void valueChanged(Qt::Orientation, double, double);

public slots:
    virtual void setBase(double min, double max);
    virtual void moveSlider(double min, double max);
private slots:
    void catchValueChanged(int value);
    void catchSliderMoved(int value);

protected:
    void sliderRange(int value, double& min, double& max) const;
    int mapToTick(double) const;
    double mapFromTick(int) const;

private:
    void init();

    bool d_inverted;
    double d_minBase;
    double d_maxBase;
    int d_baseTicks;
};
}  // End Of Namespace DA
#endif  // DACHARTSCROLLBAR_H
