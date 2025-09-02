#include "da_vector_table.hpp"  // 包含你的头文件
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// 测试辅助函数：比较两个表格是否相等
template< typename T >
bool tables_equal(const DA::da_vector_table< T >& a, const DA::da_vector_table< T >& b)
{
    if (a.row_count() != b.row_count() || a.column_count() != b.column_count()) {
        return false;
    }

    for (std::size_t i = 0; i < a.row_count(); ++i) {
        for (std::size_t j = 0; j < a.column_count(); ++j) {
            if (a(i, j) != b(i, j)) {
                return false;
            }
        }
    }

    return true;
}

// 测试辅助函数：打印表格内容
template< typename T >
void print_table(const DA::da_vector_table< T >& table, const std::string& name)
{
    std::cout << name << " (" << table.row_count() << "x" << table.column_count() << "):\n";
    for (std::size_t i = 0; i < table.row_count(); ++i) {
        for (std::size_t j = 0; j < table.column_count(); ++j) {
            std::cout << table(i, j) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

// 测试构造函数
void test_constructors()
{
    std::cout << "=== Testing Constructors ===\n";

    // 默认构造函数
    DA::da_vector_table< int > table1;
    assert(table1.row_count() == 0);
    assert(table1.column_count() == 0);
    assert(table1.empty());
    std::cout << "Default constructor: OK\n";

    // 指定大小的构造函数
    DA::da_vector_table< int > table2(3, 4);
    assert(table2.row_count() == 3);
    assert(table2.column_count() == 4);
    assert(!table2.empty());

    // 检查所有元素是否初始化为默认值
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            assert(table2(i, j) == int());
        }
    }
    std::cout << "Size constructor: OK\n";

    // 指定大小和初始值的构造函数
    DA::da_vector_table< int > table3(2, 3, 42);
    assert(table3.row_count() == 2);
    assert(table3.column_count() == 3);

    for (std::size_t i = 0; i < 2; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            assert(table3(i, j) == 42);
        }
    }
    std::cout << "Size + value constructor: OK\n";

    // 初始化列表构造函数
    DA::da_vector_table< int > table4 = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };

    assert(table4.row_count() == 3);
    assert(table4.column_count() == 3);

    int expected = 1;
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            assert(table4(i, j) == expected++);
        }
    }
    std::cout << "Initializer list constructor: OK\n";

    // 拷贝构造函数
    DA::da_vector_table< int > table5 = table4;
    assert(tables_equal(table4, table5));
    std::cout << "Copy constructor: OK\n";

    // 移动构造函数
    DA::da_vector_table< int > table6 = std::move(table5);
    assert(table6.row_count() == 3);
    assert(table6.column_count() == 3);
    assert(table5.empty());  // table5 应该被移动
    std::cout << "Move constructor: OK\n";

    std::cout << "All constructor tests passed!\n\n";
}

// 测试赋值操作符
void test_assignment()
{
    std::cout << "=== Testing Assignment Operators ===\n";

    DA::da_vector_table< int > table1 = { { 1, 2 }, { 3, 4 } };

    // 拷贝赋值
    DA::da_vector_table< int > table2;
    table2 = table1;
    assert(tables_equal(table1, table2));
    std::cout << "Copy assignment: OK\n";

    // 移动赋值
    DA::da_vector_table< int > table3;
    table3 = std::move(table2);
    assert(tables_equal(table1, table3));
    assert(table2.empty());  // table2 应该被移动
    std::cout << "Move assignment: OK\n";

    std::cout << "All assignment tests passed!\n\n";
}

// 测试元素访问
void test_element_access()
{
    std::cout << "=== Testing Element Access ===\n";

    DA::da_vector_table< int > table = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };

    // 测试 operator()
    assert(table(0, 0) == 1);
    assert(table(1, 1) == 5);
    assert(table(2, 2) == 9);
    std::cout << "Operator(): OK\n";

    // 测试 at()
    assert(table.at(0, 1) == 2);
    assert(table.at(2, 0) == 7);
    std::cout << "At(): OK\n";

    // 测试修改元素
    table(1, 1) = 55;
    assert(table(1, 1) == 55);
    std::cout << "Element modification: OK\n";

    // 测试边界检查异常
    try {
        table.at(3, 0);
        assert(false);  // 应该抛出异常
    } catch (const std::out_of_range&) {
        // 预期异常
    }

    try {
        table.at(0, 3);
        assert(false);  // 应该抛出异常
    } catch (const std::out_of_range&) {
        // 预期异常
    }
    std::cout << "Bounds checking: OK\n";

    // 测试 front() 和 back()
    assert(table.front() == 1);  // 第一个元素
    assert(table.back() == 9);   // 最后一个元素
    std::cout << "Front/Back: OK\n";

    // 测试 data()
    int* data_ptr = table.data();
    assert(data_ptr[ 0 ] == 1);
    assert(data_ptr[ 4 ] == 55);  // 修改后的元素
    std::cout << "Data pointer: OK\n";

    std::cout << "All element access tests passed!\n\n";
}

