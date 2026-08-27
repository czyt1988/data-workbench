#include "DAAgentSubagentDef.h"
#include "DAAgentPrompt.h"
// Qt
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QDebug>

namespace DA
{
namespace
{
/// frontmatter 分隔行（首尾 ---）
bool isFrontmatterDelimiter(const QString& line)
{
    return line.trimmed() == QStringLiteral("---");
}

/// 是否顶层 key 行（非缩进、形如 "key:"）；命中时经 key 返回键名
bool isTopLevelKeyLine(const QString& line, QString* key)
{
    if (line.isEmpty() || line.at(0).isSpace()) {
        return false;
    }
    const int idx = line.indexOf(':');
    if (idx <= 0) {
        return false;
    }
    if (key) {
        *key = line.left(idx).trimmed();
    }
    return true;
}

/// 解析内联列表语法 [a, b, c]（中括号可省略），逐项 trim 去空
QStringList parseInlineList(const QString& value)
{
    QString v = value.trimmed();
    if (v.startsWith('[') && v.endsWith(']')) {
        v = v.mid(1, v.length() - 2);
    }
    QStringList out;
    const QStringList parts = v.split(',');
    for (const QString& p : parts) {
        const QString t = p.trimmed();
        if (!t.isEmpty()) {
            out.append(t);
        }
    }
    return out;
}
} // namespace

/**
 * @brief 协议载荷 JSON（母文档 §7 契约：init/update_subagents 的 subagents 数组
 * 元素仅含 name/description/tools/system_prompt 四字段）
 * @return 协议 JSON 对象（不含 permissions 预留字段）
 */
QJsonObject DAAgentSubagentDef::toProtocolJson() const
{
    QJsonObject o;
    o[QStringLiteral("name")]          = name;
    o[QStringLiteral("description")]   = description;
    o[QStringLiteral("tools")]         = QJsonArray::fromStringList(tools);
    o[QStringLiteral("system_prompt")] = systemPrompt;
    return o;
}

/**
 * @brief 完整 JSON（接口层 CRUD 用，在协议四字段基础上附加 permissions 预留字段）
 * @return JSON 对象
 */
QJsonObject DAAgentSubagentDef::toJsonObject() const
{
    QJsonObject o = toProtocolJson();
    if (!permissions.isEmpty()) {
        o[QStringLiteral("permissions")] = permissions;
    }
    return o;
}

/**
 * @brief 从 JSON 还原定义（字段缺失容忍：缺 name 则为无效定义）
 * @param obj JSON 对象（toJsonObject 的产物或编辑对话框提交）
 * @return 定义结构
 */
DAAgentSubagentDef DAAgentSubagentDef::fromJsonObject(const QJsonObject& obj)
{
    DAAgentSubagentDef def;
    def.name        = obj.value(QStringLiteral("name")).toString().trimmed();
    def.description = obj.value(QStringLiteral("description")).toString();
    const QJsonArray arr = obj.value(QStringLiteral("tools")).toArray();
    for (const QJsonValue& v : arr) {
        const QString t = v.toString().trimmed();
        if (!t.isEmpty()) {
            def.tools.append(t);
        }
    }
    def.systemPrompt = obj.value(QStringLiteral("system_prompt")).toString();
    def.permissions  = obj.value(QStringLiteral("permissions")).toObject();
    return def;
}

/**
 * @brief 解析 md + frontmatter 文本为定义
 * @param markdown 文件全文（--- 分隔的 frontmatter + 正文）
 * @return 定义结构；无 frontmatter 时整体作为正文（name 留空，
 * 由 loadFromFile 回退文件名）
 *
 * frontmatter 支持：name/description/tools/permissions 四个键。
 * tools 支持内联 [a, b, c] 与块级 "- item" 两种列表语法；
 * permissions 支持内联紧凑 JSON 及缩进续行（拼接后按 JSON 解析），
 * 一期仅解析存留、不解释不生效（permission-layer.md §13）。
 */
DAAgentSubagentDef DAAgentSubagentDef::fromMarkdown(const QString& markdown)
{
    DAAgentSubagentDef def;
    const QStringList lines = markdown.split('\n');
    if (lines.isEmpty() || !isFrontmatterDelimiter(lines.first())) {
        // 无 frontmatter：整体作为正文
        def.systemPrompt = markdown;
        return def;
    }
    // 寻找闭合分隔行
    int closing = -1;
    for (int i = 1; i < lines.size(); ++i) {
        if (isFrontmatterDelimiter(lines.at(i))) {
            closing = i;
            break;
        }
    }
    if (closing < 0) {
        // 未闭合：降级整体作为正文，避免把正文误判成 frontmatter
        def.systemPrompt = markdown;
        return def;
    }
    // 逐行解析 frontmatter
    QString permissionsRaw;
    bool inPermissions = false;
    for (int i = 1; i < closing; ++i) {
        const QString line  = lines.at(i);
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }
        QString key;
        if (!isTopLevelKeyLine(line, &key)) {
            // 缩进行：permissions 的续行内容（原样拼接，稍后整体解析）
            if (inPermissions) {
                permissionsRaw += trimmed;
            }
            continue;
        }
        inPermissions   = false;
        const QString value = line.mid(line.indexOf(':') + 1).trimmed();
        if (key == QStringLiteral("name")) {
            def.name = value;
        } else if (key == QStringLiteral("description")) {
            def.description = value;
        } else if (key == QStringLiteral("tools")) {
            if (!value.isEmpty()) {
                def.tools = parseInlineList(value);
            } else {
                // 块级列表：收集后续 "- item" 行，直到遇到新的顶层键
                for (int j = i + 1; j < closing; ++j) {
                    const QString t = lines.at(j).trimmed();
                    if (t.startsWith(QStringLiteral("- "))) {
                        const QString item = t.mid(2).trimmed();
                        if (!item.isEmpty()) {
                            def.tools.append(item);
                        }
                    } else if (!t.isEmpty()) {
                        i = j - 1;  // 外层循环从该键行继续
                        break;
                    }
                }
            }
        } else if (key == QStringLiteral("permissions")) {
            permissionsRaw = value;
            inPermissions  = true;
        }
    }
    // permissions：原样内容按 JSON 解析存留（解析失败仅告警丢弃，不影响其余字段）
    if (!permissionsRaw.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(permissionsRaw.toUtf8());
        if (doc.isObject()) {
            def.permissions = doc.object();
        } else {
            qWarning() << "DAAgentSubagentDef::fromMarkdown: permissions frontmatter is not a JSON object, dropped:"
                       << permissionsRaw;
        }
    }
    // 正文：闭合分隔行之后的全部行原样拼接
    def.systemPrompt = lines.mid(closing + 1).join('\n');
    return def;
}

