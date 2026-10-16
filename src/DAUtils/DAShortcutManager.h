#ifndef DASHORTCUTMANAGER_H
#define DASHORTCUTMANAGER_H
#include "DAUtilsAPI.h"
#include "DAGlobals.h"
#include <QObject>
#include <QKeySequence>
#include <QHash>
#include <QStringList>
class QAction;

namespace DA
{
/**
 * @brief 快捷键管理器
 *
 * 统一管理程序所有可配置快捷键的命令注册、用户覆盖、冲突检测与持久化：
 * - 命令 ID = QAction 的 objectName（主程序侧契约常量见 DAUiObjectNames.h），要求全局唯一
 * - 默认快捷键在 action 创建处就地设置，attachAction 时自动收集 action 当前的 shortcut 作为默认值，
 *   因此不配置文件时程序行为即为默认快捷键
 * - 用户覆盖稀疏存储于 shortcuts.json（配置目录，原子写）：只记录用户改过的命令，空字符串表示显式清除；
 *   未出现过的命令使用默认值，版本升级新增默认快捷键不受旧配置影响
 * - 存储一律使用 PortableText，界面显示由调用方转 NativeText
 *
 * 冲突检测说明：所有命令位于同一命名空间（同一主窗口内的 QAction 快捷键互斥），
 * conflictingCommands 用于录入前校验
 */
class DAUTILS_API DAShortcutManager : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAShortcutManager)
public:
    DAShortcutManager(QObject* parent = nullptr);
    ~DAShortcutManager() override;
    // 注册命令（action 必须有唯一 objectName），默认快捷键取 action 当前 shortcut；返回是否注册成功
    bool attachAction(QAction* act, const QString& category = QString());
    // 批量注册，返回成功注册的数量
    int attachActions(const QList< QAction* >& acts, const QString& category = QString());
    // 用户覆盖：keys 为空序列表示显式清除该命令的快捷键
    void setOverride(const QString& objName, const QKeySequence& keys);
    // 整表替换用户覆盖（设置页 apply 提交快照用）：键存在即视为已覆盖，空序列为显式清除
    void setOverrides(const QHash< QString, QKeySequence >& overrides);
    // 获取当前全部用户覆盖的快照
    QHash< QString, QKeySequence > overrides() const;
    // 撤销单个命令的用户覆盖（恢复默认），返回命令是否存在
    bool resetToDefault(const QString& objName);
    // 撤销全部用户覆盖
    void resetAll();
    // 命令是否存在
    bool contains(const QString& objName) const;
    // 全部命令 ID（注册顺序）
    QStringList commandNames() const;
    // 命令所属分类（attach 时传入的文本）
    QString categoryOf(const QString& objName) const;
    // 命令对应的 action，未注册或 action 已销毁返回 nullptr
    QAction* actionOf(const QString& objName) const;
    // 命令的默认快捷键
    QKeySequence defaultKeysOf(const QString& objName) const;
    // 命令是否有用户覆盖
    bool hasOverride(const QString& objName) const;
    // 命令的用户覆盖快捷键（无覆盖返回空序列）
    QKeySequence overrideKeysOf(const QString& objName) const;
    // 命令生效快捷键 = 用户覆盖（含显式清除）优先，否则默认值
    QKeySequence effectiveKeysOf(const QString& objName) const;
    // 命令的显示名（action 文本去掉助记符与换行），无 action 时返回命令 ID
    QString displayNameOf(const QString& objName) const;
    // 冲突检测：返回除 excludeObjName 外占用了相同按键序列的命令 ID
    QStringList conflictingCommands(const QString& excludeObjName, const QKeySequence& keys) const;
    // 把生效快捷键写回全部已注册 action
    void applyAll();
    // 把单个命令的生效快捷键写回其 action
    void apply(const QString& objName);
    // 快捷键配置文件路径
    QString shortcutConfigFilePath() const;
    // 设置配置文件路径（默认为配置目录下 shortcuts.json，测试可注入临时路径）
    void setShortcutConfigFilePath(const QString& filePath);
    // 默认配置文件路径（配置目录下 shortcuts.json）
    static QString defaultShortcutConfigFilePath();

public Q_SLOTS:
    // 从快捷键配置文件加载用户覆盖（整表替换）
    bool load();
    // 把用户覆盖写入快捷键配置文件（原子写）
    bool save();

Q_SIGNALS:
    /**
     * @brief 单个命令的用户覆盖变化
     */
    void shortcutChanged(const QString& objName);
    /**
     * @brief 全部用户覆盖被重置或整表替换
     */
    void shortcutsReset();
};
}
#endif  // DASHORTCUTMANAGER_H
