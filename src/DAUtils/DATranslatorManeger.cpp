#include "DATranslatorManeger.h"
#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QDebug>
#include <memory>
#include "DADir.h"
namespace DA
{

class DATranslatorManeger::PrivateData
{
	DA_DECLARE_PUBLIC(DATranslatorManeger)
public:
	PrivateData(DATranslatorManeger* p);
	void clearAllTranslator();

public:
	QLocale mLocal;
	QList< QString > mTranslatorFilePaths;
	QList< QTranslator* > mTranslatorLists;
	//    QList< QString > mLoadedFiles;  ///< 记录加载的翻译文件
};

DATranslatorManeger::PrivateData::PrivateData(DATranslatorManeger* p) : q_ptr(p)
{
}

void DATranslatorManeger::PrivateData::clearAllTranslator()
{
	for (QTranslator* t : std::as_const(mTranslatorLists)) {
		delete t;
	}
	mTranslatorLists.clear();
}

//==============================================================
// DATranslator
//==============================================================
DATranslatorManeger::DATranslatorManeger() : d_ptr(new DATranslatorManeger::PrivateData(this))
{
	const QLocale& locale = d_ptr->mLocal;
	qDebug() << "Translator locale:" << locale.name() << "(" << QLocale::languageToString(locale.language()) << ")";
	setTranslatorFilePaths(getDefaultTranslatorFilePath());
}

DATranslatorManeger::~DATranslatorManeger()
{
}

/**
 * @brief 装载所有的翻译
 * @return
 */
int DATranslatorManeger::installAllTranslator()
{
	QString langCode = locale().name();
	int r            = installAllTranslator(langCode);
	qDebug() << QString("success install %1 translator").arg(r);
	return r;
}

/**
 * @brief 装载所有的翻译
 * @param langCode locale().name()
 * @return
 */
int DATranslatorManeger::installAllTranslator(const QString& langCode)
{
	qDebug() << "begin install translator at " << langCode;
	const QList< QTranslator* > translators = getAvailableTranslators(langCode);
	if (translators.size() > 0) {
		d_ptr->clearAllTranslator();
		for (QTranslator* t : translators) {
			if (t->isEmpty()) {
				qDebug() << "get empty translator";
				// getAvailableTranslators生成的对象，如果不使用，必须delete；
				delete t;
				continue;
			}
			if (QCoreApplication::installTranslator(t)) {
				d_ptr->mTranslatorLists.append(t);
			} else {
				qWarning() << "can not install translator to application";
				delete t;  // 如果安装不成功直接删除QTranslator
			}
		}
	}
	// 回退：如果指定语言没找到翻译文件，尝试 en_US 作为通用回退
	if (d_ptr->mTranslatorLists.isEmpty() && langCode != "en_US") {
		qDebug() << "No translator found for" << langCode << ", falling back to en_US";
		const QList< QTranslator* > fallbackTranslators = getAvailableTranslators("en_US");
		for (QTranslator* t : fallbackTranslators) {
			if (t->isEmpty()) {
				delete t;
				continue;
			}
			if (QCoreApplication::installTranslator(t)) {
				d_ptr->mTranslatorLists.append(t);
			} else {
				delete t;
			}
		}
	}
	return d_ptr->mTranslatorLists.size();
}
/**
 * @brief 设置扫描文件路径
 * @param ps
 */
void DATranslatorManeger::setTranslatorFilePaths(const QList< QString >& ps)
{
    d_ptr->mTranslatorFilePaths = ps;
}
/**
 * @brief 获取扫描文件路径
 * @return
 */
QList< QString > DATranslatorManeger::getTranslatorFilePath() const
{
    return d_ptr->mTranslatorFilePaths;
}

/**
 * @brief 设置local
 * @param l
 */
void DATranslatorManeger::setLocale(const QLocale& l)
{
    d_ptr->mLocal = l;
}

QList< QTranslator* > DATranslatorManeger::getAvailableTranslators(const QString& langCode)
{
	QList< QString > trPaths = getTranslatorFilePath();
	QList< QTranslator* > translators;
	for (const QString& p : std::as_const(trPaths)) {
        qDebug() << "search qm file in dir:" << p;
		QDir dir(p);
		if (!dir.exists()) {
			continue;
		}
        // 获取当前目录下所有条目（包括文件和子目录）
        const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

        for (const QFileInfo& fi : entries) {
            if (fi.fileName().endsWith(QString("%1.qm").arg(langCode))) {
                qDebug() << "find translate file:" << fi.fileName();
                // 找到qm文件
                std::unique_ptr< QTranslator > translator = std::make_unique< QTranslator >();
                if (translator->load(fi.fileName(), dir.absolutePath())) {
                    translators.append(translator.release());
                    qDebug() << "success load language file:" << fi.fileName() << " in dir " << dir.absolutePath();
                } else {
                    qDebug() << "can not load translator:" << fi.fileName() << " in dir " << dir.absolutePath();
                }
            } else {
                qDebug() << "skip translate file:" << fi.fileName();
            }
		}
	}
	return translators;
}
/**
 * @brief 获取QLocale
 * @return
 */
const QLocale& DATranslatorManeger::locale() const
{
    return d_ptr->mLocal;
}
/**
 * @brief 获取QLocale
 * @return
 */
QLocale& DATranslatorManeger::locale()
{
    return d_ptr->mLocal;
}
/**
 * @brief 获取QLocale
 * @return
 */
QLocale DATranslatorManeger::getLocale() const
{
    return d_ptr->mLocal;
}
/**
 * @brief 获取翻译文件路径
 *
 * 目前此函数写死在代码中，后续如果处理插件的翻译，可以通过配置文件配置翻译文件的路径从而实现动态加载翻译文件
 * @return
 */
QList< QString > DATranslatorManeger::getDefaultTranslatorFilePath()
{
    QString basePath   = DADir::getExecutablePath();
	QString pathQtTr   = QDir::toNativeSeparators(basePath + "/translations");
	QString pathUserTr = QDir::toNativeSeparators(basePath + "/translations_user");
	// 多配置生成器（如 Visual Studio）下 exe 在 bin/<config>/ 子目录，
	// 但翻译文件在 bin/translations/（与 install 目录布局一致）。
	// 添加父目录的 translations 路径，使构建目录调试时也能找到翻译文件。
	QString parentTr = QDir::toNativeSeparators(QDir(basePath + "/..").absolutePath() + "/translations");
	return { pathQtTr, pathUserTr, parentTr };
}

}
