#include "da_vector_table.hpp"  // 包含你的头文件
#include "DATable.hpp"
#include <QtTest/QtTest>
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

class DASharedTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    // da_vector_table 测试
    void testConstructors();
    void testAssignmentOperators();
    void testElementAccess();
    void testIterators();
    void testCapacityOperations();
    void testRowOperations();
    void testColumnOperations();
    void testShapeOperations();
    void testOtherFeatures();
    // DATable 测试
    void testDATableDefaultConstructor();
    void testDATableSetGetValues();
    void testDATableCopyConstructorAndAssignment();
    void testDATableMoveConstructorAndAssignment();
    void testDATableRemoveCells();
    void testDATableClear();
    void testDATableIterators();
    void testDATableFind();
    void testDATableEraseIf();
    void testDATableDropColumn();
    void testDATableTransfered();
    void testDATableTransferColumn();
    void testDATableAssignmentFromVectorTable();
    void testDATableExceptions();
    void testDATableRecalcShape();
};

void DASharedTest::testConstructors()
{
    // 默认构造函数
    DA::da_vector_table< int > table1;
    QCOMPARE(table1.row_count(), static_cast< std::size_t >(0));
    QCOMPARE(table1.column_count(), static_cast< std::size_t >(0));
    QVERIFY(table1.empty());

    // 指定大小的构造函数
    DA::da_vector_table< int > table2(3, 4);
    QCOMPARE(table2.row_count(), static_cast< std::size_t >(3));
    QCOMPARE(table2.column_count(), static_cast< std::size_t >(4));
    QVERIFY(!table2.empty());

    // 检查所有元素是否初始化为默认值
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            QCOMPARE(table2(i, j), int());
        }
    }

    // 指定大小和初始值的构造函数
    DA::da_vector_table< int > table3(2, 3, 42);
    QCOMPARE(table3.row_count(), static_cast< std::size_t >(2));
    QCOMPARE(table3.column_count(), static_cast< std::size_t >(3));

    for (std::size_t i = 0; i < 2; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            QCOMPARE(table3(i, j), 42);
        }
    }

    // 初始化列表构造函数
    DA::da_vector_table< int > table4 = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };

    QCOMPARE(table4.row_count(), static_cast< std::size_t >(3));
    QCOMPARE(table4.column_count(), static_cast< std::size_t >(3));

    int expected = 1;
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            QCOMPARE(table4(i, j), expected++);
        }
    }

    // 拷贝构造函数
    DA::da_vector_table< int > table5 = table4;
    QVERIFY(tables_equal(table4, table5));

    // 移动构造函数
    DA::da_vector_table< int > table6 = std::move(table5);
    QCOMPARE(table6.row_count(), static_cast< std::size_t >(3));
    QCOMPARE(table6.column_count(), static_cast< std::size_t >(3));
    QVERIFY(table5.empty());  // table5 应该被移动
}

void DASharedTest::testAssignmentOperators()
{
    DA::da_vector_table< int > table1 = { { 1, 2 }, { 3, 4 } };

    // 拷贝赋值
    DA::da_vector_table< int > table2;
    table2 = table1;
    QVERIFY(tables_equal(table1, table2));

    // 移动赋值
    DA::da_vector_table< int > table3;
    table3 = std::move(table2);
    QVERIFY(tables_equal(table1, table3));
    QVERIFY(table2.empty());  // table2 应该被移动
}

void DASharedTest::testElementAccess()
{
    DA::da_vector_table< int > table = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };

    // 测试 operator()
    QCOMPARE(table(0, 0), 1);
    QCOMPARE(table(1, 1), 5);
    QCOMPARE(table(2, 2), 9);

    // 测试 at()
    QCOMPARE(table.at(0, 1), 2);
    QCOMPARE(table.at(2, 0), 7);

    // 测试修改元素
    table(1, 1) = 55;
    QCOMPARE(table(1, 1), 55);

    // 测试边界检查异常
    QVERIFY_EXCEPTION_THROWN(table.at(3, 0), std::out_of_range);
    QVERIFY_EXCEPTION_THROWN(table.at(0, 3), std::out_of_range);

    // 测试 front() 和 back()
    QCOMPARE(table.front(), 1);  // 第一个元素
    QCOMPARE(table.back(), 9);   // 最后一个元素

    // 测试 data()
    int* data_ptr = table.data();
    QCOMPARE(data_ptr[ 0 ], 1);
    QCOMPARE(data_ptr[ 4 ], 55);  // 修改后的元素
}

