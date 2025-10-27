#ifndef DADEBUG_H
#define DADEBUG_H
#include <QDebug>
#include <type_traits>

namespace DA
{

struct DAEmptyDebug
{
	template< typename T >
	DAEmptyDebug& operator<<(const T&)
	{
		return *this;
	}
	DAEmptyDebug& operator<<(QTextStreamFunction)
	{
		return *this;
	}
	DAEmptyDebug& operator<<(QDebugStateSaver)
	{
		return *this;
	}
};

namespace detail
{
template< bool On >
struct DebugChooser
{
	using type = QDebug;
	static type get()
	{
		return qDebug();
	}
};
template<>
struct DebugChooser< false >
{
	using type = DAEmptyDebug;
	static type get()
	{
		return {};
	}
};
}  // namespace detail
}  // namespace DA

/*
 * @def 一个自动局部打印控制开关宏
 *
 * 在每个需要开启局部打印控制的类中，只要使用此宏定义，即可开启局部打印控制
 *
 * 举例：
 * ```cpp
 * // MyClass.cpp
 * #include "DADebug.hpp"
 * DA_DEBUG_PRINT(MyClass, true)
 *
 * MyClass::fun() {
 *  DADebug << "MyClass::fun()";  // 注意：现在使用时不加括号
 *  DADebug << "文件:" << __FILE__ << "行号:" << __LINE__;
 * }
 * ```
 * 使用时在你的类的实现文件中首先定义宏`DA_DEBUG_PRINT`,这个宏第一个参数需要输入一个别名，最好是此类类名，第二个参数代表是否开启打印，true为开启，false为关闭
 * 开启后，`DADebug`等同`qDebug()`,不开始，则不会做任何打印
 *
 * 重要变更：DADebug 现在是一个宏而不是函数，使用时不需要括号
 */
#define DA_DEBUG_PRINT(Class, Switch) \
        static constexpr bool Class##_DEBUG_PRINT = (Switch); \
        static inline auto DADebug() -> typename ::DA::detail::DebugChooser< Class##_DEBUG_PRINT >::type \
        { \
                return ::DA::detail::DebugChooser< Class##_DEBUG_PRINT >::get(); \
        }

#endif  // DADEBUG_H
