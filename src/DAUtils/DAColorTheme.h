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
class DAUTILS_API DAColorTheme
{
public:
	using iterator       = DAIndexedVector< QColor >::iterator;
	using const_iterator = DAIndexedVector< QColor >::const_iterator;

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
	DAColorTheme();
	/**
	 * @brief 不要用DAColorTheme mColorTheme { DAColorTheme::ColorTheme_Archambault }这样的初始化，会被当作std::initializer_list< QColor >捕获
	 * @param th
	 */
	DAColorTheme(ColorTheme th);
	DAColorTheme(const std::initializer_list< QColor >& v);
	~DAColorTheme();
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
	// 获取当前索引下的元素
	QColor get() const;
	// 设置当前索引下的元素
	void set(const QColor& v);
	//
	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;
	// 数量
	int size() const;

private:
	DAIndexedVector< QColor > mColorList;  ///< 原来DAColorTheme是继承DAIndexedVector< QColor >，但会导致符号重复导出的编译错误
	ColorTheme mCurrentColorTheme { ColorTheme_UserDefine };  ///< 当前的主题
};

}  // end namespace DA
Q_DECLARE_METATYPE(DA::DAColorTheme)

QDebug DAUTILS_API operator<<(QDebug debug, const DA::DAColorTheme& th);

#endif  // DACOLORTHEME_H
