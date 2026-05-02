#include <QtTest/QtTest>
#include "tst_dapynodemetadata.h"
#include "tst_linkpoints.h"

// 测试入口
int main(int argc, char* argv[])
{
    // 运行所有 QTest 测试类
    int status = 0;

#define RUN_TEST(TestClass)                                       \
    {                                                             \
        TestClass test;                                           \
        status |= QTest::qExec(&test, argc, argv);                \
    }

    RUN_TEST(DA::TestDAPyNodeMetaData);
    RUN_TEST(DA::TestLinkPoints);

    return status;
}
