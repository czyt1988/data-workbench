#include "DATranslatorManeger.h"
#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QDebug>
#include <memory>
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
    QList< QString > mTranslatorNames;
    QList< QTranslator* > mTranslatorLists;
    //    QList< QString > mLoadedFiles;  ///< 记录加载的翻译文件
};

DATranslatorManeger::PrivateData::PrivateData(DATranslatorManeger* p) : q_ptr(p)
{
    mTranslatorNames << "qt"
                     << "da";
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
    qDebug() << "Setting up translator:"
             << "\nLanguage:" << QLocale::languageToString(locale.language())
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

/**
 * @brief 指定前缀文件构造
 * @param fileNames
 */
DATranslatorManeger::DATranslatorManeger(const QList< QString >& fileNames)
    : d_ptr(new DATranslatorManeger::PrivateData(this))
{
    setTranslatorFilePaths(getDefaultTranslatorFilePath());
    setTranslatorFileNames(fileNames);
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
    return installAllTranslator(langCode);
}

/**
 * @brief 装载所有的翻译
 * @param langCode locale().name()
 * @return
 */
int DATranslatorManeger::installAllTranslator(const QString& langCode)
{
    QList< QString > trPaths = getTranslatorFilePath();
    QList< QString > trName  = getTranslatorFileNames();
    int cnt                  = 0;
    QList< QTranslator* > translators;
    for (const QString& p : qAsConst(trPaths)) {
        QDir dir(p);
        if (!dir.exists()) {
            continue;
        }
        for (const QString& name : qAsConst(trName)) {
            QScopedPointer< QTranslator > translator(new QTranslator());
            QString fullName = name + "_" + langCode;
            if (translator->load(fullName, dir.absolutePath())) {
                translators.append(translator.take());
                qDebug() << "success load language file:" << fullName << " in dir " << dir.absolutePath();
            } else {
                qDebug() << "can not find translator by " << fullName << " in dir " << dir.absolutePath();
            }
        }
    }
    if (translators.size() > 0) {
        d_ptr->clearAllTranslator();
        for (QTranslator* t : qAsConst(translators)) {
            if (QCoreApplication::installTranslator(t)) {
                d_ptr->mTranslatorLists.append(t);
            } else {
                qWarning() << "can not install translator to application";
                delete t;  //如果安装不成功直接删除QTranslator
            }
        }
    }
    return cnt;
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
 * @brief 设置翻译文件的前缀名
 * @param ps
 */
void DATranslatorManeger::setTranslatorFileNames(const QList< QString >& ps)
{
    d_ptr->mTranslatorNames = ps;
}
/**
 * @brief 获取翻译文件的前缀名
 * @param ps
 */
QList< QString > DATranslatorManeger::getTranslatorFileNames() const
{
    return d_ptr->mTranslatorNames;
}

void DATranslatorManeger::addTranslatorFileNames(const QString& ps)
{
    d_ptr->mTranslatorNames.append(ps);
}

/**
 * @brief 设置local
 * @param l
 */
void DATranslatorManeger::setLocale(const QLocale& l)
{
    d_ptr->mLocal = l;
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
    QString basePath   = QCoreApplication::applicationDirPath();
    QString pathQtTr   = QDir::toNativeSeparators(basePath + "/translations");
    QString pathUserTr = QDir::toNativeSeparators(basePath + "/translations_user");
    return { pathQtTr, pathUserTr };
}

}