// 测试迭代器
void test_iterators()
{
    std::cout << "=== Testing Iterators ===\n";

    DA::da_vector_table< int > table = { { 1, 2 }, { 3, 4 } };

    // 测试正向迭代器
    int expected = 1;
    for (auto it = table.begin(); it != table.end(); ++it) {
        assert(*it == expected++);
    }
    std::cout << "Forward iterators: OK\n";

    // 测试常量迭代器
    const auto& const_table = table;
    expected                = 1;
    for (auto it = const_table.begin(); it != const_table.end(); ++it) {
        assert(*it == expected++);
    }
    std::cout << "Const iterators: OK\n";

    // 测试反向迭代器
    expected = 4;
    for (auto it = table.rbegin(); it != table.rend(); ++it) {
        assert(*it == expected--);
    }
    std::cout << "Reverse iterators: OK\n";

    std::cout << "All iterator tests passed!\n\n";
}

// 测试容量操作
void test_capacity()
{
    std::cout << "=== Testing Capacity Operations ===\n";

    DA::da_vector_table< int > table;

    // 测试 empty() 和 size()
    assert(table.empty());
    assert(table.size() == 0);
    std::cout << "Empty table: OK\n";

    // 测试 reserve() 和 capacity()
    table.reserve(100);
    assert(table.capacity() >= 100);
    std::cout << "Reserve: OK\n";

    // 测试 resize()
    table.resize(3, 4, 42);
    assert(table.row_count() == 3);
    assert(table.column_count() == 4);
    assert(table.size() == 12);

    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            assert(table(i, j) == 42);
        }
    }
    std::cout << "Resize: OK\n";

    // 测试 shrink_to_fit()
    table.shrink_to_fit();
    assert(table.capacity() == table.size());
    std::cout << "Shrink to fit: OK\n";

    // 测试 clear()
    table.clear();
    assert(table.empty());
    assert(table.row_count() == 0);
    assert(table.column_count() == 0);
    std::cout << "Clear: OK\n";

    std::cout << "All capacity tests passed!\n\n";
}

// 测试行操作
void test_row_operations()
{
    std::cout << "=== Testing Row Operations ===\n";

    DA::da_vector_table< int > table = { { 1, 2 }, { 3, 4 } };

    // 测试 append_row
    table.append_row({ 5, 6 });
    assert(table.row_count() == 3);
    assert(table(2, 0) == 5);
    assert(table(2, 1) == 6);
    std::cout << "Append row: OK\n";

    // 测试 insert_row
    table.insert_row(1, { 7, 8 });
    assert(table.row_count() == 4);
    assert(table(1, 0) == 7);
    assert(table(1, 1) == 8);
    assert(table(2, 0) == 3);  // 原来的第二行后移
    std::cout << "Insert row: OK\n";

    // 测试 erase_row
    table.erase_row(1);
    assert(table.row_count() == 3);
    assert(table(1, 0) == 3);  // 原来的第三行前移
    assert(table(1, 1) == 4);
    std::cout << "Erase row: OK\n";

    // 测试 get_row 和 set_row
    auto row = table.get_row(0);
    assert(row.size() == 2);
    assert(row[ 0 ] == 1);
    assert(row[ 1 ] == 2);

    table.set_row(2, { 9, 10 });
    assert(table(2, 0) == 9);
    assert(table(2, 1) == 10);
    std::cout << "Get/Set row: OK\n";

    std::cout << "All row operation tests passed!\n\n";
}

