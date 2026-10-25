#ifndef DASPINBOXSTYLE_H
#define DASPINBOXSTYLE_H

#include <QProxyStyle>
#include "DAGuiAPI.h"

class QAbstractSpinBox;

namespace DA
{

/**
 * @brief 修正 QSpinBox/QDoubleSpinBox 数值区宽度的代理样式
 *
 * Windows 11 平台样式（windows11）把 QSpinBox 的上下按钮绘制成右侧并排的两个按钮；
 * 同时只要窗口设置了任意 QSS（SARibbon 主题必然会设置），Qt 就会用 QStyleSheetStyle
 * 计算 SpinBox 尺寸，把宽度压缩为「两个按钮 + 4px 文本区」，数值区无法显示。
 *
 * 本样式保留平台原生的按钮布局（Windows 11 仍是左右按钮，点击区域更大），只在控件
 * 应用样式时按字体、数值范围与基础样式的按钮宽度补足最小宽度；其余绘制与控件全部
 * 委托给基础样式。
 *
 * 应用启动时在 QApplication 构造完成后调用一次 install()。
 */
class DAGUI_API DASpinBoxStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit DASpinBoxStyle(QStyle* base = nullptr);
    ~DASpinBoxStyle() override;

    // 为当前平台样式安装代理并返回 true，样式名不可用或已安装返回 false
    static bool install();

    // 为 QSpinBox/QDoubleSpinBox 补足数值区所需的最小宽度
    void polish(QWidget* widget) override;

private:
    // 按字体、数值范围、前后缀与小数位计算数值区所需的最小宽度
    int spinBoxMinimumWidth(const QAbstractSpinBox* spin) const;
};

}  // namespace DA

#endif  // DASPINBOXSTYLE_H
