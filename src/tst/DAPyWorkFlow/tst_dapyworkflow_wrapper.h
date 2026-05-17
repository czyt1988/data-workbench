#ifndef TST_DAPYWORKFLOW_WRAPPER_H
#define TST_DAPYWORKFLOW_WRAPPER_H

#include <QObject>

namespace DA
{

/**
 * @brief DAPyWorkFlow 封装类单元测试
 *
 * 验证 DAPyWorkFlow 各方法的正确性，包括：
 * - 初始化与有效性检查 (initPyWorkflow, isValid)
 * - DAG 操作 (addNode, removeNode, nodeCount, hasNode, connectNode, disconnectNode, clear)
 * - 数据查询 (getNodeById, getNodes, getConnections, isValidDag, topologicalSort)
 * - Executor 操作 (executeAsync, terminate, pause, resume, getExecutorState, getResult, isRunning)
 * - 错误处理 (getLastError)
 *
 * 测试依赖 Python 运行环境，若 Python 不可用则自动 QSKIP。
 */
class TestDAPyWorkFlowWrapper : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 初始化 Python 环境
    void initTestCase();
    // 清理 Python 环境
    void cleanupTestCase();

    // --- 初始化与有效性 ---
    // 验证未初始化时 isValid 返回 false
    void testIsValidBeforeInit();

    // --- DAG 操作方法 ---
    // 验证 addNode 返回有效 node_id
    void testAddNode();
    // 验证 addNode(nullptr) 返回空字符串
    void testAddNodeNullProxy();
    // 验证 removeNode 返回 true，移除后 nodeCount 减少
    void testRemoveNode();
    // 验证 removeNode("nonexistent") 返回 false
    void testRemoveNodeNonexistent();
    // 验证 nodeCount 返回正确节点数
    void testNodeCount();
    // 验证 hasNode 对已添加节点返回 true，对不存在节点返回 false
    void testHasNode();
    // 验证 connectNode 返回有效连接描述符
    void testConnectNode();
    // 验证 connectNode 使用无效 ID 返回无效连接
    void testConnectNodeInvalidIds();
    // 验证 disconnectNode 返回 true
    void testDisconnectNode();
    // 验证 disconnectNode("nonexistent") 返回 false
    void testDisconnectNodeNonexistent();
    // 验证 clear 清空所有节点和连接
    void testClear();

    // --- 数据查询方法 ---
    // 验证 getNodeById 返回有效 Python 对象
    void testGetNodeById();
    // 验证 getNodeById 对不存在节点返回 py::none
    void testGetNodeByIdNonexistent();
    // 验证 getNodes 返回包含所有节点的列表
    void testGetNodes();
    // 验证 getConnections 返回包含所有连接的列表
    void testGetConnections();
    // 验证无环 DAG 时 isValidDag 返回 true
    void testIsValidDag();
    // 验证有环 DAG 时 isValidDag 返回 false
    void testIsValidDagWithCycle();
    // 验证 topologicalSort 返回正确排序
    void testTopologicalSort();

    // --- Executor 操作方法 ---
    // 验证 executeAsync 启动执行并完成
    void testExecuteAsync();
    // 验证 terminate 终止执行
    void testTerminate();
    // 验证 pause/resume 状态转换
    void testPauseResume();
    // 验证 getExecutorState 返回正确状态
    void testGetExecutorState();
    // 验证 getResult 返回执行结果
    void testGetResult();
    // 验证 isRunning 在执行期间返回 true
    void testIsRunning();

    // --- 错误处理 ---
    // 验证 getLastError 在错误后返回非空字符串
    void testGetLastErrorAfterError();
    // 验证 getLastError 在无错误时返回空字符串
    void testGetLastErrorNoError();

    // --- 指针便捷 API ---
    // 验证 removeNode(DAPyNodeProxy*) 按指针移除节点
    void test_removeNode_pointer();
    // 验证 connectNode(proxy*, srcChannel, proxy*, dstChannel) 创建有效连接
    void test_connectNode_pointer();
    // 验证 hasNode(DAPyNodeProxy*) 正确识别已有/未有节点
    void test_hasNode_pointer();

    // --- Scene O(1) 查找 ---
    // 验证 findNodeItemById O(1) 查找返回正确图形项
    void test_findNodeItemById_O1();
    // 验证 findNodeItemByProxy O(1) 查找返回正确图形项
    void test_findNodeItemByProxy_O1();

    // --- 生命周期 ---
    // 验证 setNodeProxies 正确填充 nodeId 映射
    void test_lifecycle_setNodeProxies();
};

}  // namespace DA

#endif  // TST_DAPYWORKFLOW_WRAPPER_H
