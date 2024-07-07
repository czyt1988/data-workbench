#include "DAPluginManager.h"
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include "DAPluginOption.h"
#include <QLibrary>
#include <QFile>
#include <QTextStream>
namespace DA
{
class DAPluginManager::PrivateData
{
	DA_DECLARE_PUBLIC(DAPluginManager)
public:
	PrivateData(DAPluginManager* p);
	// 判断是否有ignore文件
	bool hasIgnoreFile() const;
	// 更新忽略set
	void updateIgnoreSet();

public:
	QDir mPluginDir;
	QList< DAPluginOption > mPluginOptions;
	bool mIsLoaded { false };               ///< 标记是否加载了，可以只加载一次
	QSet< QString > mIgnorePluginBaseName;  ///< 记录忽略插件的基本名字
};

//===================================================
// DAPluginManagerPrivate
//===================================================

DAPluginManager::PrivateData::PrivateData(DAPluginManager* p) : q_ptr(p)
{
	mPluginDir.setPath(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/plugins"));
	updateIgnoreSet();
}

bool DAPluginManager::PrivateData::hasIgnoreFile() const
{
	return mPluginDir.exists(".pluginignore");
}

void DAPluginManager::PrivateData::updateIgnoreSet()
{
	if (!hasIgnoreFile()) {
		return;
	}
	QFile ignoreFile(mPluginDir.absoluteFilePath(".pluginignore"));
	if (!ignoreFile.exists()) {
		// 不存在，则创建一个
		if (ignoreFile.open(QIODevice::ReadWrite)) {
			QTextStream txt(&ignoreFile);
#if QT_VERSION_MAJOR >= 6
			txt.setEncoding(QStringConverter::Utf8);
#else
			txt.setCodec("utf-8");
#endif
			txt << "# pluginignore file,Plugins that you do not want to load are described in this file,only write the "
				   "file base name, do not need to write suffixes"
#if QT_VERSION_MAJOR >= 6
				<< Qt::endl;
#else
				<< endl;
#endif
			txt << "# 不想加载的插件在此文件描述，写入基本文件名，无需后缀"
#if QT_VERSION_MAJOR >= 6
				<< Qt::endl;
#else
				<< endl;
#endif
		}
		return;
	}
	if (!ignoreFile.open(QIODevice::ReadOnly)) {
		return;
	}
	QTextStream ss(&ignoreFile);
	while (!ss.atEnd()) {
		QString line = ss.readLine();
		line         = line.trimmed();
		if (line.isEmpty()) {
			continue;
		}
		if (line.at(0) == '#') {
			continue;
		}
		mIgnorePluginBaseName.insert(line.toLower());
	}
}

//===================================================
// DAPluginManager
//===================================================

DAPluginManager::DAPluginManager(QObject* p) : QObject(p), DA_PIMPL_CONSTRUCT
{
}

DAPluginManager::~DAPluginManager()
{
}

DAPluginManager& DAPluginManager::instance()
{
	static DAPluginManager s_plugin_mgr;

	return (s_plugin_mgr);
}

/**
 * @brief FCPluginManager::setIgnoreList
 * @param ignorePluginsName
 * @todo 还未实现
 */
void DAPluginManager::setIgnoreList(const QStringList ignorePluginsName)
{
    d_ptr->mIgnorePluginBaseName = QSet< QString >(ignorePluginsName.begin(), ignorePluginsName.end());
}

/**
 * @brief 加载插件
 */
void DAPluginManager::load(DACoreInterface* c)
{
	QFileInfoList fileInfos = d_ptr->mPluginDir.entryInfoList(QDir::Files);

	qInfo() << tr("plugin dir is:%1").arg(d_ptr->mPluginDir.absolutePath());
	for (const QFileInfo& fi : qAsConst(fileInfos)) {
		if (d_ptr->mIgnorePluginBaseName.contains(fi.baseName().toLower())) {
			qInfo() << tr("ignore plugin %1").arg(fi.baseName());
			continue;
		}
		const QString filepath = fi.absoluteFilePath();
		if (!QLibrary::isLibrary(filepath)) {
			qWarning() << tr(" ignore invalid file:%1").arg(fi.absoluteFilePath());
			continue;
		}
		DAPluginOption pluginopt;
		emit beginLoadPlugin(fi.absoluteFilePath());
		if (!pluginopt.load(fi.absoluteFilePath(), c)) {
			qDebug() << tr("can not load plugin:%1").arg(fi.absoluteFilePath());
			continue;
		}
		d_ptr->mPluginOptions.append(pluginopt);
	}
	d_ptr->mIsLoaded = true;
}

bool DAPluginManager::isLoaded() const
{
    return (d_ptr->mIsLoaded);
}

/**
 * @brief 设置插件路径，可以多次load，同一个插件（插件名称和类型组成一个key）只会加载一次
 */
void DAPluginManager::setPluginPath(const QString& path)
{
    d_ptr->mPluginDir.setPath(path);
}

/**
 * @brief 获取加载成功插件的数量
 * @return
 */
int DAPluginManager::getPluginCount() const
{
    return (d_ptr->mPluginOptions.size());
}

/**
 * @brief 获取加载成功插件的插件名
 * @return
 */
QList< QString > DAPluginManager::getPluginNames() const
{
	QList< QString > res;

	for (const DAPluginOption& opt : qAsConst(d_ptr->mPluginOptions)) {
		res.append(opt.getPluginName());
	}
	return (res);
}

/**
 * @brief 获取所有插件信息
 * @return
 */
QList< DAPluginOption > DAPluginManager::getPluginOptions() const
{
    return (d_ptr->mPluginOptions);
}

QDebug operator<<(QDebug debug, const DAPluginManager& fmg)
{
	QDebugStateSaver saver(debug);

	debug.nospace() << DAPluginManager::tr("Plugin Manager Info:is loaded=%1,plugin counts=%2")
						   .arg(fmg.isLoaded())
						   .arg(fmg.getPluginCount())
#if QT_VERSION_MAJOR >= 6
					<< Qt::endl;
#else
					<< endl;
#endif
	QList< DAPluginOption > opts = fmg.getPluginOptions();

	for (const DAPluginOption& opt : qAsConst(opts)) {
		debug.nospace() << opt;
	}
	return (debug);
}
}  // namespace DA