void DASharedTest::testIterators()
{
    DA::da_vector_table< int > table = { { 1, 2 }, { 3, 4 } };

    // 测试正向迭代器
    int expected = 1;
    for (auto it = table.begin(); it != table.end(); ++it) {
        QCOMPARE(*it, expected++);
    }

    // 测试常量迭代器
    const auto& const_table = table;
    expected                = 1;
    for (auto it = const_table.begin(); it != const_table.end(); ++it) {
        QCOMPARE(*it, expected++);
    }

    // 测试反向迭代器
    expected = 4;
    for (auto it = table.rbegin(); it != table.rend(); ++it) {
        QCOMPARE(*it, expected--);
    }
}

void DASharedTest::testCapacityOperations()
{
    DA::da_vector_table< int > table;

    // 测试 empty() 和 size()
    QVERIFY(table.empty());
    QCOMPARE(table.size(), static_cast< std::size_t >(0));

    // 测试 reserve() 和 capacity()
    table.reserve(100);
    QVERIFY(table.capacity() >= static_cast< std::size_t >(100));

    // 测试 resize()
    table.resize(3, 4, 42);
    QCOMPARE(table.row_count(), static_cast< std::size_t >(3));
    QCOMPARE(table.column_count(), static_cast< std::size_t >(4));
    QCOMPARE(table.size(), static_cast< std::size_t >(12));

    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            QCOMPARE(table(i, j), 42);
        }
    }

    // 测试 shrink_to_fit()
    // shrink_to_fit 是非绑定请求，标准不保证容量精确收缩到 size。
    // 改为验证收缩后数据完整性（元素值未改变），而非断言容量精确值
    table.shrink_to_fit();
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            QCOMPARE(table(i, j), 42);
        }
    }

    // 测试 clear()
    table.clear();
    QVERIFY(table.empty());
    QCOMPARE(table.row_count(), static_cast< std::size_t >(0));
    QCOMPARE(table.column_count(), static_cast< std::size_t >(0));
}

void DASharedTest::testRowOperations()
{
    DA::da_vector_table< int > table = { { 1, 2 }, { 3, 4 } };

    // 测试 append_row
    table.append_row({ 5, 6 });
    QCOMPARE(table.row_count(), static_cast< std::size_t >(3));
    QCOMPARE(table(2, 0), 5);
    QCOMPARE(table(2, 1), 6);

    // 测试 insert_row
    table.insert_row(1, { 7, 8 });
    QCOMPARE(table.row_count(), static_cast< std::size_t >(4));
    QCOMPARE(table(1, 0), 7);
    QCOMPARE(table(1, 1), 8);
    QCOMPARE(table(2, 0), 3);  // 原来的第二行后移

    // 测试 erase_row
    table.erase_row(1);
    QCOMPARE(table.row_count(), static_cast< std::size_t >(3));
    QCOMPARE(table(1, 0), 3);  // 原来的第三行前移
    QCOMPARE(table(1, 1), 4);

    // 测试 get_row 和 set_row
    auto row = table.get_row(0);
    QCOMPARE(row.size(), static_cast< std::size_t >(2));
    QCOMPARE(row[ 0 ], 1);
    QCOMPARE(row[ 1 ], 2);

    table.set_row(2, { 9, 10 });
    QCOMPARE(table(2, 0), 9);
    QCOMPARE(table(2, 1), 10);
}

void DASharedTest::testColumnOperations()
{
    DA::da_vector_table< int > table = { { 1, 2 }, { 3, 4 } };

    // 测试 append_column
    table.append_column({ 5, 6 });
    QCOMPARE(table.column_count(), static_cast< std::size_t >(3));
    QCOMPARE(table(0, 2), 5);
    QCOMPARE(table(1, 2), 6);

    // 测试 insert_column
    table.insert_column(1, { 7, 8 });
    QCOMPARE(table.column_count(), static_cast< std::size_t >(4));
    QCOMPARE(table(0, 1), 7);
    QCOMPARE(table(1, 1), 8);
    QCOMPARE(table(0, 2), 2);  // 原来的第二列后移

    // 测试 erase_column
    table.erase_column(1);
    QCOMPARE(table.column_count(), static_cast< std::size_t >(3));
    QCOMPARE(table(0, 1), 2);  // 原来的第三列前移
    QCOMPARE(table(1, 1), 4);

    // 测试 get_column 和 set_column
    auto col = table.get_column(0);
    QCOMPARE(col.size(), static_cast< std::size_t >(2));
    QCOMPARE(col[ 0 ], 1);
    QCOMPARE(col[ 1 ], 3);

    table.set_column(2, { 9, 10 });
    QCOMPARE(table(0, 2), 9);
    QCOMPARE(table(1, 2), 10);
}

