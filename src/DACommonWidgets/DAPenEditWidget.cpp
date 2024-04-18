#include "DAPenEditWidget.h"
#include <QVariant>
#include <QApplication>
#include <QSignalBlocker>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include "DAColorPickerButton.h"
#include "DAPenStyleComboBox.h"
namespace DA
{
class DAPenEditWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAPenEditWidget)
public:
	PrivateData(DAPenEditWidget* p);

public:
	QPen mPen;
	QHBoxLayout* horizontalLayout;
	DAColorPickerButton* colorButton;
	DAPenStyleComboBox* comboBox;
	QDoubleSpinBox* spinBoxWidth;
};

//===================================================
// DAPenEditWidgetPrivate
//===================================================

DAPenEditWidget::PrivateData::PrivateData(DAPenEditWidget* p) : q_ptr(p)
{
	if (p->objectName().isEmpty())
		p->setObjectName(QStringLiteral("DAPenEditWidget"));
	p->resize(166, 25);
	horizontalLayout = new QHBoxLayout(p);
	horizontalLayout->setSpacing(1);
	horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
	horizontalLayout->setContentsMargins(1, 1, 1, 1);
	colorButton = new DAColorPickerButton(p);
	colorButton->setAutoRaise(true);
	colorButton->setColor(QColor());
	colorButton->setObjectName(QStringLiteral("pushButtonColor"));
	horizontalLayout->addWidget(colorButton);

	comboBox = new DAPenStyleComboBox(p);
	comboBox->setObjectName(QStringLiteral("comboBox"));
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
	comboBox->setSizePolicy(sizePolicy);

	horizontalLayout->addWidget(comboBox);

	spinBoxWidth = new QDoubleSpinBox(p);
	spinBoxWidth->setObjectName(QStringLiteral("spinBoxWidth"));
	spinBoxWidth->setMinimum(0.0);
	spinBoxWidth->setMaximum(99.0);
	spinBoxWidth->setDecimals(1);
	spinBoxWidth->setSingleStep(0.5);
	horizontalLayout->addWidget(spinBoxWidth);

    p->setMinimumHeight(22);
}

//===================================================
// DAPenEditWidget
//===================================================

DAPenEditWidget::DAPenEditWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
	QPen p;
	p.setStyle(Qt::SolidLine);
	p.setColor(QColor());
	p.setWidth(1.0);
	setCurrentPen(p);
	initConnect();
}

DAPenEditWidget::DAPenEditWidget(const QPen& p, QWidget* parent) : QWidget(parent), d_ptr(new PrivateData(this))
{
	setCurrentPen(p);
	initConnect();
}

DAPenEditWidget::~DAPenEditWidget()
{
}

/**
 * @brief 设置画笔,设置画笔会触发penChanged信号
 * @param p
 */
void DAPenEditWidget::setCurrentPen(const QPen& p)
{
	QSignalBlocker bl(d_ptr->colorButton);
	QSignalBlocker bl2(d_ptr->comboBox);
	QSignalBlocker bl3(d_ptr->spinBoxWidth);
	Q_UNUSED(bl);
	Q_UNUSED(bl2);
	Q_UNUSED(bl3);
	d_ptr->mPen = p;
	d_ptr->colorButton->setColor(d_ptr->mPen.color());
	if (p.style() == Qt::NoPen) {
		d_ptr->colorButton->setColor(QColor());
	}
	d_ptr->comboBox->setCurrentPenStyle(p.style());
	d_ptr->comboBox->setPenColor(d_ptr->mPen.color());
	d_ptr->comboBox->setPenLineWidth(d_ptr->mPen.width());
	d_ptr->comboBox->updateItems();
	d_ptr->spinBoxWidth->setValue(d_ptr->mPen.width());
	d_ptr->colorButton->setEnabled(d_ptr->mPen.style() != Qt::NoPen);
	emit penChanged(d_ptr->mPen);
}

void DAPenEditWidget::retranslateUi()
{
	setWindowTitle(QCoreApplication::translate("DAPenEditWidget", "Pen Editor", nullptr));
	d_ptr->spinBoxWidth->setSuffix(QCoreApplication::translate("DAPenEditWidget", " px", nullptr));
}

QPen DAPenEditWidget::getCurrentPen() const
{
	QPen p(d_ptr->colorButton->color());
	p.setStyle(d_ptr->comboBox->getCurrentPenStyle());
	p.setWidthF(d_ptr->spinBoxWidth->value());
	return p;
}

void DAPenEditWidget::setLineStyleTextVisible(bool on)
{
	d_ptr->comboBox->setStyleTextVisible(on);
}

bool DAPenEditWidget::isLineStyleTextVisible() const
{
	return d_ptr->comboBox->isStyleTextVisible();
}

void DAPenEditWidget::onColorChanged(const QColor& c)
{
	d_ptr->mPen.setColor(c);
	d_ptr->comboBox->setPenColor(c);
	d_ptr->comboBox->updateItems();
	emit penChanged(d_ptr->mPen);
}

void DAPenEditWidget::onPenWidthValueChanged(double w)
{
	d_ptr->mPen.setWidthF(w);
	d_ptr->comboBox->setPenLineWidth(w);
	d_ptr->comboBox->updateItems();
	emit penChanged(d_ptr->mPen);
}

void DAPenEditWidget::onPenStyleChanged(Qt::PenStyle s)
{
	if (d_ptr->mPen.style() == Qt::NoPen) {
		// 原来是no pen
		if (Qt::NoPen != s) {
			d_ptr->colorButton->setColor(d_ptr->mPen.color());
		}
	} else {
		// 原来是不是no pen变为no pen
		if (Qt::NoPen == s) {
			d_ptr->colorButton->setColor(QColor());
		}
	}

	d_ptr->colorButton->setEnabled(s != Qt::NoPen);
	d_ptr->mPen.setStyle(s);
	emit penChanged(d_ptr->mPen);
}

void DAPenEditWidget::initConnect()
{
	connect(d_ptr->colorButton, &DAColorPickerButton::colorChanged, this, &DAPenEditWidget::onColorChanged);
	connect(d_ptr->spinBoxWidth,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DAPenEditWidget::onPenWidthValueChanged);
	connect(d_ptr->comboBox, &DAPenStyleComboBox::currentPenStyleChanged, this, &DAPenEditWidget::onPenStyleChanged);
}
}  // namespace DA
