#include "DAAgentSubagentManager.h"
#include "DAAgentPrompt.h"
// Qt
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace DA
{
const QString DAAgentSubagentManager::DEFAULT_EXPLORE_NAME = QStringLiteral("explore");

DAAgentSubagentManager::DAAgentSubagentManager(QObject* parent) : QObject(parent)
{
}

QString DAAgentSubagentManager::subagentsDir() const
{
    return QDir(QApplication::applicationDirPath() + "/daAgent/subagents").absolutePath();
}

void DAAgentSubagentManager::loadSubagents()
{
    m_subagents.clear();
    QDir dir(subagentsDir());
    if (!dir.exists()) {
        emit subagentListChanged();
        return;
    }
    // 仅取顶层 md 文件，按文件名排序
    QStringList filters;
    filters << "*.md";
    const QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo& info : files) {
        DAAgentSubagentDef s = DAAgentSubagentDef::loadFromFile(info.absoluteFilePath());
        if (s.isValid()) {
            m_subagents.append(s);
        }
    }
    emit subagentListChanged();
}

QList<DAAgentSubagentDef> DAAgentSubagentManager::subagents() const
{
    return m_subagents;
}

const DAAgentSubagentDef* DAAgentSubagentManager::findSubagent(const QString& name) const
{
    for (const DAAgentSubagentDef& s : m_subagents) {
        if (s.name == name) {
            return &s;
        }
    }
    return nullptr;
}

bool DAAgentSubagentManager::saveSubagent(const DAAgentSubagentDef& def, const QString& oldName)
{
    DAAgentSubagentDef copy = def;
    // 重命名场景：按 oldName 定位旧文件路径，交由 save 删除旧文件
    if (!oldName.isEmpty() && oldName != def.name) {
        for (const DAAgentSubagentDef& s : std::as_const(m_subagents)) {
            if (s.name == oldName) {
                copy.filePath = s.filePath;
                break;
            }
        }
    }
    if (!copy.save(subagentsDir())) {
        return false;
    }
    loadSubagents();
    return true;
}

bool DAAgentSubagentManager::deleteSubagent(const QString& name)
{
    for (const DAAgentSubagentDef& s : std::as_const(m_subagents)) {
        if (s.name == name) {
            QFile f(s.filePath);
            const bool ok = f.remove();
            if (ok) {
                loadSubagents();
            }
            return ok;
        }
    }
    return false;
}

void DAAgentSubagentManager::ensureDefaultSubagents()
{
    QDir dir(subagentsDir());
    if (!dir.exists()) {
        dir.mkpath(dir.absolutePath());
    }
    // 内置 explore 定义：仅文件缺失时播种，之后归用户所有（可编辑可删除）
    const QString explorePath =
        QFileInfo(subagentsDir(), DEFAULT_EXPLORE_NAME + QStringLiteral(".md")).absoluteFilePath();
    if (QFileInfo::exists(explorePath)) {
        return;
    }
    QFile rsrc(QStringLiteral(":/da/agent/subagent-explore.md"));
    if (!rsrc.open(QIODevice::ReadOnly)) {
        qWarning() << "DAAgentSubagentManager: builtin explore subagent resource not found";
        return;
    }
    const QString content = QString::fromUtf8(rsrc.readAll());
    rsrc.close();
    QFile out(explorePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentSubagentManager: cannot write builtin explore subagent to" << explorePath;
        return;
    }
    out.write(content.toUtf8());
    out.close();
}

void DAAgentSubagentManager::registerBuiltin(const QString& name, const QString& content)
{
    if (name.isEmpty()) {
        return;
    }
    const QString safeName = DAAgentPrompt::sanitizeTitle(name);
    QDir dir(subagentsDir());
    dir.mkpath(dir.absolutePath());
    const QString path = QFileInfo(subagentsDir(), safeName + QStringLiteral(".md")).absoluteFilePath();
    // 仅当文件不存在时写入，尊重用户已有的编辑/删除
    if (QFileInfo::exists(path)) {
        return;
    }
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentSubagentManager: cannot write builtin subagent to" << path;
        return;
    }
    out.write(content.toUtf8());
    out.close();
}
} // namespace DA
