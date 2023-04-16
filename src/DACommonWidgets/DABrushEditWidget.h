#ifndef DABRUSHEDITWIDGET_H
#define DABRUSHEDITWIDGET_H
#include "DACommonWidgetsAPI.h"
#include <QWidget>
#include <QBrush>
class SAColorMenu;
namespace Ui
{
class DABrushEditWidget;
}

namespace DA
{
/**
 * @brief 画刷编辑窗口
 */
class DACOMMONWIDGETS_API DABrushEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DABrushEditWidget(QWidget* parent = nullptr);
    ~DABrushEditWidget();
    //获取当前画刷
    QBrush getCurrentBrush() const;
public slots:
    //设置画刷,设置画刷会触发brushChanged信号
    void setCurrentBrush(const QBrush& b);
signals:
    /**
     * @brief 画刷发生改变
     * @param b
     */
    void brushChanged(const QBrush& b);
private slots:
    void onColorChanged(const QColor& c);
    void onBrushStyleChanged(Qt::BrushStyle s);

private:
    Ui::DABrushEditWidget* ui;
    QBrush mBrush;
    SAColorMenu* mColorMenu { nullptr };
};
}

#endif  // DABRUSHEDITWIDGET_H
