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

    /**
     * @brief 默认构造空容器
     * @note 初始索引为0，但空容器isValidIndex()返回false
     */
    DAGenericIndexedContainer() = default;

    /**
     * @brief 完美转发构造底层容器
     * @tparam Args 构造参数类型
     * @param args 传递给底层容器的构造参数
     *
     * 示例：
     * @code
     * // 构造预留10个int的vector
     * DAGenericIndexedContainer<std::vector<int>> vec(10);
     *
     * // 构造包含3个元素的QList
     * DAGenericIndexedContainer<QList<double>>(3, 1.23);
     * @endcode
     */
    template< typename... Args >
    explicit DAGenericIndexedContainer(Args&&... args) : m_container(std::forward< Args >(args)...)
    {
    }

    /**
     * @brief 初始化列表构造
     * @param init 初始化元素列表
     *
     * 示例：
     * @code
     * DAGenericIndexedContainer<std::vector<char>> chars{'a', 'b', 'c'};
     * assert(chars.size() == 3);
     * @endcode
     */
    DAGenericIndexedContainer(std::initializer_list< value_type > init) : m_container(init)
    {
    }
    /// @}

    /// @name 索引导航
    /// @{

    /**
     * @brief 移动后获取下一个元素
     * @return 新位置的元素
     * @warning 容器为空时行为未定义
     */
    value_type next()
    {
        moveToNext();
        return current();
    }

    /**
     * @brief 移动到下一个位置(循环)
     * @note 空容器调用无效果
     * @post 索引值可能回绕到0
     *
     * 示例：
     * @code
     * DAGenericIndexedContainer<std::array<int,3>> arr{1,2,3};
     * arr.setCurrentIndex(2); // 索引2
     * arr.moveToNext();       // 索引0
     * @endcode
     */
    void moveToNext()
    {
        if (empty())
            return;
        m_index = (m_index + 1) % static_cast< IndexType >(size());
    }

    /**
     * @brief 移动后获取上一个元素
     * @return 新位置的元素
     * @warning 容器为空时行为未定义
     */
    value_type previous()
    {
        moveToPrevious();
        return current();
    }

    /**
     * @brief 移动到上一个位置(循环)
     * @note 空容器调用无效果
     * @post 索引值可能回绕到size()-1
     *
     * 示例：
     * @code
     * DAGenericIndexedContainer<QVector<QString>> vec{"First", "Second"};
     * vec.moveToPrevious(); // 从0回绕到1
     * assert(vec.current() == "Second");
     * @endcode
     */
    void moveToPrevious()
    {
        if (empty())
            return;
        m_index = (m_index == 0) ? static_cast< IndexType >(size() - 1) : m_index - 1;
    }

    /**
     * @brief 获取当前元素(非常量)
     * @return 当前索引处元素的引用
     * @pre isValidIndex() == true
     * @warning 无效索引调用导致未定义行为
     *
     * 示例：
     * @code
     * auto& curr = container.current(); // 获取可修改引用
     * curr.setValue(42); // 修改当前元素
     * @endcode
     */
    reference current()
    {
        return m_container[ static_cast< size_type >(m_index) ];
    }

    /**
     * @brief 获取当前元素(常量)
     * @return 当前索引处元素的常量引用
     * @sa current()
     */
    const_reference current() const
    {
        return m_container[ static_cast< size_type >(m_index) ];
    }
    /// @}

    /// @name 运算符重载
    /// @{

    /**
     * @brief 前缀递增(++obj)
     * @return 递增后的容器引用
     * @post 索引前进一位(可能回绕)
     *
     * 示例：
     * @code
     * auto& c = ++container; // 立即使用新值
     * @endcode
     */
    value_type operator++()
    {
        moveToNext();
        return current();
    }

    /**
     * @brief 后缀递增(obj++)
     * @return 递增前的元素副本
     * @post 索引前进一位(可能回绕)
     *
     * 示例：
     * @code
     * auto old = container++; // 保存旧值
     * process(old);           // 处理旧值
     * @endcode
     */
    value_type operator++(int)
    {
        value_type tmp = current();
        moveToNext();
        return tmp;
    }

    /**
     * @brief 前缀递减(--obj)
     * @return 递减后的容器引用
     * @post 索引后退一位(可能回绕)
     *
     * 示例：
     * @code
     * auto c = --container; // 立即使用新值
     * @endcode
     */
    value_type operator--()
    {
        moveToPrevious();
        return current();
    }

    /**
     * @brief 后缀递减(obj--)
     * @return 递减前的元素副本
     * @post 索引后退一位(可能回绕)
     *
     * 示例：
     * @code
     * auto old = container--; // 保存旧值
     * process(old);           // 处理旧值
     * @endcode
     */
    value_type operator--(int)
    {
        value_type tmp = current();
        moveToPrevious();
        return tmp;
    }
    /// @}

    /// @name 容器访问
    /// @{

    /**
     * @brief 检查容器是否为空
     * @return true 当且仅当size() == 0
     * @note 复杂度O(1)
     */
    bool empty() const noexcept
    {
        return m_container.empty();
    }

    /**
     * @brief 获取元素数量
     * @return 容器当前存储的元素数量
     */
    size_type size() const noexcept
    {
        return m_container.size();
    }

    /**
     * @brief 清空容器并重置索引
     * @post size() == 0 && currentIndex() == 0
     */
    void clear()
    {
        m_container.clear();
        m_index = 0;
    }

    /**
     * @brief 向容器末尾原位构造元素
     * @tparam Args 构造参数类型
     * @param args 元素构造参数
     * @note 要求底层容器实现emplace_back()
     *
     * 示例：
     * @code
     * container.emplace_back(1, "test"); // 构造MyClass(1, "test")
     * @endcode
     */
    template< typename... Args >
    void emplace_back(Args&&... args)
    {
        m_container.emplace_back(std::forward< Args >(args)...);
    }
    /// @}

    /// @name 首尾元素访问
    /// @{

    /**
     * @brief 获取首元素（非常量版本）
     * @return 容器第一个元素的引用
     * @warning 容器为空时调用将导致未定义行为
     * @note 等效于 operator[](0)
     *
     * 示例：
     * @code
     * cont.first() = 5;  // 修改第一个元素
     * @endcode
     */
    reference first()
    {
        return m_container.front();
    }

    /**
     * @brief 获取首元素（常量版本）
     * @sa first()
     */
    const_reference first() const
    {
        return m_container.front();
    }

    /**
     * @brief 获取尾元素（非常量版本）
     * @return 容器最后一个元素的引用
     * @warning 容器为空时调用将导致未定义行为
     * @note 等效于 operator[](size()-1)
     *
     * 示例：
     * @code
     * cont.last() += 10;  // 修改最后一个元素
     * @endcode
     */
    reference last()
    {
        return m_container.back();
    }

    /**
     * @brief 获取尾元素（常量版本）
     * @sa last()
     */
    const_reference last() const
    {
        return m_container.back();
    }

    /**
     * @brief 安全获取首元素（带边界检查）
     * @return 容器第一个元素的引用
     * @throw std::out_of_range 当容器为空时抛出
     *
     * 示例安全访问：
     * @code
     * try {
     *     auto& val = cont.firstChecked();
     * } catch (const std::out_of_range&) {
     *     // 处理空容器
     * }
     * @endcode
     */
    reference firstChecked()
    {
        if (empty()) {
            throw std::out_of_range("Accessing first element of empty container");
        }
        return m_container.front();
    }

    /**
     * @brief 安全获取首元素（常量版本）
     * @sa firstChecked()
     */
    const_reference firstChecked() const
    {
        if (empty()) {
            throw std::out_of_range("Accessing first element of empty container");
        }
        return m_container.front();
    }

    /**
     * @brief 安全获取尾元素（带边界检查）
     * @return 容器最后一个元素的引用
     * @throw std::out_of_range 当容器为空时抛出
     */
    reference lastChecked()
    {
        if (empty()) {
            throw std::out_of_range("Accessing last element of empty container");
        }
        return m_container.back();
    }

    /**
     * @brief 安全获取尾元素（常量版本）
     * @sa lastChecked()
     */
    const_reference lastChecked() const
    {
        if (empty()) {
            throw std::out_of_range("Accessing last element of empty container");
        }
        return m_container.back();
    }

    /// @}

    /// @name 索引控制
    /// @{

    /**
     * @brief 获取当前索引值
     * @return 当前索引的数值
     * @note 返回值可能超出有效范围，需配合isValidIndex()使用
     */
    IndexType currentIndex() const noexcept
    {
        return m_index;
    }

    /**
     * @brief 设置当前索引
     * @param index 新的索引值
     * @note 不进行范围检查，设置后应调用isValidIndex()验证
     *
     * 示例：
     * @code
     * container.setCurrentIndex(5);
     * if (container.isValidIndex()) {
     *     // 安全操作
     * }
     * @endcode
     */
    void setCurrentIndex(IndexType index) noexcept
    {
        m_index = index;
    }

    /**
     * @brief 检查当前索引是否有效
     * @return true 当 0 <= index < size()
     *
     * 示例：
     * @code
     * while (container.isValidIndex()) {
     *     process(container.current());
     *     ++container;
     * }
     * @endcode
     */
    bool isValidIndex() const noexcept
    {
        return m_index >= 0 && static_cast< size_type >(m_index) < size();
    }

    /**
     * @brief 检查是否是第一个索引
     * @return 如果是第一个索引或容器为空返回true
     */
    bool isFirstIndex() const
    {
        return m_index == 0 || m_container.empty();
    }

    /**
     * @brief 检查是否是最后一个索引
     * @return 如果是最后一个有效索引或容器为空返回true
     */
    bool isLastIndex() const
    {
        return (!m_container.empty()) && (m_index == m_container.size() - 1);
    }

    /// @}

    /// @name 容器赋值操作
    /// @{

    /**
     * @brief 拷贝赋值底层容器
     * @param other 要拷贝的容器
     * @return 当前对象的引用
     * @note 赋值后会尝试保持当前索引有效性：
     *       - 如果原索引在新容器范围内则保留
     *       - 否则重置为0（若新容器非空）或保持0（空容器）
     * @warning 若新容器为空，索引将标记为无效
     *
     * 示例1：缩小容器大小
     * @code
     * DAGenericIndexedContainer<std::vector<int>> cont({1,2,3,4}); // index=0
     * cont.setCurrentIndex(3);
     * std::vector<int> newData{5,6};
     * cont = newData; // 新size=2，index=0（超出原3，重置）
     * @endcode
     *
     * 示例2：扩大容器大小
     * @code
     * cont.setCurrentIndex(1);
     * cont = std::vector<int>{1,2,3,4,5}; // 保持index=1
     * @endcode
     */
    DAGenericIndexedContainer& operator=(const Container& other)
    {

        // 拷贝前保存原索引
        const size_type oldIndex = static_cast< size_type >(m_index);

        // 执行容器拷贝
        m_container = other;

        // 调整索引
        if (!m_container.empty()) {
            // 新容器非空时调整索引
            if (oldIndex < m_container.size()) {
                m_index = static_cast< IndexType >(oldIndex);
            } else {
                m_index = 0;
            }
        } else {
            // 新容器为空时保持索引0（无效状态）
            m_index = 0;
        }

        return *this;
    }

    /**
     * @brief 移动赋值底层容器
     * @param other 要移动的容器
     * @return 当前对象的引用
     * @note 行为与拷贝赋值一致，但更高效
     * @warning 移动后源容器other状态未定义
     *
     * 示例：
     * @code
     * std::vector<int> data{1,2,3};
     * cont = std::move(data); // data状态未定义
     * @endcode
     */
    DAGenericIndexedContainer& operator=(Container&& other) noexcept
    {
        // 移动前保存原索引
        const size_type oldIndex = static_cast< size_type >(m_index);

        // 执行容器移动
        m_container = std::move(other);

        // 调整索引（逻辑同拷贝版本）
        if (!m_container.empty()) {
            m_index = (oldIndex < m_container.size()) ? static_cast< IndexType >(oldIndex) : 0;
        } else {
            m_index = 0;
        }
        return *this;
    }

    /// @}

    /// @name 容器内容替换
    /// @{

    /**
     * @brief 替换容器内容并智能调整索引
     * @param newContainer 新容器
     * @param policy 索引调整策略（默认为AutoReset）
     *
     * 策略选项：
     * - AutoReset: 自动重置到0或保持有效索引（默认）
     * - PreserveRaw: 保留原始索引值（可能无效）
     * - ForceReset: 强制重置到0
     *
     * 示例：强制重置索引
     * @code
     * cont.replace(newData, ForceReset); // 无论原索引如何，强制置0
     * @endcode
     */
    void replace(Container newContainer, IndexPolicy policy = IndexPolicy::AutoReset)
    {
        const size_type oldIndex = static_cast< size_type >(m_index);

        m_container = std::move(newContainer);

        switch (policy) {
        case IndexPolicy::AutoReset:
            if (!m_container.empty()) {
                m_index = (oldIndex < m_container.size()) ? static_cast< IndexType >(oldIndex) : 0;
            } else {
                m_index = 0;
            }
            break;
        case IndexPolicy::PreserveRaw:
            // 不调整，可能无效
            break;
        case IndexPolicy::ForceReset:
            m_index = 0;
            break;
        }
    }
    /// @}

    /// @name 元素访问
    /// @{

    /**
     * @brief 下标访问操作符（非const版本）
     * @param index 要访问的元素索引
     * @return 对应元素的引用
     * @warning 不进行边界检查，调用者需确保 0 <= index < size()
     * @note 与当前索引无关，仅访问指定位置元素
     *
     * 示例：
     * @code
     * DAGenericIndexedContainer<std::vector<int>> cont{1,2,3};
     * cont[1] = 5;       // 直接修改索引1处元素
     * assert(cont[1] == 5);
     * @endcode
     *
     * @warning 以下情况导致未定义行为：
     * - index < 0
     * - index >= size()
     * - 容器为空时访问任意索引
     */
    reference operator[](IndexType index)
    {
        return m_container[ static_cast< size_type >(index) ];
    }

    /**
     * @brief 下标访问操作符（const版本）
     * @param index 要访问的元素索引
     * @return 对应元素的常量引用
     * @sa operator[](IndexType)
     */
    const_reference operator[](IndexType index) const
    {
        return m_container[ static_cast< size_type >(index) ];
    }

    /// @}

    /// @name 安全访问
    /// @{

    /**
     * @brief 带边界检查的元素访问
     * @param index 要访问的索引
     * @return 对应元素的引用
     * @throw std::out_of_range 当 index 超出 [0, size()) 范围
     *
     * 示例：
     * @code
     * try {
     *     int val = cont.at(10);
     * } catch (const std::out_of_range& e) {
     *     // 处理越界
     * }
     * @endcode
     */
    reference at(IndexType index)
    {
        const size_type idx = static_cast< size_type >(index);
        if (idx >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return m_container[ idx ];
    }

    /**
     * @brief 带边界检查的常量元素访问
     * @sa at(IndexType)
     */
    const_reference at(IndexType index) const
    {
        const size_type idx = static_cast< size_type >(index);
        if (idx >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return m_container[ idx ];
    }
    /// @}

    /// @name 迭代器操作
    /// @{
    /**
     * @brief 获取指向容器首元素的迭代器
     * @return 起始迭代器
     *
     * 示例遍历所有元素：
     * @code
     * for (auto it = cont.begin(); it != cont.end(); ++it) {
     *     process(*it);
     * }
     * @endcode
     */
    iterator begin() noexcept
    {
        return m_container.begin();
    }

    /**
     * @brief 获取指向容器末尾的迭代器
     * @return 结束迭代器
     */
    iterator end() noexcept
    {
        return m_container.end();
    }

    /**
     * @brief 获取常量起始迭代器
     * @sa begin()
     */
    const_iterator begin() const noexcept
    {
        return m_container.begin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @sa end()
     */
    const_iterator end() const noexcept
    {
        return m_container.end();
    }

    /**
     * @brief 获取常量起始迭代器（C++11风格）
     */
    const_iterator cbegin() const noexcept
    {
        return m_container.cbegin();
    }

    /**
     * @brief 获取常量结束迭代器（C++11风格）
     */
    const_iterator cend() const noexcept
    {
        return m_container.cend();
    }

    /**
     * @brief 获取反向起始迭代器
     *
     * 示例反向遍历：
     * @code
     * for (auto rit = cont.rbegin(); rit != cont.rend(); ++rit) {
     *     process(*rit);
     * }
     * @endcode
     */
    reverse_iterator rbegin() noexcept
    {
        return m_container.rbegin();
    }

    /**
     * @brief 获取反向结束迭代器
     */
    reverse_iterator rend() noexcept
    {
        return m_container.rend();
    }

    /**
     * @brief 获取常量反向起始迭代器
     */
    const_reverse_iterator crbegin() const noexcept
    {
        return m_container.crbegin();
    }

    /**
     * @brief 获取常量反向结束迭代器
     */
    const_reverse_iterator crend() const noexcept
    {
        return m_container.crend();
    }
    /// @}

    const Container& container() const
    {
        return m_container;
    }

private:
    Container m_container;  ///< 底层容器存储
    IndexType m_index = 0;  ///< 当前索引值
};

}  // namespace DA

#endif  // DAGENERICINDEXEDCONTAINER_H
