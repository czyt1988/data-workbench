#include "DAChartSymbolComboBox.h"
#include <QPainter>
#include <vector>
#include <iterator>
namespace DA
{

const std::vector< QwtSymbol::Style > s_symbols = { QwtSymbol::NoSymbol,  QwtSymbol::Ellipse,   QwtSymbol::Rect,
                                                    QwtSymbol::Diamond,   QwtSymbol::Triangle,  QwtSymbol::DTriangle,
                                                    QwtSymbol::UTriangle, QwtSymbol::LTriangle, QwtSymbol::RTriangle,
                                                    QwtSymbol::Cross,     QwtSymbol::XCross,    QwtSymbol::HLine,
                                                    QwtSymbol::VLine,     QwtSymbol::Star1,     QwtSymbol::Star2,
                                                    QwtSymbol::Hexagon };

DAChartSymbolComboBox::DAChartSymbolComboBox(QWidget* par) : QComboBox(par)
{
    setIconSize(QSize(16, 16));
    buildItems();
    connect(this, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAChartSymbolComboBox::onCurrentIndexChanged);
}

void DAChartSymbolComboBox::buildItems()
{
    clear();
    QSize ics = iconSize();
    if (ics.isNull()) {
        ics = QSize(16, 16);
    }
    QPixmap icon = QPixmap(ics);
    QColor c     = QColor(Qt::transparent);
    icon.fill(c);
    const QRect r = QRect(1, 1, ics.width() - 1, ics.height() - 1);
    QPainter p(&icon);
    p.setRenderHint(QPainter::Antialiasing);
    QwtSymbol symb;
    p.setBrush(QBrush(QColor(Qt::white)));

    this->addItem(tr("No Symbol"));

    symb.setStyle(QwtSymbol::Ellipse);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Ellipse"));

    symb.setStyle(QwtSymbol::Rect);
    icon.fill(c);
    symb.drawSymbol(&p, r.adjusted(0, 0, -1, -1));
    this->addItem(icon, tr("Rectangle"));

    symb.setStyle(QwtSymbol::Diamond);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Diamond"));

    symb.setStyle(QwtSymbol::Triangle);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Triangle"));

    symb.setStyle(QwtSymbol::DTriangle);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Down Triangle"));

    symb.setStyle(QwtSymbol::UTriangle);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Up Triangle"));

    symb.setStyle(QwtSymbol::LTriangle);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Left Triangle"));

    symb.setStyle(QwtSymbol::RTriangle);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Right Triangle"));

    symb.setStyle(QwtSymbol::Cross);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Cross"));

    symb.setStyle(QwtSymbol::XCross);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Diagonal Cross"));

    symb.setStyle(QwtSymbol::HLine);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Horizontal Line"));

    symb.setStyle(QwtSymbol::VLine);
    p.eraseRect(r);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Vertical Line"));

    symb.setStyle(QwtSymbol::Star1);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Star 1"));

    symb.setStyle(QwtSymbol::Star2);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Star 2"));

    symb.setStyle(QwtSymbol::Hexagon);
    icon.fill(c);
    symb.drawSymbol(&p, r);
    this->addItem(icon, tr("Hexagon"));

    p.end();
}
void DAChartSymbolComboBox::setSymbolStyle(const QwtSymbol::Style& s)
{
    auto ite = std::find(s_symbols.begin(), s_symbols.end(), s);
    if (ite == s_symbols.end()) {
        this->setCurrentIndex(0);
    } else {
        this->setCurrentIndex(std::distance(s_symbols.begin(), ite));
    }
}

QwtSymbol::Style DAChartSymbolComboBox::getSymbolStyle() const
{
    size_t i = this->currentIndex();
    if (i < s_symbols.size()) {
        return s_symbols[ this->currentIndex() ];
    }

    return QwtSymbol::NoSymbol;
}

QwtSymbol::Style DAChartSymbolComboBox::style(int index)
{
    if (index >= 0 && index < s_symbols.size()) {
        return s_symbols[ index ];
    }

    return QwtSymbol::NoSymbol;
}

int DAChartSymbolComboBox::symbolIndex(const QwtSymbol::Style& s)
{
    auto ite = std::find(s_symbols.begin(), s_symbols.end(), s);
    if (ite == s_symbols.end()) {
        return 0;
    }

    return std::distance(s_symbols.begin(), ite);
}

void DAChartSymbolComboBox::onCurrentIndexChanged(int index)
{
    emit symbolStyleChanged(getSymbolStyle());
}

}