void DASharedTest::testShapeOperations()
{
    DA::da_vector_table< int > table = { { 1, 2, 3 }, { 4, 5, 6 } };

    // 测试 shape()
    auto shape = table.shape();
    QCOMPARE(shape.first, static_cast< std::size_t >(2));
    QCOMPARE(shape.second, static_cast< std::size_t >(3));

    // 测试 row_count() 和 column_count()
    QCOMPARE(table.row_count(), static_cast< std::size_t >(2));
    QCOMPARE(table.column_count(), static_cast< std::size_t >(3));

    // 测试 resize()
    table.resize(3, 4, 0);
    QCOMPARE(table.row_count(), static_cast< std::size_t >(3));
    QCOMPARE(table.column_count(), static_cast< std::size_t >(4));

    // 检查原有数据是否保留
    QCOMPARE(table(0, 0), 1);
    QCOMPARE(table(0, 1), 2);
    QCOMPARE(table(0, 2), 3);
    QCOMPARE(table(1, 0), 4);
    QCOMPARE(table(1, 1), 5);
    QCOMPARE(table(1, 2), 6);

    // 检查新元素是否初始化为0
    QCOMPARE(table(2, 0), 0);
    QCOMPARE(table(2, 3), 0);
    QCOMPARE(table(0, 3), 0);

    // 测试 reshape()
    DA::da_vector_table< int > table2 = { { 1, 2, 3, 4, 5, 6 } };
    table2.reshape(2, 3);
    QCOMPARE(table2.row_count(), static_cast< std::size_t >(2));
    QCOMPARE(table2.column_count(), static_cast< std::size_t >(3));
    QCOMPARE(table2(0, 0), 1);
    QCOMPARE(table2(0, 1), 2);
    QCOMPARE(table2(0, 2), 3);
    QCOMPARE(table2(1, 0), 4);
    QCOMPARE(table2(1, 1), 5);
    QCOMPARE(table2(1, 2), 6);
}

void DASharedTest::testOtherFeatures()
{
    // 测试 is_rectangular() - 一维存储总是矩形
    DA::da_vector_table< int > table = { { 1, 2, 3 }, { 4, 5, 6 } };
    QVERIFY(table.is_rectangular());

    // 测试 swap()
    DA::da_vector_table< int > table1 = { { 1, 2 }, { 3, 4 } };
    DA::da_vector_table< int > table2 = { { 5, 6, 7 }, { 8, 9, 10 } };

    table1.swap(table2);

    QCOMPARE(table1.row_count(), static_cast< std::size_t >(2));
    QCOMPARE(table1.column_count(), static_cast< std::size_t >(3));
    QCOMPARE(table1(0, 0), 5);

    QCOMPARE(table2.row_count(), static_cast< std::size_t >(2));
    QCOMPARE(table2.column_count(), static_cast< std::size_t >(2));
    QCOMPARE(table2(0, 0), 1);

    // 测试不同类型
    DA::da_vector_table< std::string > string_table(2, 2, "hello");
    QCOMPARE(string_table(0, 0), std::string("hello"));
    QCOMPARE(string_table(1, 1), std::string("hello"));
}

void DASharedTest::testDATableDefaultConstructor()
{
    DA::DATable< int > table;
    QVERIFY(table.empty());
    QCOMPARE(table.size(), static_cast< size_t >(0));
    QCOMPARE(table.rowCount(), 0);
    QCOMPARE(table.columnCount(), 0);
    QCOMPARE(table.shape(), std::make_pair(0, 0));
}

void DASharedTest::testDATableSetGetValues()
{
    DA::DATable< int > table;
    table.set(0, 0, 42);
    QVERIFY(table.contain(0, 0));
    QCOMPARE(table.at(0, 0), 42);
    QCOMPARE(table.cell(0, 0), 42);
    QCOMPARE(table.cell(1, 1), 0);  // default value

    table.set({ 1, 1 }, 100);
    QVERIFY(table.contain({ 1, 1 }));
    QCOMPARE(table.at({ 1, 1 }), 100);

    table[ { 2, 2 } ] = 200;
    QVERIFY(table.contain(2, 2));
    QCOMPARE(table(2, 2), 200);

    QCOMPARE(table.size(), static_cast< size_t >(3));
    QCOMPARE(table.rowCount(), 3);
    QCOMPARE(table.columnCount(), 3);
    QCOMPARE(table.shape(), std::make_pair(3, 3));
}

