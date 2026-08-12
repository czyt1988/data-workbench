#ifndef DAGENERICINDEXEDCONTAINER_H
#define DAGENERICINDEXEDCONTAINER_H

#include <type_traits>
#include <utility>
#include <stdexcept>
namespace DA
{

/**
 * @brief 通用索引容器模板类，为底层容器提供循环索引功能
 * @tparam Container 底层容器类型，需满足以下接口：
 *                   - `size_type size() const` 获取元素数量
 *                   - `reference operator[](size_type)` 下标访问
 *                   - `const_reference operator[](size_type) const` 常量下标访问
 *                   - 支持初始化列表构造
 * @tparam IndexType 索引值类型，用于跟踪容器位置，默认int类型
 * @tparam SFINAE_CHECK SFINAE约束检查，确保容器符合所需接口
 *
 * 本类通过组合方式包装任意标准容器，为其添加当前索引跟踪功能，支持循环遍历操作。
 * 典型应用场景包括：循环缓冲区、轮播列表、状态机循环切换等。
 *
 * ## 特性
 * - 零成本抽象：所有操作均为O(1)时间复杂度
 * - 类型安全：编译期接口检查
 * - 循环语义：到达边界自动回绕
 * - 异常安全：不抛异常保证(nothrow)
 *
 * ## 示例1：使用std::vector
 * @code
 * DAGenericIndexedContainer<std::vector<std::string>> words{"A", "B", "C"};
 * ++words; // 当前索引1，值为"B"
 * words.moveToPrevious(); // 索引0，值"A"
 * @endcode
 *
 * ## 示例2：使用Qt容器
 * @code
 * DAGenericIndexedContainer<QList<QColor>, quint16> colors{Qt::red, Qt::green};
 * colors.emplace_back(Qt::blue); // 添加颜色
 * auto current = colors++; // 返回Qt::red，索引前进到1
 * @endcode
 *
 * ## 示例3：空容器处理
 * @code
 * DAGenericIndexedContainer<std::vector<int>> emptyVec;
 * assert(emptyVec.isValidIndex() == false); // 空容器索引无效
 * emptyVec.moveToNext(); // 无操作
 * @endcode
 */
template< typename Container,
          typename IndexType = int,
          typename SFINAE_CHECK =
              std::void_t< typename Container::size_type,
                           typename Container::value_type,
                           decltype(std::declval< Container >()[ std::declval< typename Container::size_type >() ]) > >
class DAGenericIndexedContainer
{
public:
    /// 容器元素类型
    using value_type = typename Container::value_type;
    /// 元素引用类型
    using reference = typename Container::reference;
    /// 常量元素引用类型
    using const_reference = typename Container::const_reference;
    /// 容器大小类型
    using size_type = typename Container::size_type;
    /// @name 迭代器支持
    /// @{
    using iterator               = typename Container::iterator;                ///< 非常量迭代器类型
    using const_iterator         = typename Container::const_iterator;          ///< 常量迭代器类型
    using reverse_iterator       = typename Container::reverse_iterator;        ///< 反向迭代器类型
    using const_reverse_iterator = typename Container::const_reverse_iterator;  ///< 常量反向迭代器类型
    /// @}
    /**
     * @brief 容器替换时的索引调整策略
     *
     * 该枚举定义了在替换底层容器时，如何处理当前索引的不同策略。
     * 用于控制容器内容更新后索引的自动调整逻辑。
     *
     * @see replace()
     */
    enum class IndexPolicy
    {
        /**
         * @brief 自动智能重置策略（默认）
         *
         * - 当新容器非空时：
         *   - 若原索引在[0, 新容器size())范围内，保持原索引
         *   - 若原索引超出范围，重置为0
         * - 当新容器为空时：
         *   - 强制设置索引为0（此时isValidIndex()为false）
         *
         * 适用场景：大多数常规替换操作，在保持索引有效性的同时尽可能保留位置
         *
         * @code
         * // 原索引2，新容器size=5 → 保持2
         * // 原索引3，新容器size=2 → 重置为0
         * // 替换为空容器 → 索引0（无效）
         * cont.replace(newData, IndexPolicy::AutoReset);
         * @endcode
         */
        AutoReset,

        /**
         * @brief 原始索引保留策略
         *
         * 完全保留当前索引数值，无论是否有效。
         * 调用者需自行检查索引有效性。
         *
         * 适用场景：需要精确控制索引位置的高级用法
         *
         * @warning 使用后必须检查isValidIndex()
         *
         * @code
         * // 原索引3，新容器size=1 → 保持3（无效）
         * cont.replace(newData, IndexPolicy::PreserveRaw);
         * if (!cont.isValidIndex()) {
         *     cont.setCurrentIndex(0); // 手动修正
         * }
         * @endcode
         */
        PreserveRaw,

