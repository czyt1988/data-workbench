#ifndef DADOCKAREATABPOSITION_H
#define DADOCKAREATABPOSITION_H
#include "DAGuiAPI.h"
// ADS
namespace ads
{
class CDockAreaWidget;
class CDockManager;
}

namespace DA
{

/**
 * @brief dock 区域标签页方位调整工具（ADS 全局 TabsAtBottom flag 的局部化替代）
 *
 * ADS 的 CDockManager::TabsAtBottom 是进程级静态 flag，无法只对特定 dock 区域生效。
 * 本工具类通过布局调整实现单个 dock 区域的标签页方位控制：把标签栏从标题栏移到
 * 区域布局末尾（即内容下方），或移回标题栏。绑定到 CDockManager 后，运行期新创建
 * 的 dock 区域（追加标签页、拖拽拆分、布局恢复）会自动应用设定方位。
 */
class DAGUI_API DADockAreaTabPosition
{
public:
    // 把单个 dock 区域的标签栏移到底部（内容下方）
    static void moveTabsToBottom(ads::CDockAreaWidget* dockArea);
    // 把单个 dock 区域的标签栏恢复到顶部（标题栏内）
    static void moveTabsToTop(ads::CDockAreaWidget* dockArea);
    // 按 atBottom 设置单个 dock 区域的标签页方位
    static void applyToDockArea(ads::CDockAreaWidget* dockArea, bool atBottom);
    // 应用到 manager：重定位已有 dock 区域，之后新建的区域自动应用当前方位，可重复调用
    static void applyToDockManager(ads::CDockManager* dockManager, bool atBottom);
};
}  // namespace DA
#endif  // DADOCKAREATABPOSITION_H
