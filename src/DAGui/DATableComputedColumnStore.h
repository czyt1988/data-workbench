#ifndef DATABLECOMPUTEDCOLUMNSTORE_H
#define DATABLECOMPUTEDCOLUMNSTORE_H
#include "DAGuiAPI.h"
#include "DAComputedColumn.h"
#include <QObject>
#include <QList>
#include <QStringList>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 计算列子存储（元数据束成员，004 T1.23）
 *
 * 职责（设计稿 §3.1/§4.4）：
 * - 定义集合管理：DAComputedColumn 列表（按目标列名键控，保持插入顺序）；
 *   定义类在 src/DAData（语义类，无 GUI 依赖，D3 归属分层），本存储负责集合、
 *   状态与序列化；
 * - 三态状态机（R8/R9）：Valid（有效）/ Stale（待重算）/ Invalid（失效），
 *   迁移规则照设计稿 §4.4 状态图：
 *     创建（立即求值入栈）→ Valid；
 *     依赖列（或目标列自身）数据变更 → Stale（markStaleForChangedColumns）；
 *     依赖列被删除 / 表达式不可求值 → Invalid（不静默清空列数据）；
 *     重算成功 → Valid；
 * - 重算默认手动（R8）：本类只做状态标记，不触发任何求值——求值由页面 widget
 *   在用户点「重新计算」时经命令入栈执行（D4 铁律）；
 * - 列引用按列名（R20 语义类）：renameColumnReferences 批量迁移（依赖列与目标列
 *   自身），onColumnsRemoved 处理删列联动（目标列被删 → 定义移除；依赖列被删 →
 *   置失效）；
 * - XML 段：<computed-column> 进 DATableMetadata 束聚合（table-metadata.xml），
 *   expr + engine + version 一起持久化（磁盘契约 R10）；状态一并持久化——打开
 *   工程不重算（R7），加载只恢复定义与状态，不调用求值。
 *
 * 本类不入撤销栈：定义/状态的撤销联动由命令的 directional 回调驱动
 * （snapshot/restore 提供整店快照），与 DATableStyleManager 的定位一致。
 */
class DAGUI_API DATableComputedColumnStore : public QObject
{
    Q_OBJECT
public:
    // 三态（R9：表头 ƒx 角标状态色一一对应）
    enum class State
    {
        Valid,    ///< 有效（列值与公式一致）——主蓝 #5280C1
        Stale,    ///< 待重算（依赖数据已变更）——金黄 #E6C27C
        Invalid   ///< 失效（依赖列缺失/表达式不可求值）——橙红 #CE6043
    };
    Q_ENUM(State)

    // 定义 + 状态（集合元素；快照/恢复与遍历用）
    struct Entry
    {
        DAComputedColumn def;
        State state { State::Valid };
    };

    explicit DATableComputedColumnStore(QObject* parent = nullptr);

    // ===== 定义集合（键 = 目标列名，保持插入顺序）=====
    bool isEmpty() const;
    int count() const;
    QStringList targetColumns() const;
    bool hasDefinition(const QString& target) const;
    // 取定义（无定义返回默认构造的无效定义，isValid()==false）
    DAComputedColumn definition(const QString& target) const;
    // 取完整条目（定义+状态），无定义返回 false
    bool entryOf(const QString& target, Entry& out) const;
    QList< Entry > entries() const;
    // 取状态（无定义返回 Invalid）
    State stateOf(const QString& target) const;

    // ===== 增删改（widget/命令回调调用；发 definitionsChanged）=====
    void addDefinition(const DAComputedColumn& def, State state = State::Valid);
    void removeDefinition(const QString& target);
    void updateDefinition(const DAComputedColumn& def);  ///< 编辑公式（状态不变，调用方另行 setState）
    void setState(const QString& target, State state);   ///< 发 stateChanged

    // ===== 依赖联动（R20 列名引用）=====
    // 引用了 columnName 作依赖列的定义目标（不含目标列自身就是 columnName 的定义）
    QStringList definitionsReferencing(const QString& columnName) const;
    // 数据变更标记：依赖列或目标列自身被改 → Valid 转 Stale（Stale/Invalid 不降级）；
    // 返回本次被标记的目标列（排序不触发本函数，R13）
    QStringList markStaleForChangedColumns(const QStringList& changedColumns);
    // 整表操作拿不到列信息时保守全标（Valid → Stale），返回被标记的目标列
    QStringList markAllStale();
    // 列改名批量迁移：依赖列引用与目标列自身一并改名，返回受影响的定义个数
    int renameColumnReferences(const QString& oldName, const QString& newName);
    // 删列联动：目标列被删 → 定义移除（removedTargets 输出，供命令 undo 恢复）；
    // 依赖列被删 → 置 Invalid（invalidatedTargets 输出，供提示，不静默清空数据）
    void onColumnsRemoved(const QStringList& removedNames, QStringList* removedTargets = nullptr, QStringList* invalidatedTargets = nullptr);

    // ===== 整店快照/恢复（列删除等命令的 undo 联动）=====
    QList< Entry > snapshot() const;
    void restore(const QList< Entry >& entries);

    // ===== 序列化（<computed-column> 段，磁盘契约 R10：expr+engine+version+state）=====
    void toXml(QDomDocument& doc, QDomElement& parent) const;
    bool fromXml(const QDomElement& e);
    void clear();

    // 状态显示文本（表头 tooltip / 提示用，已 tr）
    static QString stateText(State state);

Q_SIGNALS:
    // 定义增删改/迁移/恢复（表头重绘、菜单构建、tooltip 更新）
    void definitionsChanged();
    // 单个定义状态迁移（表头角标变色）
    void stateChanged(const QString& target, DA::DATableComputedColumnStore::State state);

private:
    // 按目标列名找条目下标（-1 无）
    int indexOfTarget(const QString& target) const;

    QList< Entry > mEntries;  ///< 定义集合（插入顺序，round-trip 稳定）
};
}  // end of namespace DA
#endif  // DATABLECOMPUTEDCOLUMNSTORE_H