void DASharedTest::testDATableCopyConstructorAndAssignment()
{
    DA::DATable< int > table1;
    table1.set(0, 0, 42);
    table1.set(1, 1, 100);

    DA::DATable< int > table2(table1);
    QVERIFY(table2.contain(0, 0));
    QVERIFY(table2.contain(1, 1));
    QCOMPARE(table2.at(0, 0), 42);
    QCOMPARE(table2.at(1, 1), 100);

    DA::DATable< int > table3;
    table3 = table1;
    QVERIFY(table3.contain(0, 0));
    QVERIFY(table3.contain(1, 1));
    QCOMPARE(table3.at(0, 0), 42);
    QCOMPARE(table3.at(1, 1), 100);
}

void DASharedTest::testDATableMoveConstructorAndAssignment()
{
    DA::DATable< int > table1;
    table1.set(0, 0, 42);
    table1.set(1, 1, 100);

    DA::DATable< int > table2(std::move(table1));
    QVERIFY(table2.contain(0, 0));
    QVERIFY(table2.contain(1, 1));
    QCOMPARE(table2.at(0, 0), 42);
    QCOMPARE(table2.at(1, 1), 100);
    QVERIFY(table1.empty());  // table1 should be empty after move

    DA::DATable< int > table3;
    table3 = std::move(table2);
    QVERIFY(table3.contain(0, 0));
    QVERIFY(table3.contain(1, 1));
    QCOMPARE(table3.at(0, 0), 42);
    QCOMPARE(table3.at(1, 1), 100);
    QVERIFY(table2.empty());  // table2 should be empty after move
}

void DASharedTest::testDATableRemoveCells()
{
    DA::DATable< int > table;
    table.set(0, 0, 42);
    table.set(1, 1, 100);
    table.set(2, 2, 200);

    QVERIFY(table.removeCell(1, 1));
    QVERIFY(!table.contain(1, 1));
    QCOMPARE(table.size(), static_cast< size_t >(2));

    QVERIFY(table.removeCell({ 0, 0 }));
    QVERIFY(!table.contain(0, 0));
    QCOMPARE(table.size(), static_cast< size_t >(1));

    QVERIFY(!table.removeCell(5, 5));  // non-existent cell
}

void DASharedTest::testDATableClear()
{
    DA::DATable< int > table;
    table.set(0, 0, 42);
    table.set(1, 1, 100);

    table.clear();
    QVERIFY(table.empty());
    QCOMPARE(table.size(), static_cast< size_t >(0));
    QCOMPARE(table.rowCount(), 0);
    QCOMPARE(table.columnCount(), 0);
}

void DASharedTest::testDATableIterators()
{
    DA::DATable< int > table;
    table.set(0, 0, 42);
    table.set(1, 1, 100);
    table.set(2, 2, 200);

    int count = 0;
    int sum   = 0;
    for (auto it = table.begin(); it != table.end(); ++it) {
        count++;
        sum += it->second;
    }
    QCOMPARE(count, 3);
    QCOMPARE(sum, 342);

    count = 0;
    for (const auto& item : table) {
        Q_UNUSED(item)
        count++;
    }
    QCOMPARE(count, 3);
}

void DASharedTest::testDATableFind()
{
    DA::DATable< int > table;
    table.set(0, 0, 42);
    table.set(1, 1, 100);

    auto it1 = table.find(0, 0);
    QVERIFY(it1 != table.end());
    QCOMPARE(it1->second, 42);

    auto it2 = table.find({ 1, 1 });
    QVERIFY(it2 != table.end());
    QCOMPARE(it2->second, 100);

    auto it3 = table.find(5, 5);
    QVERIFY(it3 == table.end());
}

void DASharedTest::testDATableEraseIf()
{
    DA::DATable< int > table;
    table.set(0, 0, 42);
    table.set(1, 1, 100);
    table.set(2, 2, 200);
    table.set(3, 3, 42);

    size_t removed = table.erase_if(
        [](const std::pair< std::pair< int, int >, int >& item) { return item.second == 42; });

    QCOMPARE(removed, static_cast< size_t >(2));
    QCOMPARE(table.size(), static_cast< size_t >(2));
    QVERIFY(!table.contain(0, 0));
    QVERIFY(table.contain(1, 1));
    QVERIFY(table.contain(2, 2));
    QVERIFY(!table.contain(3, 3));
}

