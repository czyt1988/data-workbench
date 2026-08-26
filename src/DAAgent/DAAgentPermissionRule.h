// DAAgentPermissionRule.h
#pragma once
#include "DAAgentAPI.h"
#include <QString>
#include <QHash>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDir>

namespace DA
{
/**
 * @brief 权限策略条目：{tool, scope, action}（母文档 §6.1）
 *
 * tool 为具体工具名或 "*"（匹配全部工具）；scope 为 glob 路径范围，可含
 * ${workspace}/${project}/${data}/${exe}/${home} 变量，空串表示全局匹配；
 * action 为 allow / deny / ask。glob 匹配大小写不敏感（Windows 路径语义）。
 *
 * 头文件内联实现（轻量值类型，无 .cpp）。
 */
struct DAAgent_API DAAgentPermissionRule
{
    QString tool;    ///< 工具名，"*" 匹配所有工具
    QString scope;   ///< glob 路径范围（可含 ${var} 变量），空=全局
    QString action;  ///< allow / deny / ask

    // 判断工具名是否命中本条规则（"*" 匹配一切）
    bool matchesTool(const QString& toolName) const
    {
        return (tool == QStringLiteral("*")) || (tool == toolName);
    }

    // 解析 ${var} 变量，未提供的变量替换为空串
    static QString resolveVariables(const QString& pattern, const QHash< QString, QString >& vars)
    {
        QString out = pattern;
        for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
            out.replace(QStringLiteral("${") + it.key() + QStringLiteral("}"), it.value());
        }
        // 未解析的残留变量视为空（避免把字面量 ${xxx} 编进正则）
        static const QRegularExpression reResidual(QStringLiteral("\\$\\{[^}]*\\}"));
        out.remove(reResidual);
        return out;
    }

    // glob → 预编译正则：** 跨分隔符、* 单段内、? 单字符；大小写不敏感
    static QRegularExpression globToRegex(const QString& resolvedGlob)
    {
        QString re;
        re.reserve(resolvedGlob.size() * 2 + 8);
        const int n = resolvedGlob.size();
        for (int i = 0; i < n; ++i) {
            const QChar ch = resolvedGlob.at(i);
            if (ch == QLatin1Char('*')) {
                if (i + 1 < n && resolvedGlob.at(i + 1) == QLatin1Char('*')) {
                    re += QStringLiteral(".*");
                    ++i;
                    // "**/" 与 "**" 等价处理为 .*
                    if (i + 1 < n && resolvedGlob.at(i + 1) == QLatin1Char('/')) {
                        ++i;
                        re += QStringLiteral("/?");
                    }
                } else {
                    re += QStringLiteral("[^/]*");
                }
            } else if (ch == QLatin1Char('?')) {
                re += QStringLiteral("[^/]");
            } else {
                re += QRegularExpression::escape(QString(ch));
            }
        }
        return QRegularExpression(QStringLiteral("^") + re + QStringLiteral("$"),
                                  QRegularExpression::CaseInsensitiveOption);
    }

    // 规范化路径：正斜杠、去重、绝对化（不改变大小写，匹配时由正则忽略大小写）
    static QString normalizePath(const QString& path)
    {
        if (path.trimmed().isEmpty()) {
            return QString();
        }
        QString p = QDir::fromNativeSeparators(path);
        p = QDir::cleanPath(p);
        return p;
    }

    // 判断规范化绝对路径是否命中本条规则的 scope（空 scope=全局命中）
    bool matchesPath(const QString& normalizedAbsPath, const QHash< QString, QString >& vars) const
    {
        if (scope.trimmed().isEmpty()) {
            return true;
        }
        const QString resolved = normalizePath(resolveVariables(scope, vars));
        if (resolved.isEmpty()) {
            return false;  // 变量未注入（如工程未保存时 ${workspace} 为空）→ 不命中
        }
        const QRegularExpression re = globToRegex(resolved);
        return re.match(normalizedAbsPath).hasMatch();
    }

    // 序列化为 JSON（agent-permissions.json 的 rules 数组元素）
    QJsonObject toJson() const
    {
        return QJsonObject{{QStringLiteral("tool"), tool},
                           {QStringLiteral("scope"), scope},
                           {QStringLiteral("action"), action}};
    }

    // 从 JSON 反序列化
    static DAAgentPermissionRule fromJson(const QJsonObject& obj)
    {
        DAAgentPermissionRule r;
        r.tool   = obj.value(QStringLiteral("tool")).toString();
        r.scope  = obj.value(QStringLiteral("scope")).toString();
        r.action = obj.value(QStringLiteral("action")).toString().toLower();
        return r;
    }

    // 判重（硬 deny 回填用）
    bool equals(const DAAgentPermissionRule& other) const
    {
        return tool == other.tool && scope == other.scope
               && action.compare(other.action, Qt::CaseInsensitive) == 0;
    }
};
} // namespace DA
