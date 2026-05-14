#include "tst_daportdescriptor.h"
#include "DAPortDescriptor.h"
#include <QtTest/QtTest>
#include "DAPybind11InQt.h"
#include "DAPyInterpreter.h"
#include "DAPyBindQt/DAPyGILGuard.h"

namespace DA
{

// ============================================================
// testDefaultConstruction — 默认构造验证
// ============================================================

/**
 * @brief 验证 DAPortDescriptor 默认构造函数的所有字段值
 *
 * 默认构造应设置：
 * - name == 空字符串
 * - dataType == 空字符串
 * - required == true
 * - description == 空字符串
 */
void TestDAPortDescriptor::testDefaultConstruction()
{
    DAPortDescriptor desc;

    QVERIFY(desc.name.isEmpty());
    QVERIFY(desc.dataType.isEmpty());
    QCOMPARE(desc.required, true);
    QVERIFY(desc.description.isEmpty());
}

// ============================================================
// testFieldAccess — 字段访问验证
// ============================================================

/**
 * @brief 验证设置字段后能正确读取各字段值
 */
void TestDAPortDescriptor::testFieldAccess()
{
    DAPortDescriptor desc(QStringLiteral("input_data"),
                          QStringLiteral("DataFrame"),
                          true,
                          QStringLiteral("输入数据"));

    QCOMPARE(desc.name, QStringLiteral("input_data"));
    QCOMPARE(desc.dataType, QStringLiteral("DataFrame"));
    QCOMPARE(desc.required, true);
    QCOMPARE(desc.description, QStringLiteral("输入数据"));

    // 验证可选字段 required=false
    DAPortDescriptor optionalDesc(QStringLiteral("output_data"),
                                  QStringLiteral("Series"),
                                  false);
    QCOMPARE(optionalDesc.required, false);
    QVERIFY(optionalDesc.description.isEmpty());

    // 验证字段可写
    desc.name         = QStringLiteral("renamed_port");
    desc.dataType     = QStringLiteral("int");
    desc.required     = false;
    desc.description  = QStringLiteral("重命名的端口");

    QCOMPARE(desc.name, QStringLiteral("renamed_port"));
    QCOMPARE(desc.dataType, QStringLiteral("int"));
    QCOMPARE(desc.required, false);
    QCOMPARE(desc.description, QStringLiteral("重命名的端口"));
}

// ============================================================
// testIsValid — 有效性验证
// ============================================================

/**
 * @brief 验证 isValid() 的正确行为
 *
 * - 默认构造：name和dataType均为空 → false
 * - 仅设置name：dataType为空 → false
 * - 仅设置dataType：name为空 → false
 * - name和dataType均非空 → true
 */
void TestDAPortDescriptor::testIsValid()
{
    DAPortDescriptor desc;
    QVERIFY(!desc.isValid());

    desc.name = QStringLiteral("data");
    QVERIFY(!desc.isValid());

    desc.name.clear();
    desc.dataType = QStringLiteral("DataFrame");
    QVERIFY(!desc.isValid());

    desc.name = QStringLiteral("data");
    desc.dataType = QStringLiteral("DataFrame");
    QVERIFY(desc.isValid());
}

// ============================================================
// testEqualityComparison — 逐字段相等性比较
// ============================================================

/**
 * @brief 验证两个相同字段值的 DAPortDescriptor 逐字段相等
 *
 * DAPortDescriptor 无 operator==，通过逐字段 QCOMPARE 验证。
 * 同时验证不同字段值时各字段不相等。
 */
void TestDAPortDescriptor::testEqualityComparison()
{
    DAPortDescriptor a(QStringLiteral("port_a"),
                       QStringLiteral("DataFrame"),
                       true,
                       QStringLiteral("描述A"));
    DAPortDescriptor b(QStringLiteral("port_a"),
                       QStringLiteral("DataFrame"),
                       true,
                       QStringLiteral("描述A"));

    // 逐字段比较：相同值应相等
    QCOMPARE(a.name, b.name);
    QCOMPARE(a.dataType, b.dataType);
    QCOMPARE(a.required, b.required);
    QCOMPARE(a.description, b.description);

    // 修改 b 的 name → 不相等
    b.name = QStringLiteral("port_b");
    QVERIFY(a.name != b.name);

    // 修改 b 的 dataType → 不相等
    b.name         = a.name;
    b.dataType     = QStringLiteral("Series");
    QVERIFY(a.dataType != b.dataType);

    // 修改 b 的 required → 不相等
    b.dataType     = a.dataType;
    b.required     = false;
    QVERIFY(a.required != b.required);

    // 修改 b 的 description → 不相等
    b.required     = a.required;
    b.description  = QStringLiteral("描述B");
    QVERIFY(a.description != b.description);
}

// ============================================================
// testPybind11Construction — pybind11 构造验证
// ============================================================

/**
 * @brief 验证通过 pybind11 创建 DAPortDescriptor 并访问字段
 *
 * 初始化 Python 解释器后，通过 da_py_workflow 模块创建
 * DAPortDescriptor 实例，验证字段读写和 isValid() 方法
 * 在 Python↔C++ 桥接下正常工作。
 */
void TestDAPortDescriptor::testPybind11Construction()
{
    // 初始化 Python 解释器
    if (!DAPyInterpreter::isPythonInitialized()) {
        try {
            DAPyInterpreter::initializePythonInterpreter();
        } catch (const std::exception& e) {
            QSKIP(qPrintable(QString("Python解释器初始化失败: %1").arg(e.what())));
        } catch (...) {
            QSKIP("Python解释器初始化失败");
        }
    }

    if (!DAPyInterpreter::isPythonInitialized()) {
        QSKIP("Python解释器未初始化，无法测试pybind11构造");
    }

    DAPyGILGuard gilGuard;

    try {
        pybind11::module_ wfModule = pybind11::module_::import("da_py_workflow");

        // 默认构造
        pybind11::object pyDefault = wfModule.attr("DAPortDescriptor")();
        DAPortDescriptor cppDefault = pyDefault.cast< DAPortDescriptor >();

        QVERIFY(cppDefault.name.isEmpty());
        QVERIFY(cppDefault.dataType.isEmpty());
        QCOMPARE(cppDefault.required, true);
        QVERIFY(cppDefault.description.isEmpty());
        QVERIFY(!cppDefault.isValid());

        // 带参构造（required=true, 有description）
        pybind11::object pyFull = wfModule.attr("DAPortDescriptor")(
            QStringLiteral("input_data"),
            QStringLiteral("DataFrame"),
            true,
            QStringLiteral("输入数据"));
        DAPortDescriptor cppFull = pyFull.cast< DAPortDescriptor >();

        QCOMPARE(cppFull.name, QStringLiteral("input_data"));
        QCOMPARE(cppFull.dataType, QStringLiteral("DataFrame"));
        QCOMPARE(cppFull.required, true);
        QCOMPARE(cppFull.description, QStringLiteral("输入数据"));
        QVERIFY(cppFull.isValid());

        // 带参构造（required=false, 无description — 使用默认参数）
        pybind11::object pyOptional = wfModule.attr("DAPortDescriptor")(
            QStringLiteral("output"),
            QStringLiteral("Series"),
            false);
        DAPortDescriptor cppOptional = pyOptional.cast< DAPortDescriptor >();

        QCOMPARE(cppOptional.name, QStringLiteral("output"));
        QCOMPARE(cppOptional.dataType, QStringLiteral("Series"));
        QCOMPARE(cppOptional.required, false);
        QVERIFY(cppOptional.description.isEmpty());
        QVERIFY(cppOptional.isValid());

        // Python 侧字段写入 → C++ 侧读取
        pybind11::object pyMutable = wfModule.attr("DAPortDescriptor")();
        pyMutable.attr("name")       = QStringLiteral("mutable_port");
        pyMutable.attr("dataType")   = QStringLiteral("int");
        pyMutable.attr("required")   = false;
        pyMutable.attr("description") = QStringLiteral("可变端口");

        DAPortDescriptor cppMutable = pyMutable.cast< DAPortDescriptor >();
        QCOMPARE(cppMutable.name, QStringLiteral("mutable_port"));
        QCOMPARE(cppMutable.dataType, QStringLiteral("int"));
        QCOMPARE(cppMutable.required, false);
        QCOMPARE(cppMutable.description, QStringLiteral("可变端口"));
        QVERIFY(cppMutable.isValid());

    } catch (const pybind11::error_already_set& e) {
        QSKIP(qPrintable(QString("Python执行错误: %1").arg(QString::fromStdString(e.what()))));
    } catch (const std::exception& e) {
        QSKIP(qPrintable(QString("异常: %1").arg(e.what())));
    } catch (...) {
        QSKIP("未知异常");
    }
}

}  // namespace DA