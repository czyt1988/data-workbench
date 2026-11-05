#ifndef DATABLEVIEWSTATECONTROLLER_H
#define DATABLEVIEWSTATECONTROLLER_H
#include "DAGuiAPI.h"
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>

class QHeaderView;

namespace DA
{
class DADataTableView;
class DATablePageContext;
class DATableSelectionService;

/**
 * @brief 视图状态控制器（008 期3）
 *
 * 视图状态子存储（列宽/行头宽/行高/表头高/列序/隐藏/冻结）到窗格的
 * 采集与投影。applyFromStore() 是唯一投影路径（006b/006e）：列宽按
 * （列名+occurrence）套用、无记录列内容自适应、行头宽设给归属方、
 * 列序/隐藏/冻结投影进窗格容器；冻结集合缓存（isColumnInFrozenArea
 * 纯查表）唯一写入时机在投影内。
 * 构造注入上下文与选区服务，不持窗口指针。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableViewStateController : public QObject
{
    Q_OBJECT
public:
    explicit DATableViewStateController(const DATablePageContext& ctx,
                                       const DATableSelectionService& sel,
                                       QObject* parent = nullptr);

public:
    // 从视图状态子存储投影到窗格（唯一投影路径：列宽→行头宽→行高/表头高→
    // 列序/隐藏/冻结进容器→末轮内容自适应）。首次调用做加载收敛
    //（孤儿剪除 + 新列追加，E6：之后不再收敛）
    void applyFromStore();
    // 目标列是否落在冻结区（纯查表，冻结集合在本类投影内唯一写入；真源唯一）
    bool isColumnInFrozenArea(int logicalColumn) const;
    // 采集用户拖拽的列宽写入子存储（程序化/布局触发的 sectionResized 不记录）；
    // source = 触发信号的那个窗格
    void onHeaderSectionResized(DADataTableView* source, int logicalIndex, int newSize);
    // 按 (名, occurrence) 写一条列宽进子存储（分界条拖动/双击自适应的公共落库段）
    void storeColumnWidthAt(int logicalColumn, int width);
    // 指定列的内容自适应宽度（表头 + 视口采样单元格文本，钳制上下限；分界条双击用）
    int contentFittedColumnWidth(DADataTableView* pane, int logicalColumn) const;
    // 默认行高套到全部窗格垂直表头（默认行高+全部可见段；整表设置与加载回填共用）
    void applyRowHeightToPanes(int height);
    // 逐行行高覆盖套到全部窗格（差异增量：只处理已应用段与 store 覆盖，不遍历窗口全部段）
    void applyRowHeightOverridesToPanes();
    // 逐行行高覆盖套到指定窗格（滑动窗搬窗/结构变化后重新落到正确的窗口段）
    void applyRowHeightOverridesToPane(DADataTableView* pane);
    // 水平表头高套到全部窗格（setFixedHeight；拖拽落库与加载回填共用）
    void applyHeaderHeightToPanes(int height);
    // 把 store 里的列宽套到指定窗格（列宽段唯一实现，主/冻结窗格共用；冻结窗格懒建回填用）
    void applyColumnWidthsToPane(DADataTableView* pane);
    // store 无记录的列做内容自适应列宽（冻结窗格懒建后回填用）
    void autoFitUnrecordedColumnWidths(DADataTableView* pane);
    // 行头宽只设给归属方（冻结时归属冻结窗格，否则主窗格）
    void applyRowHeaderWidthToOwner();

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）

    bool mApplyingViewState { false };       ///< 正在投影（屏蔽 sectionResized 采集）
    QSet< int > mFrozenColumnsLogical;  ///< 本轮投影的冻结列集合（logical），isColumnInFrozenArea 查表用
    // 每个窗格已应用的逐行行高（窗口内段号 → 高度）；差异重投影只遍历这两张表
    QHash< DADataTableView*, QHash< int, int > > mAppliedRowHeights;
};
}  // end of namespace DA
#endif  // DATABLEVIEWSTATECONTROLLER_H
