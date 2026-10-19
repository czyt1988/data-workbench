#ifndef DATABLEHIGHLIGHTOVERLAY_H
#define DATABLEHIGHLIGHTOVERLAY_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QHash>
#include <QSet>
#include <QVector>
#include <QPair>
#include <QColor>
#include <QtGlobal>

namespace DA
{
/**
 * @brief 表格瞬态高亮层（查找命中 / 校验违规）
 *
 * 持有一批 (row, col) 位置及其高亮级别，提供 O(1) 查询与「整层清空 / 按来源清空 /
 * 按来源+级别清空」接口，变更时发 changed() 让视图重绘。**不序列化、不进撤销栈**
 * （瞬态：只活在会话里，工程不保存）。
 *
 * 两个来源（Source::Find 查找 / Source::Validation 校验）各自管理自己的集合，互不干扰；
 * 同一来源下多个级别（如 Find 的「全部命中 Hit」+「当前命中 CurrentHit」、Validation 的
 * 「Error + Warning」）也各自独立，一次 set 只重建「该来源该级别」的集合，不覆盖别的级别。
 *
 * 存储用两级索引 QHash<Source, QHash<Level, QSet<quint64>>>，位置键打包成一个 quint64：
 * (quint64(row) << 32) | quint32(col)。**前提：行号与列号均 < 2^31**（有 21 亿行/列足够，
 * 且滑动窗下位置键用数据源绝对行号，不会溢出）。查询用 QSet::contains，O(1)。
 *
 * levelAt 返回该格当前生效的**最高优先级级别**，优先级固定为
 * CurrentHit > Hit > ViolationError > ViolationWarning（先查 Find 来源，再查 Validation 来源）。
 * 跨来源先后是有意选的：同一格既是查找命中又是校验违规时以查找为准（查找是用户当前操作焦点）。
 *
 * 四个级别 → 语义色映射集中在本类 colorForLevel()，色值取自官方色板
 * （仓库根目录 DESIGN.md），不透明度与背景混合在 delegate 层实现。
 */
class DAGUI_API DATableHighlightOverlay : public QObject
{
    Q_OBJECT
public:
    // 高亮级别（None 表示无高亮）
    enum class Level
    {
        None,
        Hit,               ///< 查找命中（全部命中集合）
        CurrentHit,        ///< 当前命中（查找导航定位的那一格）
        ViolationError,    ///< 校验违规-错误
        ViolationWarning   ///< 校验违规-警告
    };
    // 高亮来源
    enum class Source
    {
        Find,        ///< 查找替换
        Validation   ///< 数据校验
    };

    explicit DATableHighlightOverlay(QObject* parent = nullptr);

    // 设置某来源某级别的位置集合（整体替换该 source+level 的集合，O(命中数)）
    void set(Source source, Level level, const QVector< QPair< int, int > >& cells);
    // 清空某来源的全部级别（如关闭查找时清 Find，不影响 Validation）
    void clear(Source source);
    // 清空某来源的某级别（查找导航重设 CurrentHit 时用）
    void clear(Source source, Level level);
    // 清空全部来源全部级别
    void clearAll();
    // 查询该格当前生效的最高优先级级别（无命中返回 Level::None）
    Level levelAt(int row, int col) const;
    // 是否没有任何高亮（delegate 快速路径用）
    bool isEmpty() const;
    // 级别 → 语义色（None 返回无效色 QColor()）
    static QColor colorForLevel(Level level);

Q_SIGNALS:
    // 任意集合变更后发射，视图据此重绘
    void changed();

private:
    // 位置键打包：(quint64(row) << 32) | quint32(col)，前提 row/col < 2^31
    static quint64 packKey(int row, int col);
    // 某来源某级别的集合是否含该键
    bool contains(Source source, Level level, quint64 key) const;

    // 两级索引：来源 → 级别 → 位置键集合
    QHash< Source, QHash< Level, QSet< quint64 > > > mIndex;
};

// enum class 的 qHash：Qt5 未提供通用枚举 qHash，需显式给出；Qt6 已内置（避免重复定义）
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
inline uint qHash(DATableHighlightOverlay::Source source, uint seed = 0) noexcept
{
    return ::qHash(static_cast< int >(source), seed);
}
inline uint qHash(DATableHighlightOverlay::Level level, uint seed = 0) noexcept
{
    return ::qHash(static_cast< int >(level), seed);
}
#endif
}  // end of namespace DA
#endif  // DATABLEHIGHLIGHTOVERLAY_H
