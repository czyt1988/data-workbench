#ifndef DATABLEPANECONTAINER_H
#define DATABLEPANECONTAINER_H
#include "DAGuiAPI.h"
#include <QSet>
#include <QVector>
#include <QWidget>

class QHeaderView;

namespace DA
{
class DADataTableView;

/**
 * @brief 数据表格列轴窗格容器（006 计划 WP5）
 *
 * 包装主窗格（006e 起并排冻结窗格），把页面算出的列视图状态投影到各窗格。
 * 本单只做单窗格骨架：frozenView() 恒 nullptr、freezeCount 容器内夹紧为 0
 * （冻结语义 006e 收口）。容器不持模型/委托/store——页面是唯一真源操作方。
 */
class DAGUI_API DATablePaneContainer : public QWidget
{
    Q_OBJECT
public:
    explicit DATablePaneContainer(QWidget* parent = nullptr);

    // 运行时包装：主窗格重父化到本容器布局（ui->tableView 指针不变）
    void setMainView(DADataTableView* mainView);
    DADataTableView* mainView() const;
    // 冻结窗格（本单恒 nullptr，006e 懒建）
    DADataTableView* frozenView() const;

    // 列状态投影：ordered = 全部数据列显示顺序（含隐藏列，logical 升序表）；
    // hidden = 用户隐藏集合；freezeCount 本容器内夹紧为 0（006e 起生效）
    void applyColumnState(const QVector< int >& orderedLogical, const QSet< int >& hiddenLogical, int freezeCount);
    // 生效冻结列数（本单恒 0）
    int effectiveFrozenColumnCount() const;
    // 最近一次投影的全局列序 / 隐藏集合缓存（006c 合并全局列序用）
    QVector< int > cachedOrderedLogical() const;
    QSet< int > cachedHiddenLogical() const;
    // 只读口径 getter（006d「冻结到此列」/ 006e/007 冻结区判定共用，调用点不要重算"列序剔隐藏"）
    QVector< int > orderedColumnsLogical() const;     // 数据列全序（logical，含隐藏列）
    QVector< int > visibleColumnOrderLogical() const; // 可见列序（logical，按视图顺序剔除隐藏列）

Q_SIGNALS:
    // 冻结窗格懒建完成（006e 起发；此时窗格已进布局但尚未装配，页面在槽里 configurePaneView）
    void frozenViewCreated(DA::DADataTableView* frozenView);
    // 拖动落库后的全局列序（006c 起发；页面入栈后统一重投影）
    void columnOrderChanged(const QVector< int >& newGlobalOrderLogical);

private:
    // 单窗格投影：可见性（不在显示集合里的段隐藏）+ 顺序（目标列 visualIndex 依次挪到 0,1,2…）
    void projectPane(DADataTableView* pane, const QVector< int >& paneVisibleOrdered);

private:
    DADataTableView* mMainView { nullptr };  ///< 非拥有（页面持有，本容器重父化到自己的布局）
    QVector< int > mOrderedLogical;          ///< 全局列序缓存
    QSet< int > mHiddenLogical;              ///< 隐藏集合缓存
    bool mApplyingColumnState { false };     ///< 投影守卫（006c 屏蔽 sectionMoved 回写）
};
}  // end of namespace DA
#endif  // DATABLEPANECONTAINER_H