/**
 * @brief 序列化为 md + frontmatter 文本
 * @return --- 分隔的 frontmatter + 正文；permissions 非空时以紧凑 JSON 原样保留
 *
 * description 中的换行替换为空格（frontmatter 值必须单行）。
 */
QString DAAgentSubagentDef::toMarkdown() const
{
    QStringList fm;
    fm << QStringLiteral("---");
    fm << QStringLiteral("name: %1").arg(name.trimmed());
    QString desc = description;
    desc.replace('\n', ' ');
    fm << QStringLiteral("description: %1").arg(desc.trimmed());
    fm << QStringLiteral("tools: [%1]").arg(tools.join(QStringLiteral(", ")));
    if (!permissions.isEmpty()) {
        fm << QStringLiteral("permissions: %1")
                  .arg(QString::fromUtf8(QJsonDocument(permissions).toJson(QJsonDocument::Compact)));
    }
    fm << QStringLiteral("---");
    return fm.join('\n') + '\n' + systemPrompt;
}

/**
 * @brief 从文件加载定义
 * @param filePath md 文件路径
 * @return 定义结构；文件不存在/非 .md/不可读时为无效定义
 */
DAAgentSubagentDef DAAgentSubagentDef::loadFromFile(const QString& filePath)
{
    DAAgentSubagentDef def;
    QFileInfo info(filePath);
    if (!info.exists() || info.suffix().toLower() != QStringLiteral("md")) {
        return def;
    }
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        return def;
    }
    def          = fromMarkdown(QString::fromUtf8(f.readAll()));
    def.filePath = info.absoluteFilePath();
    if (def.name.isEmpty()) {
        // frontmatter 无 name 时回退文件名去扩展名（与 DAAgentPrompt 惯例一致）
        def.name = info.completeBaseName();
    }
    return def;
}

/**
 * @brief 保存定义到磁盘（文件名 = 清洗后的 name + .md）
 * @param subagentsDir 子 agent 定义目录
 * @return 保存成功返回 true
 *
 * 命名清洗复用 DAAgentPrompt::sanitizeTitle 同款规则（替换文件名非法字符）。
 * 重命名场景：旧 filePath 与新路径不同时删除旧文件。
 */
bool DAAgentSubagentDef::save(const QString& subagentsDir)
{
    const QString finalName = DAAgentPrompt::sanitizeTitle(name);
    if (finalName.isEmpty()) {
        return false;
    }
    name = finalName;
    QDir().mkpath(subagentsDir);
    const QString finalPath = QFileInfo(subagentsDir, finalName + QStringLiteral(".md")).absoluteFilePath();
    if (!filePath.isEmpty() && QFileInfo(filePath).absoluteFilePath() != finalPath) {
        QFile oldFile(filePath);
        if (oldFile.exists()) {
            oldFile.remove();
        }
    }
    QFile f(finalPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    f.write(toMarkdown().toUtf8());
    f.close();
    filePath = finalPath;
    return true;
}

/**
 * @brief 删除磁盘上的定义文件
 * @return 删除成功返回 true
 */
bool DAAgentSubagentDef::remove()
{
    if (filePath.isEmpty()) {
        return false;
    }
    QFile f(filePath);
    return f.remove();
}
} // namespace DA
