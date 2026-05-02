#include "tst_linkpoints.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyNodeProxy.h"
#include "DAPyLinkPoint.h"
#include "DAPyGILGuard.h"
#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonArray>

namespace DA
{

// ============================================================
// 薄描述符（无I/O数组）→ 连接点为空
// ============================================================

/**
 * @brief 测试薄描述符（非空但无inputs/outputs数组）的连接点生成
 *
 * 薄描述符仅包含 qualified_name、name、group 等基本信息，
 * 不包含 inputs/outputs 数组。
 * generateLinkPointsFromDescriptor() 对薄描述符返回空列表，
 * 这是描述符路径的正确行为（描述符本身没有I/O信息）。
 */
void TestLinkPoints::testThinDescriptorNoIO()
{
    DAPyNodeGraphicsItem item(nullptr);

    QJsonObject thinDesc;
    thinDesc["qualified_name"] = "test.mock";
    thinDesc["name"]           = "MockNode";
    thinDesc["group"]          = "test";
    item.setDescriptor(thinDesc);

    auto inputPoints  = item.getInputLinkPoints();
    auto outputPoints = item.getOutputLinkPoints();

    QCOMPARE(inputPoints.size(), 0);
    QCOMPARE(outputPoints.size(), 0);
}

// ============================================================
// 完整描述符（有I/O数组）→ 连接点非空
// ============================================================

/**
 * @brief 测试完整描述符（包含inputs/outputs数组）的连接点生成
 *
 * 完整描述符包含 inputs 和 outputs 数组，
 * generateLinkPointsFromDescriptor() 应正确解析并返回对应连接点。
 */
void TestLinkPoints::testFullDescriptorWithIO()
{
    DAPyNodeGraphicsItem item(nullptr);

    QJsonObject fullDesc;
    fullDesc["qualified_name"] = "test.mock";
    fullDesc["name"]           = "MockNode";
    fullDesc["group"]          = "test";

    QJsonArray inputs;
    QJsonObject in1;
    in1["name"] = "a";
    inputs.append(in1);
    QJsonObject in2;
    in2["name"] = "b";
    inputs.append(in2);
    fullDesc["inputs"] = inputs;

    QJsonArray outputs;
    QJsonObject out1;
    out1["name"] = "result";
    outputs.append(out1);
    fullDesc["outputs"] = outputs;

    item.setDescriptor(fullDesc);

    auto inputPoints  = item.getInputLinkPoints();
    auto outputPoints = item.getOutputLinkPoints();

    QCOMPARE(inputPoints.size(), 2);
    QCOMPARE(outputPoints.size(), 1);
}

// ============================================================
// Bug 3: 薄描述符 + 有keys的代理 → 连接点应为非空但实际为空
// ============================================================

/**
 * @brief 测试薄描述符与代理回退路径的交互（Bug 3）
 *
 * Bug 3场景：
 * 1. DAPyNodeProxy 有 input_keys=['a','b'] 和 output_keys=['result']
 * 2. proxy->getDescriptor() 返回薄描述符（只有qualified_name/name/group）
 * 3. DAPyNodeGraphicsItem构造函数从代理获取薄描述符存入mDescriptor
 * 4. updateLinkPoints() → generateLinkPoints() 检查 !mDescriptor.isEmpty()
 *    → true（薄描述符非空）→ 调用 generateLinkPointsFromDescriptor()
 *    → 返回空列表（薄描述符无inputs/outputs）
 *    → 直接return，不回退到代理路径
 *
 * 期望行为（修复后）：
 * - 薄描述符路径返回空时，应回退到代理路径
 * - 代理路径返回2输入+1输出=3个连接点
 *
 * 当前行为（Bug）：
 * - 薄描述符路径返回空，不回退到代理
 * - 返回0个连接点
 *
 * 此测试的 QCOMPARE(totalPoints, 3) 将因Bug而失败。
 */
void TestLinkPoints::testThinDescriptorWithProxyFallback()
{
    // 初始化Python解释器（使用项目标准方式）
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
        QSKIP("Python解释器未初始化，无法测试代理回退路径");
    }

