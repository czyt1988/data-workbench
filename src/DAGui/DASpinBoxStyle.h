#ifndef DASPINBOXSTYLE_H
#define DASPINBOXSTYLE_H

#include <QProxyStyle>
#include "DAGuiAPI.h"

class QAbstractSpinBox;

namespace DA
{

/**
 * @brief 修正 QSpinBox/QDoubleSpinBox 上下按钮布局与数值区宽度的代理样式
 *
 * Windows 11 平台样式（windows11）把 QSpinBox 的上下按钮绘制成右侧并排的两个按钮；
 * 同时只要窗口设置了任意 QSS（SARibbon 主题必然会设置），Qt 就会用 QStyleSheetStyle
 * 计算 SpinBox 尺寸，把宽度压缩为「两个按钮 + 4px 文本区」，数值区无法显示。
 *
 * 本样式只介入 CC_SpinBox：把左右并排的上下按钮重排为右侧竖排，并按字体与数值
 * 范围保证数值区的最小宽度；其余控件与绘制全部委托给基础样式。
 *
 * 应用启动时在 QApplication 构造完成后调用一次 install()。
 */
class DAGUI_API DASpinBoxStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit DASpinBoxStyle(QStyle* base = nullptr);
    ~DASpinBoxStyle() override;

    // 平台样式为 windows11 时安装本代理并返回 true，其它样式无需修正返回 false
    static bool install();

    // 重排 CC_SpinBox 的上下按钮与数值区，其它子控件委托基础样式
    QRect subControlRect(ComplexControl cc,
                         const QStyleOptionComplex* opt,
                         SubControl sc,
                         const QWidget* widget) const override;
    // 为 QSpinBox/QDoubleSpinBox 补足数值区所需的最小宽度
    void polish(QWidget* widget) override;

private:
    // 按字体、前后缀与小数位计算数值区所需的最小宽度
    int spinBoxMinimumWidth(const QAbstractSpinBox* spin) const;
};

}  // namespace DA

#endif  // DASPINBOXSTYLE_H
