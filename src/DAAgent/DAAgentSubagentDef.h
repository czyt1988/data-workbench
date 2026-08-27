#ifndef DAAGENTSUBAGENTDEF_H
#define DAAGENTSUBAGENTDEF_H
#include "DAAgentAPI.h"
#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace DA
{
/**
 * @brief 子 agent 定义数据结构，对应 <exe>/daAgent/subagents 目录下的一个
 * md 文件（YAML frontmatter + markdown 正文，子 agent 一期 Q4）
 *
 * name 为子 agent 唯一名称（snake_case，参与序列化，不翻译，与文件名一一对应）；
 * description 进入父 agent 派发工具的描述（不翻译）；tools 为工具白名单
 * （白名单 × 权限层正交，Q5）；systemPrompt 为 md 正文（子 agent 系统提示词）；
 * permissions 为预留字段——一期仅解析存留、不解释不生效，语义为"叠加在主策略
 * 之上的额外约束"（见 permission-layer.md §13 衔接契约）。
 */
struct DAAgentSubagentDef
{
    QString name;             ///< 子 agent 名称（snake_case，参与序列化不翻译）
    QString description;      ///< 描述（英文，进入父派发工具描述）
    QStringList tools;        ///< 工具白名单
    QString systemPrompt;     ///< md 正文，作为子 agent 系统提示词
    QString filePath;         ///< 文件绝对路径
    QJsonObject permissions;  ///< 预留字段：解析存留，一期不解释不生效

    /// 是否有效（name 非空）
    bool isValid() const { return !name.isEmpty(); }

    /// 协议载荷 JSON（随 init/update_subagents 下发 Python）：
    /// 仅 name/description/tools/system_prompt 四字段（母文档 §7 契约逐字一致）
    DAAgent_API QJsonObject toProtocolJson() const;
    /// 完整 JSON（接口层 CRUD 用，含 permissions 预留字段）
    DAAgent_API QJsonObject toJsonObject() const;
    /// 从 JSON 还原定义（字段缺失容忍）
    static DAAgent_API DAAgentSubagentDef fromJsonObject(const QJsonObject& obj);

    /// 解析 md + frontmatter 文本为定义（无 frontmatter 时整体作为正文）
    static DAAgent_API DAAgentSubagentDef fromMarkdown(const QString& markdown);
    /// 序列化为 md + frontmatter 文本（--- 分隔，tools 内联列表语法，
    /// permissions 原样以紧凑 JSON 保留）
    DAAgent_API QString toMarkdown() const;

    /// 从文件加载（.md），name 取 frontmatter（缺失时回退文件名去扩展名）
    static DAAgent_API DAAgentSubagentDef loadFromFile(const QString& filePath);
    /// 保存到磁盘（filePath 为空时按 subagentsDir + name.md 写入，
    /// 重命名场景删除旧文件），返回是否成功
    DAAgent_API bool save(const QString& subagentsDir);
    /// 删除磁盘上的文件，返回是否成功
    DAAgent_API bool remove();
};
} // namespace DA

#endif // DAAGENTSUBAGENTDEF_H
