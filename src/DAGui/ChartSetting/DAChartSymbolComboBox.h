#ifndef DACHARTSYMBOLCOMBOBOX_H
#define DACHARTSYMBOLCOMBOBOX_H
#include <QComboBox>
#include "DAGuiAPI.h"
#include "qwt_symbol.h"
namespace DA
{
/**
 * @brief 针对QwtSymbol的combobox，此类参考qtiplot的SymbolBox
 */
class DAGUI_API DAChartSymbolComboBox : public QComboBox
{
    Q_OBJECT
public:
    DAChartSymbolComboBox(QWidget* par = 0);
    //
    void setSymbolStyle(const QwtSymbol::Style& s);
    QwtSymbol::Style getSymbolStyle() const;
    static QwtSymbol::Style style(int index);
    static int symbolIndex(const QwtSymbol::Style& s);
signals:
    /**
     * @brief 符号标记改变信号
     * @param s
     */
    void symbolStyleChanged(QwtSymbol::Style s);
private slots:
    void onCurrentIndexChanged(int index);

private:
    void buildItems();

private:
    //    static const QwtSymbol::Style s_symbols[];
};
}
Q_DECLARE_METATYPE(QwtSymbol::Style)

#endif  // DACHARTSYMBOLCOMBOBOX_H
