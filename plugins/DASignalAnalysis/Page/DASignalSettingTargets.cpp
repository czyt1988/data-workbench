#include "DASignalSettingTargets.h"
// 插件
#include "Page/DASignalAnalysisView.h"

namespace DA
{
/**
 * @brief 按当前视图状态判定右侧设置面板该显示哪一页
 *
 * 判定口径与设置上下文提供者的 applySettingTarget() 一致：三维瀑布子图激活时显示 3D 设置页，
 * 其余（2D 子图 / 瀑布隐藏）显示 2D 绘图设置页；没有当前视图时两页都不该有 target。
 * @param view 当前信号视图，可为 nullptr
 * @return 设置页类型
 */
DASignalSettingPage daSignalSettingPage(const DASignalAnalysisView* view)
{
    if (nullptr == view) {
        return DASignalSettingPage::None;
    }
    return view->is3DChartActive() ? DASignalSettingPage::Chart3D : DASignalSettingPage::Chart2D;
}

/**
 * @brief 两个 figure 列表是否同构
 *
 * 顺序也参与比较：图元树 combo/树的顺序就是列表顺序，顺序变化必须重建。
 * @param a 旧列表（提供者缓存）
 * @param b 新列表（mUI->signalFigures()）
 * @return 长度、顺序与指针全部相同返回 true
 */
bool daSameFigureList(const QList< DAFigureWidget* >& a, const QList< DAFigureWidget* >& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (int i = 0; i < a.size(); ++i) {
        if (a.at(i) != b.at(i)) {
            return false;
        }
    }
    return true;
}
}  // namespace DA