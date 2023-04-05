#ifndef DAFONTEDITPANNELWIDGET_H
#define DAFONTEDITPANNELWIDGET_H
#include "DACommonWidgetsAPI.h"
#include <QWidget>

namespace Ui
{
class DAFontEditPannelWidget;
}
namespace DA
{
class DACOMMONWIDGETS_API DAFontEditPannelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAFontEditPannelWidget(QWidget* parent = nullptr);
    ~DAFontEditPannelWidget();
    //字体颜色
    QColor getCurrentFontColor() const;
    void setCurrentFontColor(const QColor& c);
    //设置当前的字体
    void setCurrentFont(const QFont& f);
    QFont getCurrentFont();
    //字体大小
    void setFontWeight(int w);
    int getFontWeight() const;
private slots:
    void onFontComboBoxCurrentFontChanged(const QFont& f);
    void onComboBoxFontSizeTextChanged(const QString& t);
    void onFontSizeAdd();
    void onFontSizeSub();
    //发射字体改变的信号
    void signalEmitFontChanged();
signals:
    /**
     * @brief 当前字体改变发射的信号
     * @param font
     */
    void currentFontChanged(const QFont& f);
    /**
     * @brief 当前字体颜色改变发射的信号
     * @param c
     */
    void currentFontColorChanged(const QColor& c);

private:
    Ui::DAFontEditPannelWidget* ui;
    int _fontPointSize;
};
}
#endif  // DAFONTEDITPANNELWIDGET_H
