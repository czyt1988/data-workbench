#ifndef DASIGNALBLOCKERS_H
#define DASIGNALBLOCKERS_H
#include <vector>
#include <QObject>
namespace DA{
/**
 * @brief 支持多个变量执行类似QSingalBlocker的操作
 *
 * @code
 * QObject* a,b,c,d;
 * ....
 * DASignalBlockers block(a,b,c,d);
 *
 * //类似：
 * // QSingalBlocker block1(a),block2(b),block3(c),block4(d);
 * @endcode
 */
class DASignalBlockers
{
public:
	// 构造函数，接受多个QObject*参数
	template< typename... Args >
	explicit DASignalBlockers(Args... args)
	{
		// 使用初始化列表和完美转发来存储QObject指针
		objects.reserve(sizeof...(args));
		((void)std::initializer_list<int>{0, ((void)objects.emplace_back(std::forward<Args>(args)), 0)...});


				// 阻塞所有对象的信号
		blockSignals(true);
	}

			// 析构函数，解锁所有对象的信号
	~DASignalBlockers()
	{
		blockSignals(false);
	}

			// 手动控制信号阻塞的函数
	void blockSignals(bool on)
	{
		for (QObject* obj : objects) {
			if (obj) {  // 确保obj不是nullptr
				obj->blockSignals(on);
			}
		}
	}

private:
	// 存储需要阻塞信号的QObject指针
	std::vector< QObject* > objects;
};
}
#endif  // DASIGNALBLOCKERS_H
