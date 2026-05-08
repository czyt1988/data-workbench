#include "tst_daportdescriptor.h"
#include "DAPortDescriptor.h"
#include <QtTest/QtTest>
#include <QJsonObject>

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
// testJsonRoundTrip — JSON序列化往返
// ============================================================

/**
 * @brief 验证 DAPortDescriptor JSON往返序列化
 *
 * 创建带值描述符 → toJson → fromJson → 验证值与原始一致
 */
void TestDAPortDescriptor::testJsonRoundTrip()
{
    DAPortDescriptor orig(QStringLiteral("input_data"),
                          QStringLiteral("DataFrame"),
                          false,
                          QStringLiteral("输入DataFrame数据"));

    QJsonObject json = orig.toJson();
    DAPortDescriptor roundTrip = DAPortDescriptor::fromJson(json);

    QCOMPARE(roundTrip.name, orig.name);
    QCOMPARE(roundTrip.dataType, orig.dataType);
    QCOMPARE(roundTrip.required, orig.required);
    QCOMPARE(roundTrip.description, orig.description);
}

}  // namespace DA