void DASharedTest::testDATableDropColumn()
{
    DA::DATable< int > table;
    table.set(0, 0, 1);
    table.set(0, 1, 2);
    table.set(0, 2, 3);
    table.set(1, 0, 4);
    table.set(1, 1, 5);
    table.set(1, 2, 6);

    table.dropColumn(1);

    QVERIFY(table.contain(0, 0));
    QVERIFY(table.contain(0, 1));  // originally column 2
    QVERIFY(!table.contain(0, 2));

    QVERIFY(table.contain(1, 0));
    QVERIFY(table.contain(1, 1));  // originally column 2
    QVERIFY(!table.contain(1, 2));

    QCOMPARE(table.at(0, 0), 1);
    QCOMPARE(table.at(0, 1), 3);  // originally column 2
    QCOMPARE(table.at(1, 0), 4);
    QCOMPARE(table.at(1, 1), 6);  // originally column 2

    QCOMPARE(table.columnCount(), 2);
}

void DASharedTest::testDATableTransfered()
{
    DA::DATable< int > table;
    table.set(0, 0, 42);
    table.set(1, 1, 100);

    auto doubleTable = table.transfered< double >([](const int& v) { return v * 2.0; });

    QVERIFY(doubleTable.contain(0, 0));
    QVERIFY(doubleTable.contain(1, 1));
    QCOMPARE(doubleTable.at(0, 0), 84.0);
    QCOMPARE(doubleTable.at(1, 1), 200.0);
}

void DASharedTest::testDATableTransferColumn()
{
    DA::DATable< int > table;
    table.set(0, 0, 1);
    table.set(1, 0, 2);
    table.set(2, 0, 3);
    table.set(0, 1, 4);
    table.set(1, 1, 5);
    table.set(2, 1, 6);

    std::vector< int > colValues;
    table.transferColumn(0, [ &colValues ](const int& v) {
        colValues.push_back(v);
        return true;
    });

    QCOMPARE(colValues.size(), static_cast< size_t >(3));
    QCOMPARE(colValues[ 0 ], 1);
    QCOMPARE(colValues[ 1 ], 2);
    QCOMPARE(colValues[ 2 ], 3);
}

void DASharedTest::testDATableAssignmentFromVectorTable()
{
    DA::da_vector_table< int > vecTable(2, 3);
    vecTable(0, 0) = 1;
    vecTable(0, 1) = 2;
    vecTable(0, 2) = 3;
    vecTable(1, 0) = 4;
    vecTable(1, 1) = 5;
    vecTable(1, 2) = 6;

    DA::DATable< int > table;
    table = vecTable;

    QCOMPARE(table.rowCount(), 2);
    QCOMPARE(table.columnCount(), 3);
    QCOMPARE(table.at(0, 0), 1);
    QCOMPARE(table.at(0, 1), 2);
    QCOMPARE(table.at(0, 2), 3);
    QCOMPARE(table.at(1, 0), 4);
    QCOMPARE(table.at(1, 1), 5);
    QCOMPARE(table.at(1, 2), 6);
}

void DASharedTest::testDATableExceptions()
{
    DA::DATable< int > table;
    QVERIFY_EXCEPTION_THROWN(table.at(0, 0), std::out_of_range);

    table.set(0, 0, 42);
    QCOMPARE(table.at(0, 0), 42);
}

void DASharedTest::testDATableRecalcShape()
{
    DA::DATable< int > table;
    table.set(0, 0, 1);
    table.set(2, 2, 9);

    QCOMPARE(table.rowCount(), 3);
    QCOMPARE(table.columnCount(), 3);

    // Directly manipulate internal data
    table.rawData()[ { 1, 1 } ] = 5;
    table.rawData()[ { 3, 3 } ] = 10;

    // Shape should not be updated until recalcShape is called
    QCOMPARE(table.rowCount(), 3);
    QCOMPARE(table.columnCount(), 3);

    table.recalcShape();
    QCOMPARE(table.rowCount(), 4);
    QCOMPARE(table.columnCount(), 4);
}

QTEST_APPLESS_MAIN(DASharedTest)
#include "main.moc"
