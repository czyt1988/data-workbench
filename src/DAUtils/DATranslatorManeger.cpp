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
	for (QTranslator* t : qAsConst(mTranslatorLists)) {
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
	qDebug() << "Setting up translator:" << "\nLanguage:" << QLocale::languageToString(locale.language())
             << "\nCountry:" << QLocale::countryToString(locale.country())
             << "\nScript:" << QLocale::scriptToString(locale.script()) << "\nName:" << locale.name()
             << "\nbcp47 Name:" << locale.bcp47Name() << "\n ui language:" << locale.uiLanguages()
             << "\nAm Text:" << locale.amText() << "\nPm Text:" << locale.pmText()
             << "\nCurrency Symbol:" << locale.currencySymbol() << "\nDate Format:" << locale.dateFormat()
             << "\nDate Time Format:" << locale.dateTimeFormat() << "\nDecimal point:" << locale.decimalPoint()
             << "\nGroup separator:" << locale.groupSeparator() << "\nExponential:" << locale.exponential()
             << "\nZero digit:" << locale.zeroDigit() << "\nPercent:" << locale.percent()
             << "\nPositive sign:" << locale.positiveSign() << "\nNegative sign:" << locale.negativeSign();
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
	for (const QString& p : qAsConst(trPaths)) {
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
	return { pathQtTr, pathUserTr };
}

}
