#include "DAGlobalColorTheme.h"
namespace DA
{
DAGlobalColorTheme::DAGlobalColorTheme(DAColorTheme::ColorThemeStyle t) : QObject(nullptr), mTheme(t)
{
}

DAGlobalColorTheme& DAGlobalColorTheme::getInstance()
{
	static DAGlobalColorTheme s_value(DAColorTheme::Style_Archambault);
	return s_value;
}

void DAGlobalColorTheme::setTheme(const DAColorTheme& v)
{
	mTheme = v;
}

DAColorTheme DAGlobalColorTheme::getTheme() const
{
	return mTheme;
}

DAColorTheme& DAGlobalColorTheme::theme()
{
	return mTheme;
}

const DAColorTheme& DAGlobalColorTheme::theme() const
{
	return mTheme;
}

/**
 * @brief 从主题获取颜色，主题的颜色自动下移
 * @return
 */
QColor DAGlobalColorTheme::color()
{
	QColor c = mTheme.current();
	mTheme.moveToNext();
	return c;
}

}
