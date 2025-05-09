#ifndef DAGLOBALCOLORTHEME_H
#define DAGLOBALCOLORTHEME_H

#include <QObject>
#include "DAGuiAPI.h"
#include "DAColorTheme.h"
namespace DA
{
/**
 * @brief 全局的主题管理
 */
class DAGUI_API DAGlobalColorTheme : public QObject
{
	Q_OBJECT
private:
	DAGlobalColorTheme(DAColorTheme::ColorThemeStyle t);

public:
	static DAGlobalColorTheme& getInstance();
	// 设置主题
	void setTheme(const DAColorTheme& v);
	DAColorTheme getTheme() const;
	DAColorTheme& theme();
	const DAColorTheme& theme() const;
	// 从主题获取颜色，主题的颜色自动下移
	QColor color();
signals:
	/**
	 * @brief 主题发生变换的信号
	 * @param v
	 */
	void themeChanged(const DAColorTheme& v);

private:
	DAColorTheme mTheme;  ///< 主题
};
}

#endif  // DAGLOBALCOLORTHEME_H
