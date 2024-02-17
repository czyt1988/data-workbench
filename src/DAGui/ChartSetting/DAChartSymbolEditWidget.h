#ifndef DACHARTSYMBOLEDITWIDGET_H
#define DACHARTSYMBOLEDITWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "qwt_symbol.h"
namespace Ui
{
class DAChartSymbolEditWidget;
}
namespace DA
{
class DAGUI_API DAChartSymbolEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartSymbolEditWidget(QWidget* parent = nullptr);
    ~DAChartSymbolEditWidget();

    // Symbol Style
    void setSymbolStyle(QwtSymbol::Style v);
    QwtSymbol::Style getSymbolStyle() const;
    // Symbol Size
    void setSymbolSize(int v);
    int getSymbolSize() const;
    // Symbol Color
    void setSymbolColor(const QColor& v);
    QColor getSymbolColor() const;
    // Outline
    void setSymbolOutlinePen(const QPen& v);
    QPen getSymbolOutlinePen() const;
signals:
    /**
     * @brief 符号标记改变信号
     * @param s
     */
    void symbolStyleChanged(QwtSymbol::Style);

    /**
     * @brief 符号尺寸改变
     * @param v
     */
    void symbolSizeChanged(int);

    /**
     * @brief 符号颜色改变
     * @param v
     */
    void symbolColorChanged(const QColor&);

    /**
     * @brief 符号Outline改变
     * @param v
     */
    void symbolOutlinePenChanged(const QPen&);

private:
    void init();

private:
    Ui::DAChartSymbolEditWidget* ui;
};
}

#endif  // DACHARTSYMBOLEDITWIDGET_H
