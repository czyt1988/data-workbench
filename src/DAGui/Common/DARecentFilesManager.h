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
 * @brief 管理"最近打开文件"列表，支持自动过滤失效路径、菜单集成
 */
class DAGUI_API DARecentFilesManager : public QObject
{
	Q_OBJECT
public:
	explicit DARecentFilesManager(QObject* parent        = nullptr,
								  int maxEntries         = 8,
								  const QString& orgName = QString(),
								  const QString& appName = QString());

	// 把"最近文件"子菜单挂到任意 QMenu
	void attachToMenu(QMenu* parentMenu, const QString& title = tr("Recent files"));  // cn:最近打开文件

	// 成功打开/保存文件后加入列表
	void addFile(const QString& filePath);

	// 清空所有记录
	void clear();

	// 重新扫描：删除不存在文件
	void rescan();

    // 设置最大条目数，运行期可调用，会立即裁剪超出的记录并刷新菜单
    void setMaxEntries(int max);

    // 获取最大条目数
    int getMaxEntries() const;

    // 获取最近文件列表（最新在前；可能包含已失效的路径，由展示方决定失效条目的呈现方式）
    QStringList getRecentFiles() const;

    // 选中某个文件：文件存在时发射 fileSelected 信号并返回 true；文件不存在时把该路径从列表移除并返回 false
    bool selectFile(const QString& filePath);

Q_SIGNALS:

    /**
     * @brief 用户从菜单选中某文件
     * @param filePath
     */
    void fileSelected(const QString& filePath);

    /**
     * @brief 最近文件列表发生变化（新增/移除/清空/失效清理/条目数裁剪）后发射
     */
    void recentFilesChanged();

private:
	void rebuildMenu();                              // 重建菜单
	void readAndCleanSettings();                     // 首次读取并清理
	void writeSettings();                            // 保存到注册表/ini
	bool fileExists(const QString& filePath) const;  // 判断文件是否存在

	QActionGroup* mGroup { nullptr };
	QMenu* mRecentMenu { nullptr };
	QStringList mFiles;
	int mMax;
	QString mOrg;
	QString mApp;
};

}  // end DA

#endif  // DARECENTFILESMANAGER_H
