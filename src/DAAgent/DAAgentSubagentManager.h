#ifndef DAAGENTSUBAGENTMANAGER_H
#define DAAGENTSUBAGENTMANAGER_H
#include "DAAgentAPI.h"
#include "DAAgentSubagentDef.h"
#include <QObject>
#include <QList>
#include <QString>

namespace DA
{
/**
 * @brief 子 agent 定义库管理器，负责从 <exe>/daAgent/subagents 目录
 * 加载/保存/删除子 agent 定义 md 文件（子 agent 一期，镜像 DAAgentManager）
 *
 * 由 DAAgentModule 组合持有，经 DAAgentInterface 的子 agent 管理接口暴露给
 * 上层（管理对话框 CRUD / 编辑器）。首次启动若内置定义缺失，从 qrc 资源播种
 * explore（仅文件缺失时写入，之后归用户所有）；插件可通过 registerBuiltin
 * 注入领域子 agent 定义（仅当对应文件不存在时写入，尊重用户已有编辑）。
 * 定义增删改后经 DAAgentBridge::sendUpdateSubagents 热更新子进程（Q17）。
 */
class DAAgent_API DAAgentSubagentManager : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentSubagentManager(QObject* parent = nullptr);

    /// 子 agent 定义所在目录（<exe>/daAgent/subagents）
    QString subagentsDir() const;

    /// 扫描目录加载所有 *.md，按文件名排序
    void loadSubagents();

    /// 当前加载的子 agent 定义列表
    QList<DAAgentSubagentDef> subagents() const;

    /// 按名称查找子 agent 定义，找不到返回 nullptr
    const DAAgentSubagentDef* findSubagent(const QString& name) const;

    /// 保存定义（新增或更新）；oldName 非空且与 def.name 不同时为重命名
    /// （定位旧文件交由 save 删除）；成功返回 true
    bool saveSubagent(const DAAgentSubagentDef& def, const QString& oldName = QString());

    /// 删除指定名称的定义；成功返回 true
    bool deleteSubagent(const QString& name);

    /// 内置定义播种：仅当对应文件缺失时从 qrc 写入（尊重用户编辑/删除）
    void ensureDefaultSubagents();

    /// 注册内置子 agent：仅当 <subagents>/<name>.md 不存在时写入（尊重用户编辑）
    void registerBuiltin(const QString& name, const QString& content);

    /// 内置 explore 子 agent 名称
    static const QString DEFAULT_EXPLORE_NAME;

Q_SIGNALS:
    /// 子 agent 定义列表发生变化（加载/保存/删除后发射），供 UI 刷新
    void subagentListChanged();

private:
    QList<DAAgentSubagentDef> m_subagents;
};
} // namespace DA

#endif // DAAGENTSUBAGENTMANAGER_H
