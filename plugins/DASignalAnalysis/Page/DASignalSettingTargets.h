#ifndef DASIGNALSETTINGTARGETS_H
#define DASIGNALSETTINGTARGETS_H
#include <QList>

namespace DA
{
class DASignalAnalysisView;
class DAFigureWidget;

/**
 * @brief 信号分析页右侧设置面板当前该显示哪一页
 *
 * 无当前视图（页面没有信号，或信号标签页被关闭）时是 None：2D 页 target 置空，
 * 3D 页 target 也要复位，避免继续编辑已经销毁的信号子图（D1-10）。
 */
enum class DASignalSettingPage
{
    None,
    Chart2D,
    Chart3D
};

// 按当前视图状态判定设置面板该显示哪一页（纯逻辑，供设置上下文提供者与单测共用）
DASignalSettingPage daSignalSettingPage(const DASignalAnalysisView* view);

// 两个 figure 列表是否同构（长度、顺序、指针全同）
// 图元树据此判定「列表没变、只切当前项」，不重建 combo/树视图（D1-09）
bool daSameFigureList(const QList< DAFigureWidget* >& a, const QList< DAFigureWidget* >& b);
}  // namespace DA

#endif  // DASIGNALSETTINGTARGETS_H