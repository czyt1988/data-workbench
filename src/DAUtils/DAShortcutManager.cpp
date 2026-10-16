#include "DAShortcutManager.h"
#include "DADir.h"
#include <QAction>
#include <QPointer>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>

namespace DA
{

class DAShortcutManager::PrivateData
{
    DA_DECLARE_PUBLIC(DAShortcutManager)
public:
    PrivateData(DAShortcutManager* p);
    struct CommandInfo
    {
        QKeySequence defaultKeys;  ///< 默认快捷键（attach 时从 action 收集）
        QString category;          ///< 分类文本（设置页分组用）
        QPointer< QAction > action;
    };
    QHash< QString, CommandInfo > mCommands;    ///< objName -> 命令信息
    QHash< QString, QKeySequence > mOverrides;  ///< objName -> 覆盖键序列（键存在即已覆盖，空序列为显式清除）
    QString mConfigFilePath;                    ///< 快捷键配置文件路径
};

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// 工具函数
//===================================================

/**
 * @brief 把多绑定的 QKeySequence 拆成单绑定列表
 *
 * PortableText 序列化时多个绑定以 ", " 连接，此处按其还原
 */
static QList< QKeySequence > splitSequence(const QKeySequence& s)
{
    QList< QKeySequence > res;
    if (s.isEmpty()) {
        return res;
    }
    const QString joined = s.toString(QKeySequence::PortableText);
    const QStringList parts = joined.split(QStringLiteral(", "), Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        QKeySequence seq = QKeySequence::fromString(p, QKeySequence::PortableText);
        if (!seq.isEmpty()) {
            res.append(seq);
        }
    }
    return res;
}

/**
 * @brief 把单绑定列表合并为一个多绑定 QKeySequence
 */
static QKeySequence joinSequences(const QList< QKeySequence >& seqs)
{
    QStringList parts;
    for (const QKeySequence& s : seqs) {
        if (!s.isEmpty()) {
            parts.append(s.toString(QKeySequence::PortableText));
        }
    }
    if (parts.isEmpty()) {
        return QKeySequence();
    }
    return QKeySequence::fromString(parts.join(QStringLiteral(", ")), QKeySequence::PortableText);
}

/**
 * @brief 取按键序列的全部单绑定 PortableText 字符串（冲突比较用）
 */
static QStringList portableBindings(const QKeySequence& s)
{
    QStringList res;
    const QList< QKeySequence > parts = splitSequence(s);
    for (const QKeySequence& p : parts) {
        res.append(p.toString(QKeySequence::PortableText));
    }
    return res;
}

//===================================================
// DAShortcutManager::PrivateData
//===================================================

DAShortcutManager::PrivateData::PrivateData(DAShortcutManager* p) : q_ptr(p)
{
}

//===================================================
// DAShortcutManager
//===================================================

DAShortcutManager::DAShortcutManager(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mConfigFilePath = defaultShortcutConfigFilePath();
}

DAShortcutManager::~DAShortcutManager()
{
}

/**
 * @brief 注册命令
 *
 * action 需有全局唯一的 objectName（命令 ID）。默认快捷键取 action 当前 shortcut，
 * 因此默认值在 action 创建处就地定义；重复注册同一命令时仅更新 action 指针与分类，默认值保持首次注册值
 * @param act action
 * @param category 分类文本（设置页分组用）
 * @return 是否注册成功
 */
bool DAShortcutManager::attachAction(QAction* act, const QString& category)
{
    if (nullptr == act) {
        qWarning() << "DAShortcutManager::attachAction received a null action";
        return false;
    }
    if (act->objectName().isEmpty()) {
        qWarning() << "DAShortcutManager::attachAction received an action without objectName, text:"
                   << act->text();
        return false;
    }
    DA_D(d);
    const QString objName = act->objectName();
    auto it = d->mCommands.find(objName);
    if (it != d->mCommands.end()) {
        // 已注册：仅更新 action 指针与分类，默认值保持首次注册值
        it->action   = act;
        if (!category.isEmpty()) {
            it->category = category;
        }
        return true;
    }
    PrivateData::CommandInfo info;
    info.defaultKeys = joinSequences(act->shortcuts());
    info.category    = category;
    info.action      = act;
    d->mCommands.insert(objName, info);
    return true;
}

/**
 * @brief 批量注册命令
 * @param acts action 列表
 * @param category 分类文本
 * @return 成功注册的数量
 */
int DAShortcutManager::attachActions(const QList< QAction* >& acts, const QString& category)
{
    int count = 0;
    for (QAction* a : acts) {
        if (attachAction(a, category)) {
            ++count;
        }
    }
    return count;
}

/**
 * @brief 设置单个命令的用户覆盖
 *
 * keys 为空序列表示显式清除该命令的快捷键；本函数不写 action，调用方随后经 @ref applyAll 生效
 * @param objName 命令 ID
 * @param keys 覆盖键序列
 */
void DAShortcutManager::setOverride(const QString& objName, const QKeySequence& keys)
{
    DA_D(d);
    d->mOverrides.insert(objName, keys);
    Q_EMIT shortcutChanged(objName);
}

/**
 * @brief 整表替换用户覆盖（设置页 apply 提交快照用）
 * @param overrides objName -> 覆盖键序列（键存在即已覆盖，空序列为显式清除）
 */
void DAShortcutManager::setOverrides(const QHash< QString, QKeySequence >& overrides)
{
    DA_D(d);
    d->mOverrides = overrides;
    Q_EMIT shortcutsReset();
}

/**
 * @brief 获取当前全部用户覆盖的快照
 * @return objName -> 覆盖键序列（键存在即已覆盖，空序列为显式清除）
 */
QHash< QString, QKeySequence > DAShortcutManager::overrides() const
{
    DA_DC(d);
    return d->mOverrides;
}

/**
 * @brief 撤销单个命令的用户覆盖（恢复默认）
 * @param objName 命令 ID
 * @return 命令是否存在覆盖
 */
bool DAShortcutManager::resetToDefault(const QString& objName)
{
    DA_D(d);
    if (d->mOverrides.remove(objName) > 0) {
        Q_EMIT shortcutChanged(objName);
        return true;
    }
    return false;
}

/**
 * @brief 撤销全部用户覆盖
 */
void DAShortcutManager::resetAll()
{
    DA_D(d);
    if (!d->mOverrides.isEmpty()) {
        d->mOverrides.clear();
        Q_EMIT shortcutsReset();
    }
}

/**
 * @brief 命令是否存在
 * @param objName 命令 ID
 * @return 是否已注册
 */
bool DAShortcutManager::contains(const QString& objName) const
{
    DA_DC(d);
    return d->mCommands.contains(objName);
}

/**
 * @brief 全部命令 ID
 * @return 命令 ID 列表（QHash 顺序，调用方可自行排序）
 */
QStringList DAShortcutManager::commandNames() const
{
    DA_DC(d);
    return d->mCommands.keys();
}

/**
 * @brief 命令所属分类
 * @param objName 命令 ID
 * @return 分类文本，未注册返回空字符串
 */
QString DAShortcutManager::categoryOf(const QString& objName) const
{
    DA_DC(d);
    auto it = d->mCommands.constFind(objName);
    if (it == d->mCommands.constEnd()) {
        return QString();
    }
    return it->category;
}

/**
 * @brief 命令对应的 action
 * @param objName 命令 ID
 * @return action 指针，未注册或 action 已销毁返回 nullptr
 */
QAction* DAShortcutManager::actionOf(const QString& objName) const
{
    DA_DC(d);
    auto it = d->mCommands.constFind(objName);
    if (it == d->mCommands.constEnd()) {
        return nullptr;
    }
    return it->action;
}

/**
 * @brief 命令的默认快捷键
 * @param objName 命令 ID
 * @return 默认键序列，未注册返回空序列
 */
QKeySequence DAShortcutManager::defaultKeysOf(const QString& objName) const
{
    DA_DC(d);
    auto it = d->mCommands.constFind(objName);
    if (it == d->mCommands.constEnd()) {
        return QKeySequence();
    }
    return it->defaultKeys;
}

/**
 * @brief 命令是否有用户覆盖
 * @param objName 命令 ID
 * @return 是否已覆盖
 */
bool DAShortcutManager::hasOverride(const QString& objName) const
{
    DA_DC(d);
    return d->mOverrides.contains(objName);
}

/**
 * @brief 命令的用户覆盖快捷键
 * @param objName 命令 ID
 * @return 覆盖键序列（无覆盖返回空序列，与「显式清除」无法区分，区分用 @ref hasOverride）
 */
QKeySequence DAShortcutManager::overrideKeysOf(const QString& objName) const
{
    DA_DC(d);
    return d->mOverrides.value(objName);
}

/**
 * @brief 命令生效快捷键
 *
 * 用户覆盖（含显式清除）优先，否则默认值
 * @param objName 命令 ID
 * @return 生效键序列
 */
QKeySequence DAShortcutManager::effectiveKeysOf(const QString& objName) const
{
    DA_DC(d);
    auto it = d->mOverrides.constFind(objName);
    if (it != d->mOverrides.constEnd()) {
        return it.value();
    }
    auto cit = d->mCommands.constFind(objName);
    if (cit == d->mCommands.constEnd()) {
        return QKeySequence();
    }
    return cit->defaultKeys;
}

/**
 * @brief 命令的显示名
 *
 * 取 action 文本，去掉助记符 '&' 与换行（ribbon 两行文本），无 action 时返回命令 ID
 * @param objName 命令 ID
 * @return 显示名
 */
QString DAShortcutManager::displayNameOf(const QString& objName) const
{
    QAction* act = actionOf(objName);
    if (nullptr == act) {
        return objName;
    }
    QString name = act->text();
    if (name.isEmpty()) {
        return objName;
    }
    name.remove(QLatin1Char('&'));
    name.replace(QLatin1Char('\n'), QLatin1Char(' '));
    while (name.contains(QStringLiteral("  "))) {
        name.replace(QStringLiteral("  "), QStringLiteral(" "));
    }
    return name.trimmed();
}

/**
 * @brief 冲突检测
 *
 * 返回除 excludeObjName 外占用了相同按键序列的命令 ID。所有命令位于同一命名空间
 * （同一主窗口内 QAction 快捷键互斥），比较基于单绑定 PortableText 文本交集
 * @param excludeObjName 排除的命令 ID（通常是正在录入的命令自身）
 * @param keys 待校验的键序列
 * @return 冲突命令 ID 列表
 */
QStringList DAShortcutManager::conflictingCommands(const QString& excludeObjName, const QKeySequence& keys) const
{
    QStringList conflicts;
    if (keys.isEmpty()) {
        return conflicts;
    }
    DA_DC(d);
    const QStringList inputBindings = portableBindings(keys);
    if (inputBindings.isEmpty()) {
        return conflicts;
    }
    auto cit = d->mCommands.constBegin();
    for (; cit != d->mCommands.constEnd(); ++cit) {
        const QString& objName = cit.key();
        if (objName == excludeObjName) {
            continue;
        }
        const QKeySequence other = effectiveKeysOf(objName);
        if (other.isEmpty()) {
            continue;
        }
        const QStringList otherBindings = portableBindings(other);
        for (const QString& b : otherBindings) {
            if (inputBindings.contains(b)) {
                conflicts.append(objName);
                break;
            }
        }
    }
    return conflicts;
}

/**
 * @brief 把生效快捷键写回全部已注册 action
 */
void DAShortcutManager::applyAll()
{
    DA_D(d);
    auto it = d->mCommands.constBegin();
    for (; it != d->mCommands.constEnd(); ++it) {
        apply(it.key());
    }
}

/**
 * @brief 把单个命令的生效快捷键写回其 action
 * @param objName 命令 ID
 */
void DAShortcutManager::apply(const QString& objName)
{
    DA_D(d);
    auto it = d->mCommands.find(objName);
    if (it == d->mCommands.end()) {
        return;
    }
    QAction* act = it->action;
    if (nullptr == act) {
        return;
    }
    const QKeySequence eff = effectiveKeysOf(objName);
    if (eff.isEmpty()) {
        act->setShortcut(QKeySequence());
        return;
    }
    act->setShortcuts(splitSequence(eff));
}

/**
 * @brief 快捷键配置文件路径
 * @return 当前配置文件路径
 */
QString DAShortcutManager::shortcutConfigFilePath() const
{
    DA_DC(d);
    return d->mConfigFilePath;
}

/**
 * @brief 设置配置文件路径
 * @param filePath 新的配置文件路径
 */
void DAShortcutManager::setShortcutConfigFilePath(const QString& filePath)
{
    DA_D(d);
    d->mConfigFilePath = filePath;
}

/**
 * @brief 默认配置文件路径
 * @return 配置目录下的 shortcuts.json 绝对路径
 */
QString DAShortcutManager::defaultShortcutConfigFilePath()
{
    return DADir::getConfigPath() + QDir::separator() + QStringLiteral("shortcuts.json");
}

/**
 * @brief 从快捷键配置文件加载用户覆盖
 *
 * 文件不存在视为无覆盖（返回 false 但状态干净，不报错）；加载后整表替换当前覆盖。
 * 文件中未注册命令的条目也保留（命令可能在之后 attach）
 * @return 是否成功加载到覆盖
 */
bool DAShortcutManager::load()
{
    DA_D(d);
    QFile f(d->mConfigFilePath);
    if (!f.exists()) {
        return false;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "DAShortcutManager::load cannot open" << d->mConfigFilePath << ":"
                   << f.errorString();
        return false;
    }
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "DAShortcutManager::load cannot parse" << d->mConfigFilePath << ":"
                   << parseErr.errorString();
        return false;
    }
    const QJsonObject shortcutsObj = doc.object().value(QStringLiteral("shortcuts")).toObject();
    QHash< QString, QKeySequence > loaded;
    auto it = shortcutsObj.constBegin();
    for (; it != shortcutsObj.constEnd(); ++it) {
        // 空字符串表示显式清除（空序列），键存在即视为已覆盖
        loaded.insert(it.key(), QKeySequence::fromString(it.value().toString(), QKeySequence::PortableText));
    }
    setOverrides(loaded);
    return true;
}

/**
 * @brief 把用户覆盖写入快捷键配置文件
 *
 * 稀疏存储：只写用户改过的命令；QSaveFile 原子写
 * @return 是否写入成功
 */
bool DAShortcutManager::save()
{
    DA_D(d);
    QJsonObject shortcutsObj;
    auto it = d->mOverrides.constBegin();
    for (; it != d->mOverrides.constEnd(); ++it) {
        shortcutsObj.insert(it.key(), it.value().toString(QKeySequence::PortableText));
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("shortcuts"), shortcutsObj);
    QSaveFile f(d->mConfigFilePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAShortcutManager::save cannot open" << d->mConfigFilePath << ":"
                   << f.errorString();
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        qWarning() << "DAShortcutManager::save failed to commit" << d->mConfigFilePath;
        return false;
    }
    return true;
}

}  // end of DA
