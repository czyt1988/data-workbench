#include <QApplication>
#include <QtTest/QtTest>
#include "tst_dapynodemetadata.h"
#include "tst_linkpoints.h"
#include "tst_param_type_registry.h"
#include "tst_node_param_setting_panel.h"
#include "tst_node_param_setting_factory.h"
#include "tst_node_param_setting_widget.h"

// 测试入口
int main(int argc, char* argv[])
{
    // 部分测试需要 QApplication（如 DAParamTypeRegistry 创建 QWidget）
    QApplication app(argc, argv);

    // 运行所有 QTest 测试类
    int status = 0;

#define RUN_TEST(TestClass)                                       \
    {                                                             \
        TestClass test;                                           \
        status |= QTest::qExec(&test, argc, argv);                \
    }

    RUN_TEST(DA::TestDAPyNodeMetaData);
    RUN_TEST(DA::TestLinkPoints);
    RUN_TEST(DA::TestDAParamTypeRegistry);
    RUN_TEST(DA::TestNodeParamSettingPanel);
    RUN_TEST(DA::TestNodeParamSettingFactory);
    RUN_TEST(DA::TestDANodeParamSettingPanelWidget);

    return status;
}
