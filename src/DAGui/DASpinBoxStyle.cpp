#include "DASpinBoxStyle.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QFontMetrics>
#include <QSpinBox>
#include <QStyleFactory>
#include <QStyleOptionSpinBox>

namespace DA
{
namespace
{
// 数值样本覆盖符号位与常见量级，保证最小宽度不随当前值跳动
const char* const kNumberSample = "-99999";
}  // namespace

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
 * 所有平台样式都需要本类：windows11 的上下按钮是左右并排（需要重排为竖排），
 * 而任何样式在窗口挂上 QSS 后都会由 QStyleSheetStyle 计算 SpinBox 尺寸，把
 * 数值区压到不足 4px（需要补最小宽度）。子控件重排只在基础样式确实左右并排时
 * 生效，其它样式保持原生矩形。
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
 * @brief 计算 SpinBox 各子控件矩形
 *
 * 基础样式的上下按钮左右并排时，把按钮列改到数值区右侧并上下平分高度，数值区
 * 取剩余宽度；按钮本身已是竖排的样式（windowsvista/Fusion 等）保持原生矩形。
 * 箭头绘制仍由基础样式完成，它会通过 proxy()->subControlRect() 取用这里的矩形。
 *
 * @param cc 复杂控件类型，仅处理 CC_SpinBox
 * @param opt 样式选项
 * @param sc 子控件
 * @param widget 目标控件
 * @return 子控件矩形
 */
QRect DASpinBoxStyle::subControlRect(ComplexControl cc,
                                     const QStyleOptionComplex* opt,
                                     SubControl sc,
                                     const QWidget* widget) const
{
    if (cc != CC_SpinBox) {
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }
    const auto* spinOpt = qstyleoption_cast< const QStyleOptionSpinBox* >(opt);
    if (!spinOpt || spinOpt->buttonSymbols == QAbstractSpinBox::NoButtons) {
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }
    // 问基础样式要按钮矩形：只有左右并排（top 相同、left 不同）才需要重排
    const QRect baseUp = QProxyStyle::subControlRect(cc, opt, SC_SpinBoxUp, widget);
    const QRect baseDown = QProxyStyle::subControlRect(cc, opt, SC_SpinBoxDown, widget);
    const bool horizontalButtons = !baseUp.isEmpty() && !baseDown.isEmpty() && baseUp.top() == baseDown.top()
                                   && baseUp.left() != baseDown.left();
    if (!horizontalButtons) {
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }

    const QRect rect = spinOpt->rect;
    const int frameWidth = pixelMetric(PM_SpinBoxFrameWidth, spinOpt, widget);
    const int buttonWidth = qMax(baseUp.width(), baseDown.width());
    const int innerHeight = rect.height() - 2 * frameWidth;
    if (buttonWidth <= 0 || innerHeight <= 0) {
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }
    const int upHeight = (innerHeight + 1) / 2;
    const bool rightToLeft = (spinOpt->direction == Qt::RightToLeft);
    const int buttonX = rightToLeft ? rect.left() + frameWidth : rect.right() - frameWidth - buttonWidth + 1;
    const int editX = rightToLeft ? rect.left() + frameWidth + buttonWidth : rect.left() + frameWidth;
    const int editWidth = qMax(0, rect.width() - 2 * frameWidth - buttonWidth);
    switch (sc) {
    case SC_SpinBoxUp:
        return QRect(buttonX, rect.top() + frameWidth, buttonWidth, upHeight);
    case SC_SpinBoxDown:
        return QRect(buttonX, rect.top() + frameWidth + upHeight, buttonWidth, innerHeight - upHeight);
    case SC_SpinBoxEditField:
        return QRect(editX, rect.top() + frameWidth, editWidth, innerHeight);
    default:
        break;
    }
    return QProxyStyle::subControlRect(cc, opt, sc, widget);
}

/**
 * @brief 控件应用样式时补足 SpinBox 的最小宽度
 *
 * 窗口一旦挂有 QSS，Qt 会用 QStyleSheetStyle 计算 QSpinBox 尺寸，其结果只有
 * 「按钮宽度 + 4px 文本区」，数值区无法显示。这里按字体、前后缀、小数位与
 * 样式给出的按钮宽度计算最小宽度，保证任何布局下数值都可见。
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
 * @param spin 目标 SpinBox
 * @return 最小宽度（像素）；非 QSpinBox/QDoubleSpinBox 返回 0
 */
int DASpinBoxStyle::spinBoxMinimumWidth(const QAbstractSpinBox* spin) const
{
    QString sample;
    if (const auto* intSpin = qobject_cast< const QSpinBox* >(spin)) {
        sample = intSpin->prefix() + QString::fromLatin1(kNumberSample) + intSpin->suffix();
    } else if (const auto* doubleSpin = qobject_cast< const QDoubleSpinBox* >(spin)) {
        sample = doubleSpin->prefix() + QString::fromLatin1(kNumberSample);
        if (doubleSpin->decimals() > 0) {
            sample += QLatin1Char('.') + QString(doubleSpin->decimals(), QLatin1Char('9'));
        }
        sample += doubleSpin->suffix();
    } else {
        return 0;
    }
    const QFontMetrics fm(spin->font());
    // 特殊值文本（如「Disabled」）可能比数值更长
    if (!spin->specialValueText().isEmpty()
        && fm.horizontalAdvance(spin->specialValueText()) > fm.horizontalAdvance(sample)) {
        sample = spin->specialValueText();
    }
    QStyleOptionSpinBox opt;
    opt.initFrom(spin);
    opt.buttonSymbols = spin->buttonSymbols();
    int buttonWidth = 0;
    if (spin->buttonSymbols() != QAbstractSpinBox::NoButtons) {
        const QRect upRect = QProxyStyle::subControlRect(CC_SpinBox, &opt, SC_SpinBoxUp, spin);
        buttonWidth = upRect.isValid() ? upRect.width() : 16;
    }
    const int frameWidth = pixelMetric(PM_SpinBoxFrameWidth, &opt, spin);
    // 文本区左右各留 4px，避免数字贴边
    return fm.horizontalAdvance(sample) + buttonWidth + 2 * frameWidth + 8;
}

}  // namespace DA
