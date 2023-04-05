#include "DAPenEditWidget.h"
#include <QVariant>
#include <QApplication>
#include <QSignalBlocker>
#include <QHBoxLayout>
#include <QSpinBox>
#include "DAColorPickerButton.h"
#include "DAPenStyleComboBox.h"

namespace DA
{
class DAPenEditWidgetPrivate
{
    DA_IMPL_PUBLIC(DAPenEditWidget)
public:
    DAPenEditWidgetPrivate(DAPenEditWidget* p);

public:
    QHBoxLayout* horizontalLayout;
    DAColorPickerButton* pushButtonColor;
    DAPenStyleComboBox* comboBox;
    QSpinBox* spinBoxWidth;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPenEditWidgetPrivate
//===================================================

DAPenEditWidgetPrivate::DAPenEditWidgetPrivate(DAPenEditWidget* p) : q_ptr(p)
{
    if (p->objectName().isEmpty())
        p->setObjectName(QStringLiteral("DAPenEditWidget"));
    p->resize(166, 25);
    horizontalLayout = new QHBoxLayout(p);
    horizontalLayout->setSpacing(1);
    horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    horizontalLayout->setContentsMargins(1, 1, 1, 1);
    pushButtonColor = new DAColorPickerButton(p);
    pushButtonColor->setObjectName(QStringLiteral("pushButtonColor"));

    horizontalLayout->addWidget(pushButtonColor);

    comboBox = new DAPenStyleComboBox(p);
    comboBox->setObjectName(QStringLiteral("comboBox"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
    comboBox->setSizePolicy(sizePolicy);

    horizontalLayout->addWidget(comboBox);

    spinBoxWidth = new QSpinBox(p);
    spinBoxWidth->setObjectName(QStringLiteral("spinBoxWidth"));

    horizontalLayout->addWidget(spinBoxWidth);
}

DAPenEditWidget::DAPenEditWidget(QWidget* parent) : QWidget(parent), d_ptr(new DAPenEditWidgetPrivate(this))
{
    QPen p;
    p.setStyle(Qt::SolidLine);
    p.setColor(Qt::black);
    p.setWidth(1);
    setCurrentPen(p);
    init();
}

DAPenEditWidget::DAPenEditWidget(const QPen& p, QWidget* parent)
    : QWidget(parent), d_ptr(new DAPenEditWidgetPrivate(this)), _pen(p)
{
    setCurrentPen(p);
    init();
}

DAPenEditWidget::~DAPenEditWidget()
{
}

void DAPenEditWidget::setCurrentPen(const QPen& p)
{
    QSignalBlocker bl(d_ptr->pushButtonColor);
    QSignalBlocker bl2(d_ptr->comboBox);
    QSignalBlocker bl3(d_ptr->spinBoxWidth);
    Q_UNUSED(bl);
    Q_UNUSED(bl2);
    Q_UNUSED(bl3);
    _pen = p;
    d_ptr->pushButtonColor->setColor(_pen.color());
    d_ptr->comboBox->setPenColor(_pen.color());
    d_ptr->comboBox->setPenLineWidth(_pen.width());
    d_ptr->comboBox->updateItems();
    d_ptr->spinBoxWidth->setValue(_pen.width());
    emit penChanged(_pen);
}

void DAPenEditWidget::retranslateUi()
{
    setWindowTitle(QCoreApplication::translate("DAPenEditWidget", "Pen Editor", nullptr));
    d_ptr->spinBoxWidth->setSuffix(QCoreApplication::translate("DAPenEditWidget", " px", nullptr));
}

QPen DAPenEditWidget::getCurrentPen() const
{
    return _pen;
}

void DAPenEditWidget::onColorChanged(const QColor& c)
{
    _pen.setColor(c);
    d_ptr->comboBox->setPenColor(c);
    d_ptr->comboBox->updateItems();
    emit penChanged(_pen);
}

void DAPenEditWidget::onPenWidthValueChanged(int w)
{
    _pen.setWidth(w);
    d_ptr->comboBox->setPenLineWidth(w);
    d_ptr->comboBox->updateItems();
    emit penChanged(_pen);
}

void DAPenEditWidget::onPenStyleChanged(Qt::PenStyle s)
{
    _pen.setStyle(s);
    emit penChanged(_pen);
}

void DAPenEditWidget::init()
{
    d_ptr->comboBox->setPenColor(_pen.color());
    d_ptr->comboBox->setPenLineWidth(_pen.width());
    d_ptr->comboBox->updateItems();
    connect(d_ptr->pushButtonColor, &DAColorPickerButton::colorChanged, this, &DAPenEditWidget::onColorChanged);
    connect(d_ptr->spinBoxWidth, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAPenEditWidget::onPenWidthValueChanged);
    connect(d_ptr->comboBox, &DAPenStyleComboBox::currentPenStyleChanged, this, &DAPenEditWidget::onPenStyleChanged);
}