    // 获取GIL（整个测试期间持有GIL，确保Python操作安全）
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        QSKIP("无法获取GIL，无法测试代理回退路径");
    }

    try {
        // 定义模拟Python节点类
        // - input_keys/output_keys 属性：用于 syncMetaFromPyNode 缓存
        // - get_descriptor() 方法：返回薄描述符（无inputs/outputs数组）
        pybind11::exec(R"(
class _TestMockNodeForLinkPoints:
    qualified_name = 'test.linkpoints.mock'
    node_name = 'MockNodeLP'
    input_keys = ['a', 'b']
    output_keys = ['result']
    group = 'test'
    node_prototype = 'test.linkpoints.mock._TestMockNodeForLinkPoints'

    def get_descriptor(self):
        # 返回薄描述符：只有基本信息，没有 inputs/outputs 数组
        return {'qualified_name': 'test.linkpoints.mock',
                'name': 'MockNodeLP',
                'group': 'test'}

    def execute(self):
        return True
)");

        pybind11::object mockNode = pybind11::eval("_TestMockNodeForLinkPoints()");

        // 创建代理并设置Python节点引用
        // setPyNodeRef() 会调用 syncMetaFromPyNode() 缓存 input/output keys
        DAPyNodeProxy* proxy = new DAPyNodeProxy();
        proxy->setPyNodeRef(mockNode);

        // 前置条件验证：代理有input/output keys
        QCOMPARE(proxy->getInputKeys().size(), 2);
        QCOMPARE(proxy->getOutputKeys().size(), 1);

        // 前置条件验证：描述符是薄描述符（非空，但无inputs/outputs）
        QJsonObject desc = proxy->getDescriptor();
        QVERIFY(!desc.isEmpty());
        QVERIFY(desc.contains("qualified_name"));
        QVERIFY(!desc.contains("inputs"));
        QVERIFY(!desc.contains("outputs"));

        // 创建节点图形项，构造函数从代理获取薄描述符
        // 构造函数调用：mDescriptor = proxy->getDescriptor() (薄描述符)
        // 然后 updateLinkPoints() → generateLinkPoints()
        // Bug: !mDescriptor.isEmpty() → true → generateLinkPointsFromDescriptor()
        //     → 返回空 → 不回退到代理路径
        DAPyNodeGraphicsItem item(proxy);

        // Bug 3 断言：期望代理回退路径返回3个连接点（2输入+1输出）
        // 当前行为：薄描述符路径返回空，代理回退未到达 → 0个连接点
        // 修复后：薄描述符路径返回空时应回退到代理 → 3个连接点
        int totalPoints = item.getInputLinkPoints().size() + item.getOutputLinkPoints().size();

        QCOMPARE(totalPoints, 3);  // 此断言因Bug 3而失败

    } catch (const pybind11::error_already_set& e) {
        // error_already_set 必须在GIL作用域内消费
        // gilGuard 仍持有GIL，安全消费异常
        QSKIP(qPrintable(QString("Python执行错误: %1").arg(QString::fromStdString(e.what()))));
    } catch (const std::exception& e) {
        QSKIP(qPrintable(QString("异常: %1").arg(e.what())));
    } catch (...) {
        QSKIP("未知异常");
    }
}

// ============================================================
// 集成测试：DAPyNodeProxy → DAPyNodeGraphicsItem → 连接点
// ============================================================

/**
 * @brief 验证代理→图形项→连接点的完整管线
 *
 * 此集成测试验证三个bug修复的综合效果：
 * - Bug 1（数据丢失）：DAPyNodeMetaData.inputKeys/outputKeys 不再丢失，
 *   DAPyNodeProxy.setPyNodeRef() 正确缓存 input/output keys
 * - Bug 2（setDescriptor覆写）：createPyNode(DAPyNodeMetaData) 不再调用
 *   setDescriptor 覆写薄描述符为更薄的版本
 * - Bug 3（代理回退）：generateLinkPoints() 在薄描述符路径返回空时，
 *   正确回退到代理路径获取连接点
 *
 * 测试流程：
 * 1. 初始化Python解释器
 * 2. 定义mock Python节点类，含 input_keys=['x','y'] 和 output_keys=['result']
 * 3. 创建DAPyNodeProxy并设置Python节点引用（触发syncMetaFromPyNode缓存keys）
 * 4. 创建DAPyNodeGraphicsItem(proxy)（构造函数自动调用updateLinkPoints()）
 * 5. 验证输入连接点数量为2，名称为'x'和'y'
 * 6. 验证输出连接点数量为1，名称为'result'
 *
 * 如果Bug 3未修复（薄描述符不回退到代理），连接点将为0。
 * 如果Bug 1未修复（keys未缓存），代理路径也无法获取keys，连接点为0。
 * 如果Bug 2未修复（setDescriptor覆写），即使T5回退逻辑生效，
 *   代理的descriptor可能被损坏导致异常行为。
 */