// 测试列操作
void test_column_operations()
{
    std::cout << "=== Testing Column Operations ===\n";

    DA::da_vector_table< int > table = { { 1, 2 }, { 3, 4 } };

    // 测试 append_column
    table.append_column({ 5, 6 });
    assert(table.column_count() == 3);
    assert(table(0, 2) == 5);
    assert(table(1, 2) == 6);
    std::cout << "Append column: OK\n";

    // 测试 insert_column
    table.insert_column(1, { 7, 8 });
    assert(table.column_count() == 4);
    assert(table(0, 1) == 7);
    assert(table(1, 1) == 8);
    assert(table(0, 2) == 2);  // 原来的第二列后移
    std::cout << "Insert column: OK\n";

    // 测试 erase_column
    table.erase_column(1);
    assert(table.column_count() == 3);
    assert(table(0, 1) == 2);  // 原来的第三列前移
    assert(table(1, 1) == 4);
    std::cout << "Erase column: OK\n";

    // 测试 get_column 和 set_column
    auto col = table.get_column(0);
    assert(col.size() == 2);
    assert(col[ 0 ] == 1);
    assert(col[ 1 ] == 3);

    table.set_column(2, { 9, 10 });
    assert(table(0, 2) == 9);
    assert(table(1, 2) == 10);
    std::cout << "Get/Set column: OK\n";

    std::cout << "All column operation tests passed!\n\n";
}

// 测试形状操作
void test_shape_operations()
{
    std::cout << "=== Testing Shape Operations ===\n";

    DA::da_vector_table< int > table = { { 1, 2, 3 }, { 4, 5, 6 } };

    // 测试 shape()
    auto shape = table.shape();
    assert(shape.first == 2);
    assert(shape.second == 3);
    std::cout << "Shape: OK\n";

    // 测试 row_count() 和 column_count()
    assert(table.row_count() == 2);
    assert(table.column_count() == 3);
    std::cout << "Row/Column count: OK\n";

    // 测试 resize()
    table.resize(3, 4, 0);
    assert(table.row_count() == 3);
    assert(table.column_count() == 4);

    // 检查原有数据是否保留
    assert(table(0, 0) == 1);
    assert(table(0, 1) == 2);
    assert(table(0, 2) == 3);
    assert(table(1, 0) == 4);
    assert(table(1, 1) == 5);
    assert(table(1, 2) == 6);

    // 检查新元素是否初始化为0
    assert(table(2, 0) == 0);
    assert(table(2, 3) == 0);
    assert(table(0, 3) == 0);
    std::cout << "Resize: OK\n";

    // 测试 reshape()
    DA::da_vector_table< int > table2 = { { 1, 2, 3, 4, 5, 6 } };
    table2.reshape(2, 3);
    assert(table2.row_count() == 2);
    assert(table2.column_count() == 3);
    assert(table2(0, 0) == 1);
    assert(table2(0, 1) == 2);
    assert(table2(0, 2) == 3);
    assert(table2(1, 0) == 4);
    assert(table2(1, 1) == 5);
    assert(table2(1, 2) == 6);
    std::cout << "Reshape: OK\n";

    std::cout << "All shape operation tests passed!\n\n";
}

// 测试其他功能
void test_other_features()
{
    std::cout << "=== Testing Other Features ===\n";

    // 测试 is_rectangular() - 一维存储总是矩形
    DA::da_vector_table< int > table = { { 1, 2, 3 }, { 4, 5, 6 } };
    assert(table.is_rectangular());
    std::cout << "Is rectangular: OK\n";

    // 测试 swap()
    DA::da_vector_table< int > table1 = { { 1, 2 }, { 3, 4 } };
    DA::da_vector_table< int > table2 = { { 5, 6, 7 }, { 8, 9, 10 } };

    table1.swap(table2);

    assert(table1.row_count() == 2);
    assert(table1.column_count() == 3);
    assert(table1(0, 0) == 5);

    assert(table2.row_count() == 2);
    assert(table2.column_count() == 2);
    assert(table2(0, 0) == 1);
    std::cout << "Swap: OK\n";

    // 测试不同类型
    DA::da_vector_table< std::string > string_table(2, 2, "hello");
    assert(string_table(0, 0) == "hello");
    assert(string_table(1, 1) == "hello");
    std::cout << "Different types: OK\n";

    std::cout << "All other feature tests passed!\n\n";
}

// 主测试函数
int main()
{
    std::cout << "Starting da_vector_table unit tests...\n\n";

    try {
        test_constructors();
        test_assignment();
        test_element_access();
        test_iterators();
        test_capacity();
        test_row_operations();
        test_column_operations();
        test_shape_operations();
        test_other_features();

        std::cout << "All tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception\n";
        return 1;
    }
}
