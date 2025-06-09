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
 * 通过设置翻译文件夹路径，自动扫描以{QLocale.name}.qm结尾的语言文件并加载
 *
 * 你的翻译文件应该形如:xxx_zh_CN.qm
 */
class DAUTILS_API DATranslatorManeger
{
	DA_DECLARE_PRIVATE(DATranslatorManeger)
public:
	DATranslatorManeger();
	// 指定前缀文件构造
	~DATranslatorManeger();
	// 装载所以的翻译
	int installAllTranslator();
	// 根据langCode装载所以的翻译，如中文langCode=zh_CN
	int installAllTranslator(const QString& langCode);
	// 设置扫描文件路径
	void setTranslatorFilePaths(const QList< QString >& ps);
	QList< QString > getTranslatorFilePath() const;
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
