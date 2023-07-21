#ifndef DAGRAPHICSPIXMAPITEMSETTINGWIDGET_H
#define DAGRAPHICSPIXMAPITEMSETTINGWIDGET_H

#include <QWidget>

namespace Ui
{
class DAGraphicsPixmapItemSettingWidget;
}

namespace DA
{

class DAGraphicsPixmapItemSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAGraphicsPixmapItemSettingWidget(QWidget* parent = nullptr);
    ~DAGraphicsPixmapItemSettingWidget();
signals:
    /**
     * @brief 图片的透明度改变信号
     * @param v
     */
    void pixmapAlphaValueChanged(int v);
private slots:
    //滑竿控件改变
    void onHorizontalSliderAlphaValueChanged(int v);
    //数值控件改变
    void onSpinBoxValueChanged(int v);

private:
    Ui::DAGraphicsPixmapItemSettingWidget* ui;
};
}

#endif  // DAGRAPHICSPIXMAPITEMSETTINGWIDGET_H
