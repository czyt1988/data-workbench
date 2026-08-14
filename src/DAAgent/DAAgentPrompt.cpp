#include "DAAgentPrompt.h"
// Qt
#include <QFile>
#include <QFileInfo>
#include <QDir>

namespace DA
{
DAAgentPrompt DAAgentPrompt::loadFromFile(const QString& filePath)
{
    DAAgentPrompt agent;
    QFileInfo info(filePath);
    if (!info.exists() || info.suffix().toLower() != "md") {
        return agent;
    }
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        return agent;
    }
    agent.content  = QString::fromUtf8(f.readAll());
    agent.title    = info.completeBaseName();
    agent.filePath = info.absoluteFilePath();
    return agent;
}

bool DAAgentPrompt::save(const QString& agentDir)
{
    QString finalTitle = sanitizeTitle(title);
    if (finalTitle.isEmpty()) {
        return false;
    }
    QDir().mkpath(agentDir);
    QString finalPath = QFileInfo(agentDir, finalTitle + ".md").absoluteFilePath();
    // 如果有旧的 filePath 且与新路径不同，删除旧文件（重命名场景）
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
    f.write(content.toUtf8());
    f.close();
    title    = finalTitle;
    filePath = finalPath;
    return true;
}

bool DAAgentPrompt::remove()
{
    if (filePath.isEmpty()) {
        return false;
    }
    QFile f(filePath);
    return f.remove();
}

QString DAAgentPrompt::sanitizeTitle(const QString& title)
{
    QString t = title.trimmed();
    // 替换 Windows 文件名非法字符
    for (const QChar& c : QString("\\/:*?\"<>|")) {
        t.replace(c, '_');
    }
    return t;
}
} // namespace DA
