#ifndef DACOLORTHEME_H
#define DACOLORTHEME_H
#include "DAIndexedVector.hpp"
#include <initializer_list>
#include <QColor>
#include <QDebug>
#include "DAUtilsAPI.h"
namespace DA
{

/**
 * @brief 颜色主题
 */
class DAUTILS_API DAColorTheme : public DAIndexedVector< QColor >
{
public:
	/**
	 * @brief 已有的主题
	 *
	 * 配色参考：https://github.com/BlakeRMills/MetBrewer
	 *
	 * 自动化生成代码见：src/PyScripts/create_color_theme.py
	 */
	enum ColorTheme
	{
		ColorTheme_Archambault,
		ColorTheme_Cassatt1,
		ColorTheme_Cassatt2,
		ColorTheme_Demuth,
		ColorTheme_Derain,
		ColorTheme_Egypt,
		ColorTheme_Greek,
		ColorTheme_Hiroshige,
		ColorTheme_Hokusai2,
		ColorTheme_Hokusai3,
		ColorTheme_Ingres,
		ColorTheme_Isfahan1,
		ColorTheme_Isfahan2,
		ColorTheme_Java,
		ColorTheme_Johnson,
		ColorTheme_Kandinsky,
		ColorTheme_Morgenstern,
		ColorTheme_OKeeffe1,
		ColorTheme_OKeeffe2,
		ColorTheme_Pillement,
		ColorTheme_Tam,
		ColorTheme_Troy,
		ColorTheme_VanGogh3,
		ColorTheme_Veronese,
		ColorTheme_End,               ///< 结束
		ColorTheme_UserDefine = 2000  ///< 用户自定义
	};

public:
	DAColorTheme() : DAIndexedVector< QColor >()
	{
	}
	/**
	 * @brief 不要用DAColorTheme mColorTheme { DAColorTheme::ColorTheme_Archambault }这样的初始化，会被当作std::initializer_list< QColor >捕获
	 * @param th
	 */
	DAColorTheme(ColorTheme th) : DAIndexedVector< QColor >()
	{
		*this = DAColorTheme::create(th);
	}
	DAColorTheme(const std::initializer_list< QColor >& v) : DAIndexedVector< QColor >(v)
	{
	}
	~DAColorTheme()
	{
	}
	/**
	 * @brief 创建一个color theme
	 * @param t
	 * @return
	 */
	static DAColorTheme create(ColorTheme t);
	/**
	 * @brief 重载等于操作符，可以直接通过主题赋值
	 * @param th
	 * @return
	 */
	DAColorTheme& operator=(const ColorTheme& th);

private:
	ColorTheme mCurrentColorTheme { ColorTheme_UserDefine };  ///< 当前的主题
};

}  // end namespace DA
Q_DECLARE_METATYPE(DA::DAColorTheme)

QDebug DAUTILS_API operator<<(QDebug debug, const DA::DAColorTheme& th);

#endif  // DACOLORTHEME_H
