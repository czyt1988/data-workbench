#include "DAChartAutoincrementSeriesWidget.h"
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>

namespace DA
{

class DAChartAutoincrementSeriesWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartAutoincrementSeriesWidget)
public:
    PrivateData(DAChartAutoincrementSeriesWidget* p);

    QDoubleSpinBox* mStart { nullptr };
    QDoubleSpinBox* mStep { nullptr };
};

DAChartAutoincrementSeriesWidget::PrivateData::PrivateData(DAChartAutoincrementSeriesWidget* p) : q_ptr(p)
{
}

/**
 * @brief 构造自增序列参数组
 * @param parent 父控件
 */
DAChartAutoincrementSeriesWidget::DAChartAutoincrementSeriesWidget(QWidget* parent)
    : QGroupBox(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    setTitle(tr("Autoincrement series"));  // cn:自增序列
    setCheckable(true);
    setChecked(false);

    QGridLayout* lay = new QGridLayout(this);
    lay->setContentsMargins(8, 4, 8, 4);
    lay->setSpacing(4);

    QLabel* startLabel = new QLabel(tr("Initial value"), this);  // cn:初始值
    d->mStart          = new QDoubleSpinBox(this);
    d->mStart->setObjectName(QStringLiteral("startValueSpin"));
    d->mStart->setDecimals(6);
    d->mStart->setRange(-1e12, 1e12);
    d->mStart->setValue(1.0);

    QLabel* stepLabel = new QLabel(tr("Self increasing step size"), this);  // cn:自增步长
    d->mStep          = new QDoubleSpinBox(this);
    d->mStep->setObjectName(QStringLiteral("stepValueSpin"));
    d->mStep->setDecimals(6);
    d->mStep->setRange(-1e12, 1e12);
    d->mStep->setValue(1.0);

    lay->addWidget(startLabel, 0, 0);
    lay->addWidget(d->mStart, 0, 1);
    lay->addWidget(stepLabel, 1, 0);
    lay->addWidget(d->mStep, 1, 1);

    connect(this, &QGroupBox::toggled, this, &DAChartAutoincrementSeriesWidget::seriesEnabledChanged);
    connect(d->mStart, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, [ this ](double) {
        Q_EMIT seriesConfigChanged();
    });
    connect(d->mStep, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, [ this ](double) {
        Q_EMIT seriesConfigChanged();
    });
}

/**
 * @brief 析构
 */
DAChartAutoincrementSeriesWidget::~DAChartAutoincrementSeriesWidget()
{
}

/**
 * @brief 设置是否使用自增序列
 * @param on true = 使用自增序列
 */
void DAChartAutoincrementSeriesWidget::setSeriesEnabled(bool on)
{
    setChecked(on);
}

/**
 * @brief 是否使用自增序列
 * @return true = 使用自增序列
 */
bool DAChartAutoincrementSeriesWidget::isSeriesEnabled() const
{
    return isChecked();
}

/**
 * @brief 设置初值
 * @param v 初值
 */
void DAChartAutoincrementSeriesWidget::setStartValue(double v)
{
    DA_D(d);
    d->mStart->setValue(v);
}

/**
 * @brief 获取初值
 * @return 初值
 */
double DAChartAutoincrementSeriesWidget::startValue() const
{
    DA_DC(d);
    return d->mStart->value();
}

/**
 * @brief 设置步长
 * @param v 步长
 */
void DAChartAutoincrementSeriesWidget::setStepValue(double v)
{
    DA_D(d);
    d->mStep->setValue(v);
}

/**
 * @brief 获取步长
 * @return 步长
 */
double DAChartAutoincrementSeriesWidget::stepValue() const
{
    DA_DC(d);
    return d->mStep->value();
}

/**
 * @brief 设置初值/步长的取值范围
 * @param min 最小值
 * @param max 最大值
 */
void DAChartAutoincrementSeriesWidget::setRange(double min, double max)
{
    DA_D(d);
    d->mStart->setRange(min, max);
    d->mStep->setRange(min, max);
}

/**
 * @brief 设置初值/步长的小数位数
 * @param n 小数位数
 */
void DAChartAutoincrementSeriesWidget::setDecimals(int n)
{
    DA_D(d);
    d->mStart->setDecimals(n);
    d->mStep->setDecimals(n);
}

}  // namespace DA