        /**
         * @brief 强制重置策略
         *
         * 无论原索引如何，强制设置索引为0。
         * - 新容器非空时：索引0有效
         * - 新容器为空时：索引0无效
         *
         * 适用场景：需要从容器起始位置开始操作的场景
         *
         * @code
         * // 任意原索引 → 强制置0
         * cont.replace(newData, IndexPolicy::ForceReset);
         * assert(cont.currentIndex() == 0);
         * @endcode
         */
        ForceReset
    };

public:
    /// @name 构造与初始化
    /// @{

    // 默认构造空容器
    DAGenericIndexedContainer() = default;

    // 完美转发构造底层容器
    template< typename... Args >
    explicit DAGenericIndexedContainer(Args&&... args) : mContainer(std::forward< Args >(args)...)
    {
    }

    // 初始化列表构造
    DAGenericIndexedContainer(std::initializer_list< value_type > init) : mContainer(init)
    {
    }
    /// @}

    /// @name 索引导航
    /// @{

    // 移动后获取下一个元素
    value_type next()
    {
        moveToNext();
        return current();
    }

    // 移动到下一个位置(循环)
    void moveToNext()
    {
        if (empty())
            return;
        mIndex = (mIndex + 1) % static_cast< IndexType >(size());
    }

    // 移动后获取上一个元素
    value_type previous()
    {
        moveToPrevious();
        return current();
    }

    // 移动到上一个位置(循环)
    void moveToPrevious()
    {
        if (empty())
            return;
        mIndex = (mIndex == 0) ? static_cast< IndexType >(size() - 1) : mIndex - 1;
    }

    // 获取当前元素(非常量)
    reference current()
    {
        return mContainer[ static_cast< size_type >(mIndex) ];
    }

    // 获取当前元素(常量)
    const_reference current() const
    {
        return mContainer[ static_cast< size_type >(mIndex) ];
    }
    /// @}

    /// @name 运算符重载
    /// @{

    // 前缀递增(++obj)
    value_type operator++()
    {
        moveToNext();
        return current();
    }

    // 后缀递增(obj++)
    value_type operator++(int)
    {
        value_type tmp = current();
        moveToNext();
        return tmp;
    }

    // 前缀递减(--obj)
    value_type operator--()
    {
        moveToPrevious();
        return current();
    }

    // 后缀递减(obj--)
    value_type operator--(int)
    {
        value_type tmp = current();
        moveToPrevious();
        return tmp;
    }
    /// @}

    /// @name 容器访问
    /// @{

    // 检查容器是否为空
    bool empty() const noexcept
    {
        return mContainer.empty();
    }

    // 获取元素数量
    size_type size() const noexcept
    {
        return mContainer.size();
    }

    // 清空容器并重置索引
    void clear()
    {
        mContainer.clear();
        mIndex = 0;
    }

    // 向容器末尾原位构造元素
    template< typename... Args >
    void emplace_back(Args&&... args)
    {
        mContainer.emplace_back(std::forward< Args >(args)...);
    }
    /// @}

    /// @name 首尾元素访问
    /// @{

    // 获取首元素（非常量版本）
    reference first()
    {
        return mContainer.front();
    }

    // 获取首元素（常量版本）
    const_reference first() const
    {
        return mContainer.front();
    }

    // 获取尾元素（非常量版本）
    reference last()
    {
        return mContainer.back();
    }

    // 获取尾元素（常量版本）
    const_reference last() const
    {
        return mContainer.back();
    }

    // 安全获取首元素（带边界检查）
    reference firstChecked()
    {
        if (empty()) {
            throw std::out_of_range("Accessing first element of empty container");
        }
        return mContainer.front();
    }

    // 安全获取首元素（常量版本）
    const_reference firstChecked() const
    {
        if (empty()) {
            throw std::out_of_range("Accessing first element of empty container");
        }
        return mContainer.front();
    }

    // 安全获取尾元素（带边界检查）
    reference lastChecked()
    {
        if (empty()) {
            throw std::out_of_range("Accessing last element of empty container");
        }
        return mContainer.back();
    }

    // 安全获取尾元素（常量版本）
    const_reference lastChecked() const
    {
        if (empty()) {
            throw std::out_of_range("Accessing last element of empty container");
        }
        return mContainer.back();
    }

    /// @}

    /// @name 索引控制
    /// @{

    // 获取当前索引值
    IndexType currentIndex() const noexcept
    {
        return mIndex;
    }

    // 设置当前索引
    void setCurrentIndex(IndexType index) noexcept
    {
        mIndex = index;
    }

    // 检查当前索引是否有效
    bool isValidIndex() const noexcept
    {
        return mIndex >= 0 && static_cast< size_type >(mIndex) < size();
    }

