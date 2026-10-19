#include "DATableHighlightOverlay.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父对象
 */
DATableHighlightOverlay::DATableHighlightOverlay(QObject* parent) : QObject(parent)
{
}

/**
 * @brief 位置键打包
 *
 * (quint64(row) << 32) | quint32(col)。前提：行/列号 < 2^31——row 左移 32 位后
 * 与 col 的低 32 位不重叠；col 用 quint32 承接保证非负列号不串位。
 * @param row 数据源绝对行号
 * @param col 列号
 * @return 打包键
 */
quint64 DATableHighlightOverlay::packKey(int row, int col)
{
    return (static_cast< quint64 >(static_cast< quint32 >(row)) << 32) | static_cast< quint32 >(col);
}

/**
 * @brief 某来源某级别的集合是否含该键
 * @param source 来源
 * @param level 级别
 * @param key 打包键
 * @return 含返回 true
 */
bool DATableHighlightOverlay::contains(Source source, Level level, quint64 key) const
{
    auto srcIt = mIndex.constFind(source);
    if (srcIt == mIndex.constEnd()) {
        return false;
    }
    auto lvlIt = srcIt->constFind(level);
    if (lvlIt == srcIt->constEnd()) {
        return false;
    }
    return lvlIt->contains(key);
}

/**
 * @brief 设置某来源某级别的位置集合
 *
 * 整体替换该 (source, level) 的集合，只重建这一份（O(命中数)），
 * 其它来源/级别的集合不受影响（级别隔离：set(Find, Hit) 不会抹掉 set(Find, CurrentHit)）。
 * @param source 来源
 * @param level 级别
 * @param cells 位置列表 (row, col)
 */
void DATableHighlightOverlay::set(Source source, Level level, const QVector< QPair< int, int > >& cells)
{
    QSet< quint64 >& bucket = mIndex[ source ][ level ];
    bucket.clear();
    bucket.reserve(cells.size());
    for (const auto& cell : cells) {
        bucket.insert(packKey(cell.first, cell.second));
    }
    Q_EMIT changed();
}

/**
 * @brief 清空某来源的全部级别
 *
 * 来源隔离：clear(Find) 不影响 Validation 的集合。
 * @param source 来源
 */
void DATableHighlightOverlay::clear(Source source)
{
    if (mIndex.remove(source) > 0) {
        Q_EMIT changed();
    }
}

/**
 * @brief 清空某来源的某级别
 *
 * 查找导航单独重设 CurrentHit 时用（先 clear(Find, CurrentHit) 再 set）。
 * @param source 来源
 * @param level 级别
 */
void DATableHighlightOverlay::clear(Source source, Level level)
{
    auto srcIt = mIndex.find(source);
    if (srcIt == mIndex.end()) {
        return;
    }
    if (srcIt->remove(level) > 0) {
        Q_EMIT changed();
    }
    if (srcIt->isEmpty()) {
        mIndex.erase(srcIt);
    }
}

/**
 * @brief 清空全部来源全部级别
 */
void DATableHighlightOverlay::clearAll()
{
    if (mIndex.isEmpty()) {
        return;
    }
    mIndex.clear();
    Q_EMIT changed();
}

/**
 * @brief 查询该格当前生效的最高优先级级别
 *
 * 固定优先级表：CurrentHit > Hit > ViolationError > ViolationWarning。
 * 先查 Find 来源（CurrentHit 优先于 Hit），Find 无命中再查 Validation 来源
 * （Error 优先于 Warning）。最多 4 次哈希 contains，仍 O(1)，不遍历集合。
 *
 * 跨来源以查找为准是**有意决策**（查找是用户当前的操作焦点），单测钉住此语义，
 * 后续不许改成校验优先；表达力不足（错误格上同时看出被查找命中）留给二期角标/描边。
 * @param row 数据源绝对行号
 * @param col 列号
 * @return 生效级别，无命中返回 Level::None
 */
DATableHighlightOverlay::Level DATableHighlightOverlay::levelAt(int row, int col) const
{
    const quint64 key = packKey(row, col);
    if (contains(Source::Find, Level::CurrentHit, key)) {
        return Level::CurrentHit;
    }
    if (contains(Source::Find, Level::Hit, key)) {
        return Level::Hit;
    }
    if (contains(Source::Validation, Level::ViolationError, key)) {
        return Level::ViolationError;
    }
    if (contains(Source::Validation, Level::ViolationWarning, key)) {
        return Level::ViolationWarning;
    }
    return Level::None;
}

/**
 * @brief 是否没有任何高亮
 * @return 全空返回 true
 */
bool DATableHighlightOverlay::isEmpty() const
{
    for (auto srcIt = mIndex.constBegin(); srcIt != mIndex.constEnd(); ++srcIt) {
        for (auto lvlIt = srcIt->constBegin(); lvlIt != srcIt->constEnd(); ++lvlIt) {
            if (!lvlIt->isEmpty()) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 级别 → 语义色
 *
 * 色值取自官方色板（仓库根目录 DESIGN.md）：命中/警告 = 金黄 #E6C27C、
 * 当前命中 = 主蓝 #5280C1、错误 = 橙红 #CE6043。不透明度与背景混合规则在
 * delegate 层③实现（本类只给不透明语义色）。
 * @param level 级别
 * @return 语义色，None 返回无效色
 */
QColor DATableHighlightOverlay::colorForLevel(Level level)
{
    switch (level) {
    case Level::Hit:
        return QColor(0xE6, 0xC2, 0x7C);  // 金黄（Favorite Yellow）
    case Level::CurrentHit:
        return QColor(0x52, 0x80, 0xC1);  // 主蓝（Primary Blue）
    case Level::ViolationError:
        return QColor(0xCE, 0x60, 0x43);  // 橙红（Danger Red）
    case Level::ViolationWarning:
        return QColor(0xE6, 0xC2, 0x7C);  // 金黄（Favorite Yellow）
    case Level::None:
    default:
        return QColor();
    }
}

}  // end of namespace DA
