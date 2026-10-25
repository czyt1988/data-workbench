#include "DASpinBoxStyle.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QFontMetrics>
#include <QSpinBox>
#include <QStyleFactory>
#include <QStyleOptionSpinBox>

namespace DA
{

/**
 * @brief 构造代理样式
 * @param base 基础样式，为空时由 QProxyStyle 使用应用当前样式；本类在销毁时接管其所有权
 */
DASpinBoxStyle::DASpinBoxStyle(QStyle* base) : QProxyStyle(base)
{
}

/**
 * @brief 析构
 */
DASpinBoxStyle::~DASpinBoxStyle() = default;

/**
 * @brief 安装到应用
 *
 * 任何样式在窗口挂上 QSS 后（SARibbon 主题必然挂）都会由 QStyleSheetStyle 计算
 * SpinBox 尺寸，把数值区压到不足 4px，因此所有平台样式都需要补最小宽度。按钮
 * 布局保持平台原生（Windows 11 的左右按钮点击区域比竖排更大）。
 *
 * 基础样式重新创建为独立实例：QApplication::setStyle 会释放旧样式，不能复用
 * QApplication::style() 返回的指针。
 *
 * @return 已安装返回 true；样式名为空（已经是代理样式）或无法创建基础样式返回 false
 */
bool DASpinBoxStyle::install()
{
    QStyle* current = QApplication::style();
    if (!current) {
        return false;
    }
    const QString styleName = current->objectName();
    if (styleName.isEmpty()) {
        return false;
    }
    QStyle* base = QStyleFactory::create(styleName);
    if (!base) {
        return false;
    }
    QApplication::setStyle(new DASpinBoxStyle(base));
    return true;
}

/**
 * @brief 控件应用样式时补足 SpinBox 的最小宽度
 *
 * 窗口一旦挂有 QSS，Qt 会用 QStyleSheetStyle 计算 QSpinBox 尺寸，其结果只有
 * 「按钮宽度 + 4px 文本区」，数值区无法显示。这里按字体、数值范围、前后缀与
 * 小数位计算最小宽度，保证任何布局下数值都可见。
 *
 * @param widget 正在应用样式的控件
 */
void DASpinBoxStyle::polish(QWidget* widget)
{
    QProxyStyle::polish(widget);
    auto* spin = qobject_cast< QAbstractSpinBox* >(widget);
    if (!spin) {
        return;
    }
    if (!qobject_cast< QSpinBox* >(spin) && !qobject_cast< QDoubleSpinBox* >(spin)) {
        return;
    }
    const int minimumWidth = spinBoxMinimumWidth(spin);
    if (minimumWidth > spin->minimumWidth()) {
        spin->setMinimumWidth(minimumWidth);
    }
}

/**
 * @brief 计算数值区可见所需的最小宽度
 *
 * 取样值覆盖数值范围两端（含符号、前后缀与小数位），并至少保留三位数字的
 * 文本宽度；按钮占用宽度按基础样式的实际矩形累加（左右并排时为两个按钮之和，
 * 竖排时取单列宽度）。
 *
 * @param spin 目标 SpinBox
 * @return 最小宽度（像素）；非 QSpinBox/QDoubleSpinBox 返回 0
 */
int DASpinBoxStyle::spinBoxMinimumWidth(const QAbstractSpinBox* spin) const
{
    const QFontMetrics fm(spin->font());
    const auto widerText = [&fm](const QString& a, const QString& b) {
        return fm.horizontalAdvance(a) >= fm.horizontalAdvance(b) ? a : b;
    };

    QString sample;
    if (const auto* intSpin = qobject_cast< const QSpinBox* >(spin)) {
        sample = widerText(intSpin->prefix() + QString::number(intSpin->minimum()) + intSpin->suffix(),
                           intSpin->prefix() + QString::number(intSpin->maximum()) + intSpin->suffix());
        sample = widerText(sample, QStringLiteral("000"));
    } else if (const auto* doubleSpin = qobject_cast< const QDoubleSpinBox* >(spin)) {
        const int decimals = doubleSpin->decimals();
        sample = widerText(doubleSpin->prefix() + QString::number(doubleSpin->minimum(), 'f', decimals)
                               + doubleSpin->suffix(),
                           doubleSpin->prefix() + QString::number(doubleSpin->maximum(), 'f', decimals)
                               + doubleSpin->suffix());
        sample = widerText(sample, QStringLiteral("0.00"));
    } else {
        return 0;
    }
    // 特殊值文本（如「Disabled」）可能比数值更长
    if (!spin->specialValueText().isEmpty()) {
        sample = widerText(sample, spin->specialValueText());
    }

    QStyleOptionSpinBox opt;
    opt.initFrom(spin);
    opt.buttonSymbols = spin->buttonSymbols();
    int buttonsWidth = 0;
    if (spin->buttonSymbols() != QAbstractSpinBox::NoButtons) {
        const QRect upRect = QProxyStyle::subControlRect(CC_SpinBox, &opt, SC_SpinBoxUp, spin);
        const QRect downRect = QProxyStyle::subControlRect(CC_SpinBox, &opt, SC_SpinBoxDown, spin);
        if (upRect.isValid() && downRect.isValid()) {
            const bool horizontal = upRect.top() == downRect.top() && upRect.left() != downRect.left();
            buttonsWidth = horizontal ? upRect.width() + downRect.width()
                                      : qMax(upRect.width(), downRect.width());
        } else {
            buttonsWidth = 16;
        }
    }
    const int frameWidth = pixelMetric(PM_SpinBoxFrameWidth, &opt, spin);
    // 文本区左右各留 4px，避免数字贴边
    return fm.horizontalAdvance(sample) + buttonsWidth + 2 * frameWidth + 8;
}

}  // namespace DA
