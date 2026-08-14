#include "DAAgentManager.h"
// Qt
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace DA
{
const QString DAAgentManager::DEFAULT_AGENT_TITLE = QStringLiteral("数据分析助手");

DAAgentManager::DAAgentManager(QObject* parent) : QObject(parent)
{
}

QString DAAgentManager::agentDir() const
{
    return QDir(QApplication::applicationDirPath() + "/daAgent").absolutePath();
}

void DAAgentManager::loadAgents()
{
    m_agents.clear();
    QDir dir(agentDir());
    if (!dir.exists()) {
        return;
    }
    // 仅取顶层 md 文件，按文件名排序
    QStringList filters;
    filters << "*.md";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo& info : files) {
        DAAgentPrompt a = DAAgentPrompt::loadFromFile(info.absoluteFilePath());
        if (a.isValid()) {
            m_agents.append(a);
        }
    }
    emit agentListChanged();
}

QList<DAAgentPrompt> DAAgentManager::agents() const
{
    return m_agents;
}

const DAAgentPrompt* DAAgentManager::findAgent(const QString& title) const
{
    for (const DAAgentPrompt& a : m_agents) {
        if (a.title == title) {
            return &a;
        }
    }
    return nullptr;
}

QList<DAAgentPrompt> DAAgentManager::agentPrompts() const
{
    return m_agents;
}

bool DAAgentManager::saveAgent(const QString& title, const QString& content, const QString& oldTitle)
{
    DAAgentPrompt agent;
    agent.title   = title;
    agent.content = content;
    // 定位旧文件路径（重命名场景）
    if (!oldTitle.isEmpty()) {
        for (const DAAgentPrompt& a : std::as_const(m_agents)) {
            if (a.title == oldTitle) {
                agent.filePath = a.filePath;
                break;
            }
        }
    }
    if (!agent.save(agentDir())) {
        return false;
    }
    loadAgents();
    return true;
}

bool DAAgentManager::deleteAgent(const QString& title)
{
    for (const DAAgentPrompt& a : std::as_const(m_agents)) {
        if (a.title == title) {
            QFile f(a.filePath);
            bool ok = f.remove();
            if (ok) {
                loadAgents();
            }
            return ok;
        }
    }
    return false;
}

void DAAgentManager::ensureDefaultAgent()
{
    QDir dir(agentDir());
    if (!dir.exists()) {
        dir.mkpath(agentDir());
    }
    // 若目录已存在任何 md，则不播种通用默认（尊重用户已有内容）
    QStringList filters;
    filters << "*.md";
    if (!dir.entryInfoList(filters, QDir::Files).isEmpty()) {
        return;
    }
    // 从 qrc 资源读取通用默认 agent 内容
    QFile rsrc(":/da/agent/default-agent.md");
    if (!rsrc.open(QIODevice::ReadOnly)) {
        qWarning() << "DAAgentManager: default agent resource not found";
        return;
    }
    QString content = QString::fromUtf8(rsrc.readAll());
    rsrc.close();
    // 写入 daAgent/数据分析助手.md
    QString defaultPath = QFileInfo(agentDir(), DEFAULT_AGENT_TITLE + ".md").absoluteFilePath();
    QFile out(defaultPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentManager: cannot write default agent to" << defaultPath;
        return;
    }
    out.write(content.toUtf8());
    out.close();
}

void DAAgentManager::registerBuiltin(const QString& name, const QString& content)
{
    if (name.isEmpty()) {
        return;
    }
    QString safeName = DAAgentPrompt::sanitizeTitle(name);
    QDir dir(agentDir());
    dir.mkpath(agentDir());
    QString path = QFileInfo(agentDir(), safeName + ".md").absoluteFilePath();
    // 仅当文件不存在时写入，尊重用户已有的编辑/删除
    if (QFileInfo::exists(path)) {
        return;
    }
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentManager: cannot write builtin agent to" << path;
        return;
    }
    out.write(content.toUtf8());
    out.close();
}
} // namespace DA
