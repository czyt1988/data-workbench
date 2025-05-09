#ifndef DASIGNALBLOCKERS_H
#define DASIGNALBLOCKERS_H
#include <vector>
#include <QObject>
#include <QSignalBlocker>
namespace DA
{
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
    // 使用可变参数模板构造函数
    template< typename... Args >
    explicit DASignalBlockers(Args*... objects)
    {
        addObjects(objects...);
    }

    // 禁止拷贝和赋值
    DASignalBlockers(const DASignalBlockers&)            = delete;
    DASignalBlockers& operator=(const DASignalBlockers&) = delete;

    // 移动构造函数
    DASignalBlockers(DASignalBlockers&&) = default;

    // 移动赋值运算符
    DASignalBlockers& operator=(DASignalBlockers&&) = default;

private:
    std::vector< QSignalBlocker > blockers;  // 直接在栈上存储 QSignalBlocker

    // 辅助函数：递归添加 QObject 到 blockers
    template< typename First, typename... Rest >
    void addObjects(First* first, Rest*... rest)
    {
        if (first) {
            blockers.emplace_back(*first);  // 在栈上创建 QSignalBlocker
        }
        if constexpr (sizeof...(rest) > 0) {  // 如果还有剩余参数
            addObjects(rest...);
        }
    }
};
}
#endif  // DASIGNALBLOCKERS_H
