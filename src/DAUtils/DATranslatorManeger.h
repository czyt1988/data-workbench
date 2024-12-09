#ifndef DATRANSLATORMANEGER_H
#define DATRANSLATORMANEGER_H
#include <QTranslator>
#include <QObject>
#include "DAUtilsAPI.h"
namespace DA
{
/**
 * @brief 翻译器管理
 *
 * 通过设置翻译文件夹路径，自动扫描以QLocale.name结尾的语言文件并加载
 *
 * 使用此类有如下操作：
 * - 设置扫描路径
 */
class DAUTILS_API DATranslatorManeger
{
	DA_DECLARE_PRIVATE(DATranslatorManeger)
public:
	DATranslatorManeger();
	// 指定前缀文件构造
	DATranslatorManeger(const QList< QString >& fileNames);
	~DATranslatorManeger();
	// 装载所以的翻译
	int installAllTranslator();
	// 根据langCode装载所以的翻译，如中文langCode=zh_CN
	int installAllTranslator(const QString& langCode);
	// 设置扫描文件路径
	void setTranslatorFilePaths(const QList< QString >& ps);
	QList< QString > getTranslatorFilePath() const;
	// 设置翻译文件的前缀，注意只要前缀
	void setTranslatorFileNames(const QList< QString >& ps);
	QList< QString > getTranslatorFileNames() const;
	void addTranslatorFileNames(const QString& ps);
	// 获取QLocale
	const QLocale& locale() const;
	QLocale& locale();
	QLocale getLocale() const;
	// 设置local
	void setLocale(const QLocale& l);
	// 获取可用的翻译器，注意，如果不使用，需要delete
	QList< QTranslator* > getAvailableTranslators(const QString& langCode);

public:
	// 获取翻译文件路径
	static QList< QString > getDefaultTranslatorFilePath();
};

}  // end of DA

#endif  // DATRANSLATOR_H