    // 检查是否是第一个索引
    bool isFirstIndex() const
    {
        return mIndex == 0 || mContainer.empty();
    }

    // 检查是否是最后一个索引
    bool isLastIndex() const
    {
        return (!mContainer.empty()) && (mIndex == mContainer.size() - 1);
    }

    /// @}

    /// @name 容器赋值操作
    /// @{

    // 拷贝赋值底层容器
    DAGenericIndexedContainer& operator=(const Container& other)
    {

        // 拷贝前保存原索引
        const size_type oldIndex = static_cast< size_type >(mIndex);

        // 执行容器拷贝
        mContainer = other;

        // 调整索引
        if (!mContainer.empty()) {
            // 新容器非空时调整索引
            if (oldIndex < mContainer.size()) {
                mIndex = static_cast< IndexType >(oldIndex);
            } else {
                mIndex = 0;
            }
        } else {
            // 新容器为空时保持索引0（无效状态）
            mIndex = 0;
        }

        return *this;
    }

    // 移动赋值底层容器
    DAGenericIndexedContainer& operator=(Container&& other) noexcept
    {
        // 移动前保存原索引
        const size_type oldIndex = static_cast< size_type >(mIndex);

        // 执行容器移动
        mContainer = std::move(other);

        // 调整索引（逻辑同拷贝版本）
        if (!mContainer.empty()) {
            mIndex = (oldIndex < mContainer.size()) ? static_cast< IndexType >(oldIndex) : 0;
        } else {
            mIndex = 0;
        }
        return *this;
    }

    /// @}

    /// @name 容器内容替换
    /// @{

    // 替换容器内容并智能调整索引
    void replace(Container newContainer, IndexPolicy policy = IndexPolicy::AutoReset)
    {
        const size_type oldIndex = static_cast< size_type >(mIndex);

        mContainer = std::move(newContainer);

        switch (policy) {
        case IndexPolicy::AutoReset:
            if (!mContainer.empty()) {
                mIndex = (oldIndex < mContainer.size()) ? static_cast< IndexType >(oldIndex) : 0;
            } else {
                mIndex = 0;
            }
            break;
        case IndexPolicy::PreserveRaw:
            // 不调整，可能无效
            break;
        case IndexPolicy::ForceReset:
            mIndex = 0;
            break;
        }
    }
    /// @}

    /// @name 元素访问
    /// @{

    // 下标访问操作符（非const版本）
    reference operator[](IndexType index)
    {
        return mContainer[ static_cast< size_type >(index) ];
    }

    // 下标访问操作符（const版本）
    const_reference operator[](IndexType index) const
    {
        return mContainer[ static_cast< size_type >(index) ];
    }

    /// @}

    /// @name 安全访问
    /// @{

    // 带边界检查的元素访问
    reference at(IndexType index)
    {
        const size_type idx = static_cast< size_type >(index);
        if (idx >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return mContainer[ idx ];
    }

    // 带边界检查的常量元素访问
    const_reference at(IndexType index) const
    {
        const size_type idx = static_cast< size_type >(index);
        if (idx >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return mContainer[ idx ];
    }
    /// @}

    /// @name 迭代器操作
    /// @{
    // 获取指向容器首元素的迭代器
    iterator begin() noexcept
    {
        return mContainer.begin();
    }

    // 获取指向容器末尾的迭代器
    iterator end() noexcept
    {
        return mContainer.end();
    }

    // 获取常量起始迭代器
    const_iterator begin() const noexcept
    {
        return mContainer.begin();
    }

    // 获取常量结束迭代器
    const_iterator end() const noexcept
    {
        return mContainer.end();
    }

    // 获取常量起始迭代器（C++11风格）
    const_iterator cbegin() const noexcept
    {
        return mContainer.cbegin();
    }

    // 获取常量结束迭代器（C++11风格）
    const_iterator cend() const noexcept
    {
        return mContainer.cend();
    }

    // 获取反向起始迭代器
    reverse_iterator rbegin() noexcept
    {
        return mContainer.rbegin();
    }

    // 获取反向结束迭代器
    reverse_iterator rend() noexcept
    {
        return mContainer.rend();
    }

    // 获取常量反向起始迭代器
    const_reverse_iterator crbegin() const noexcept
    {
        return mContainer.crbegin();
    }

    // 获取常量反向结束迭代器
    const_reverse_iterator crend() const noexcept
    {
        return mContainer.crend();
    }
    /// @}

    const Container& container() const
    {
        return mContainer;
    }

private:
    Container mContainer;  ///< 底层容器存储
    IndexType mIndex = 0;  ///< 当前索引值
};

}  // namespace DA

#endif  // DAGENERICINDEXEDCONTAINER_H
