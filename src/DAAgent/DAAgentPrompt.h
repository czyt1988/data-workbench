#ifndef DAAGENTPROMPT_H
#define DAAGENTPROMPT_H
#include "DAAgentAPI.h"
#include <QString>

namespace DA
{
/**
 * @brief Agent 提示词数据结构，对应 daAgent 目录下的一个 md 文件
 *
 * title 为 agent 标题（同时作为文件名去扩展名），content 为 md 全文（提示词正文），
 * filePath 为文件绝对路径。title 与文件名一一对应，重命名 title 即重命名文件。
 */
struct DAAgentPrompt
{
    QString title;     ///< agent 标题（= 文件名去扩展名）
    QString content;   ///< md 全文，作为提示词发送给 AI
    QString filePath;  ///< 文件绝对路径

    /// 是否有效（title 非空）
    bool isValid() const { return !title.isEmpty(); }

    /// 从文件加载，title 取文件名去扩展名，content 取全文
    static DAAgent_API DAAgentPrompt loadFromFile(const QString& filePath);

    /// 保存到磁盘（filePath 为空时按 agentDir + title.md 写入），返回是否成功
    DAAgent_API bool save(const QString& agentDir);

    /// 删除磁盘上的文件，返回是否成功
    DAAgent_API bool remove();

    /// 把标题中的非法文件名字符替换为下划线
    static DAAgent_API QString sanitizeTitle(const QString& title);
};
} // namespace DA

#endif // DAAGENTPROMPT_H
