#ifndef DARECENTFILESMANAGER_H
#define DARECENTFILESMANAGER_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QStringList>
class QActionGroup;
class QMenu;
namespace DA
{

/**
 * @brief 管理“最近打开文件”列表，支持自动过滤失效路径、菜单集成
 */
class DAGUI_API DARecentFilesManager : public QObject
{
    Q_OBJECT
public:
    explicit DARecentFilesManager(QObject* parent        = nullptr,
                                  int maxEntries         = 8,
                                  const QString& orgName = QString(),
                                  const QString& appName = QString());

    //! 把“最近文件”子菜单挂到任意 QMenu
    void attachToMenu(QMenu* parentMenu, const QString& title = tr("Recent files"));

    //! 成功打开/保存文件后加入列表
    void addFile(const QString& filePath);

    //! 清空所有记录
    void clear();

    //! 重新扫描：删除不存在文件
    void rescan();

Q_SIGNALS:

    /**
     * @brief 用户从菜单选中某文件
     * @param filePath
     */
    void fileSelected(const QString& filePath);

private:
    void rebuildMenu();                              //!< 重建菜单
    void readAndCleanSettings();                     //!< 首次读取并清理
    void writeSettings();                            //!< 保存到注册表/ini
    bool fileExists(const QString& filePath) const;  //!< 判断文件是否存在

    QActionGroup* m_group { nullptr };
    QMenu* m_recentMenu { nullptr };
    QStringList m_files;
    int m_max;
    QString m_org;
    QString m_app;
};
}  // end DA

#endif  // DARECENTFILESMANAGER_H