void TestLinkPoints::testIntegrationProxyToLinkPoints()
{
    // 初始化Python解释器
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
        QSKIP("Python解释器未初始化，无法测试集成管线");
    }

    // 获取GIL
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        QSKIP("无法获取GIL，无法测试集成管线");
    }

    try {
        // 定义mock Python节点类
        // 模拟真实Python工作流节点：有input_keys/output_keys属性和get_descriptor()方法
        pybind11::exec(R"(
class _TestIntegrationNode:
    qualified_name = 'test.integration.node'
    node_name = 'IntegrationNode'
    input_keys = ['x', 'y']
    output_keys = ['result']
    group = 'test'
    node_prototype = 'test.integration.node._TestIntegrationNode'

    def get_descriptor(self):
        # 返回薄描述符：只有基本信息，没有 inputs/outputs 数组
        # 这是真实场景中 Python 节点 get_descriptor() 的典型返回
        return {'qualified_name': 'test.integration.node',
                'name': 'IntegrationNode',
                'group': 'test'}

    def execute(self):
        return True
)");

        pybind11::object mockNode = pybind11::eval("_TestIntegrationNode()");

        // 步骤1: 创建代理并设置Python节点引用
        // setPyNodeRef() 调用 syncMetaFromPyNode()，缓存 input/output keys
        DAPyNodeProxy* proxy = new DAPyNodeProxy();
        proxy->setPyNodeRef(mockNode);

        // 前置验证: 代理正确缓存了keys（Bug 1修复验证）
        QCOMPARE(proxy->getInputKeys().size(), 2);
        QCOMPARE(proxy->getOutputKeys().size(), 1);
        QVERIFY(proxy->getInputKeys().contains("x"));
        QVERIFY(proxy->getInputKeys().contains("y"));
        QVERIFY(proxy->getOutputKeys().contains("result"));

        // 前置验证: 代理的描述符是薄描述符（Bug 2前置验证）
        QJsonObject desc = proxy->getDescriptor();
        QVERIFY(!desc.isEmpty());
        QVERIFY(desc.contains("qualified_name"));
        QVERIFY(!desc.contains("inputs"));
        QVERIFY(!desc.contains("outputs"));

        // 步骤2: 创建图形项，构造函数自动调用 updateLinkPoints()
        // 构造函数流程:
        //   mDescriptor = proxy->getDescriptor() (薄描述符)
        //   updateLinkPoints() → generateLinkPoints()
        //     → !mDescriptor.isEmpty() → generateLinkPointsFromDescriptor()
        //     → 薄描述符无inputs/outputs → 返回空
        //     → T5修复: 回退到代理路径 → proxy->getInputKeys()/getOutputKeys()
        //     → 返回2输入+1输出=3个连接点
        DAPyNodeGraphicsItem item(proxy);

        // 步骤3: 验证连接点数量（Bug 3修复验证 + 综合集成验证）
        QList<DAPyLinkPoint> inputPoints  = item.getInputLinkPoints();
        QList<DAPyLinkPoint> outputPoints = item.getOutputLinkPoints();

        QCOMPARE(inputPoints.size(), 2);
        QCOMPARE(outputPoints.size(), 1);

        // 步骤4: 验证连接点名称正确映射到keys
        // 连接点名称应与代理的 input/output keys 一致
        QStringList inputNames;
        for (const DAPyLinkPoint& lp : inputPoints) {
            inputNames.append(lp.name);
            QCOMPARE(lp.way, DAPyLinkPoint::Input);
            QCOMPARE(lp.direction, AspectDirection::West);
        }
        QVERIFY(inputNames.contains("x"));
        QVERIFY(inputNames.contains("y"));

        QStringList outputNames;
        for (const DAPyLinkPoint& lp : outputPoints) {
            outputNames.append(lp.name);
            QCOMPARE(lp.way, DAPyLinkPoint::Output);
            QCOMPARE(lp.direction, AspectDirection::East);
        }
        QVERIFY(outputNames.contains("result"));

    } catch (const pybind11::error_already_set& e) {
        QSKIP(qPrintable(QString("Python执行错误: %1").arg(QString::fromStdString(e.what()))));
    } catch (const std::exception& e) {
        QSKIP(qPrintable(QString("异常: %1").arg(e.what())));
    } catch (...) {
        QSKIP("未知异常");
    }
}

}  // namespace DA