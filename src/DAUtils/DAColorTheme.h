#ifndef DACOLORTHEME_H
#define DACOLORTHEME_H
#include "DAGenericIndexedContainer.hpp"
#include <initializer_list>
#include <QColor>
#include <QDebug>
#include "DAUtilsAPI.h"
namespace DA
{

/**
 * @brief 颜色主题
 */
class DAUTILS_API DAColorTheme
{
public:
	using ColorList      = QVector< QColor >;
	using container      = DAGenericIndexedContainer< ColorList >;
	using iterator       = container::iterator;
	using const_iterator = container::const_iterator;

public:
	/**
	 * @brief 已有的主题
	 *
	 * 配色参考：https://github.com/BlakeRMills/MetBrewer
	 *
	 * 自动化生成代码见：src/PyScripts/create_color_theme.py
	 */
	enum ColorThemeStyle
	{
		Style_Archambault,
		Style_Cassatt1,
		Style_Cassatt2,
		Style_Demuth,
		Style_Derain,
		Style_Egypt,
		Style_Greek,
		Style_Hiroshige,
		Style_Hokusai2,
		Style_Hokusai3,
		Style_Ingres,
		Style_Isfahan1,
		Style_Isfahan2,
		Style_Java,
		Style_Johnson,
		Style_Kandinsky,
		Style_Morgenstern,
		Style_OKeeffe1,
		Style_OKeeffe2,
		Style_Pillement,
		Style_Tam,
		Style_Troy,
		Style_VanGogh3,
		Style_Veronese,
		Style_End,               ///< 结束
		Style_UserDefine = 2000  ///< 用户自定义
	};

public:
	DAColorTheme();
	/**
	 * @brief 不要用DAColorTheme mColorTheme { DAColorTheme::ColorTheme_Archambault }这样的初始化，会被当作std::initializer_list< QColor >捕获
	 * @param th
	 */
	DAColorTheme(ColorThemeStyle th);
	DAColorTheme(const std::initializer_list< QColor >& v);
	~DAColorTheme();
	/**
	 * @brief 创建一个color theme
	 * @param t
	 * @return
	 */
	static DAColorTheme create(ColorThemeStyle t);
	/**
	 * @brief 重载等于操作符，可以直接通过主题赋值
	 * @param th
	 * @return
	 */
	DAColorTheme& operator=(const ColorThemeStyle& th);
	// 获取下一个元素(索引后移)
	QColor next();
	// 把索引移动到下一个,如果超过范围，会回到头
	void moveToNext();
	// 前缀操作,++v,返回下一个
	QColor operator++();
	// 后缀操作,v++，返回当前，并移动到下一个
	QColor operator++(int);
	// 获取前一个元素(索引前移)
	QColor previous();
	// 把索引移动到下一个,如果超过范围，会回尾部
	void moveToPrevious();
	// 前缀操作--v,回退并返回值
	QColor operator--();
	// 后缀操作,v--，返回当前，并回退
	QColor operator--(int);
	// 获取当前的元素，在调用前使用isValidIndex确认索引的正确性
	QColor current() const;
	// 按照索引获取
	QColor at(int index) const;
	//[]操作
	QColor& operator[](int index);
	const QColor& operator[](int index) const;
	// 最后的颜色
	QColor lastColor() const;
	// 第一个颜色
	QColor firstColor() const;
	// 判断当前索引是否是第一个
	bool isFirstIndex() const;
	// 判断当前索引是否是最后一个
	bool isLastIndex() const;
	// 判断当前索引是否是合理范围内
	bool isValidIndex() const;
	// 获取当前的索引
	int getCurrentIndex() const;
	// 设置当前的索引
	void setCurrentIndex(int v);
	//
	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;
	// 数量
	int size() const;
	// 按照比例获取颜色，颜色将使用插值获取，proportion必须为0~1之间
	QColor getColorAtPosition(float proportion) const;
	// 颜色插值，在颜色1和2之间的比例取值,t必须为0~1之间的数
	static QColor interpolateColor(const QColor& color1, const QColor& color2, float t);
	// 主题样式
	ColorThemeStyle getColorThemeStyle() const;
	void setColorThemeStyle(ColorThemeStyle style);
	// 设置用户自定义的颜色列表
	void setUserDefineColorList(const ColorList& cls, ColorThemeStyle style = Style_UserDefine);

protected:
	container createColorList(const ColorThemeStyle& th);

private:
	container mColorList;
	ColorThemeStyle mCurrentColorTheme { Style_Archambault };  ///< 当前的主题
};

QDebug DAUTILS_API operator<<(QDebug debug, const DAColorTheme& th);

}  // end namespace DA
Q_DECLARE_METATYPE(DA::DAColorTheme)

#endif  // DACOLORTHEME_H
