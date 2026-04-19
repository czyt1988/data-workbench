# 前置预研阶段详细实施计划

## 阶段目标

解决 AI Agent 升级计划中的所有阻断性问题，确定技术方案，为后续实施扫清障碍。本阶段是后续所有工作的前提，必须 100% 完成并通过验收才能进入 Phase 1。

**周期**: 2 周  
**优先级**: 🔴 最高（阻断性）

---

## 任务分解

### 任务 0.1: Python 版本升级评估

**目标**: 将嵌入式 Python 从 3.7+ 升级到 3.10+，满足 crewAI/LangGraph 硬性要求

**涉及文件**:
- `src/DAPyScripts/` - Python 脚本相关代码
- `src/DAPyBindQt/` - pybind11 绑定代码
- `CMakeLists.txt` - Python 配置相关
- `requirements.txt` - Python 依赖清单

**子步骤**:

#### Step 1: 调查当前 Python 嵌入配置
- 检查 `src/DAPyBindQt/` 目录下 pybind11 的初始化代码
- 查找 Python 路径配置、版本检测逻辑
- 记录所有 Python 相关 CMake 变量

**验收**: 输出《Python 嵌入现状分析报告》，包含：
- 当前 Python 版本检测代码位置
- Python 解释器初始化方式
- 所有 Python 相关 CMake 配置项

#### Step 2: 升级嵌入式 Python 到 3.10+
- 修改 CMake 配置，指定 Python 3.10 为最低版本
- 更新 pybind11 到支持 Python 3.10 的版本（建议 2.10+）
- 验证 Python 初始化代码在 3.10 下正常

**CMake 修改示例**:
```cmake
# 在相关 CMakeLists.txt 中
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 2.10 REQUIRED)
```

**验收**: 
```bash
# 构建后执行验证
python -c "import sys; assert sys.version_info >= (3,10), 'Python version too old'"
echo "Python version check passed"
```

#### Step 3: 验证现有 Python 脚本兼容性
- 收集 `src/PyScripts/` 和 `src/DAPyScripts/` 下所有 Python 脚本
- 在 Python 3.10 环境下逐一执行，记录语法错误、API 变更
- 重点检查：`pandas`, `numpy`, `scipy` 相关调用

**验收**: 输出《Python 3.10 兼容性报告》，包含：
- 不兼容的语法列表（如 `match-case` 在 3.10 新增）
- 需要更新的依赖版本
- 修改建议

#### Step 4: 处理 pybind11 绑定兼容性
- 检查所有 `PYBIND11_MODULE` 定义
- 验证 C++→Python 类型转换在 3.10 下正常
- 测试 Qt 类型与 Python 类型的互操作

**验收**: 编译通过且运行以下测试无错误：
```cpp
// 在测试代码中
py::gil_scoped_acquire acquire;
py::exec("import da; print(da.__version__)");
```

---

### 任务 0.2: AI 依赖冲突预检

**目标**: 验证 crewAI、LangChain、LangGraph 与现有依赖的兼容性

**涉及文件**:
- `requirements.txt` - 依赖清单
- `src/DAPyScripts/` - 测试脚本

**子步骤**:

#### Step 1: 创建干净 Python 环境进行依赖测试
```bash
# 创建虚拟环境
python -m venv .venv_test
.venv_test\Scripts\activate

# 安装所有依赖
pip install -r requirements.txt
```

**验收**: `pip install` 无冲突、无错误

#### Step 2: 验证所有 AI 库可正常导入
```python
# test_ai_imports.py
import sys
print(f"Python {sys.version}")

# 核心依赖
import pandas as pd
import numpy as np
import scipy

# AI 框架
import crewai
import langchain
import langgraph
import openai

print("All imports successful!")
```

**验收**: 脚本执行成功，无 `ImportError`

#### Step 3: 记录依赖版本矩阵
创建《AI 依赖版本兼容表》，包含：
- 每个库的版本号
- 依赖关系图
- 已知冲突及解决方案

**验收**: 文档完整，版本信息准确

#### Step 4: 解决依赖冲突（如有）
- 如遇冲突，使用 `pip-tools` 或 `poetry` 锁定兼容版本
- 更新 `requirements.txt`

**验收**: 所有依赖可共存，导入测试通过

---

### 任务 0.3: 异步架构设计

**目标**: 完成 Qt 事件循环与 Python asyncio 桥接方案设计

**涉及文件**:
- `src/DAWorkFlow/DAWorkFlowExecuter.h` - 执行引擎
- `src/DAWorkFlow/DAAbstractNode.h` - 节点基类
- 新文件：`docs/design/async-architecture.md`

**子步骤**:

#### Step 1: 分析现有执行引擎同步模型
- 阅读 `DAWorkFlowExecuter::exec()` 实现
- 理解 `DAAbstractNode::exec()` 调用链
- 识别同步阻塞点

**验收**: 输出《现有执行引擎分析报告》

#### Step 2: 设计 Qt 与 asyncio 双事件循环桥接方案
**核心设计**:
```cpp
// 方案要点
class DAAsyncWorkflowExecuter : public DAWorkFlowExecuter {
    Q_OBJECT
public:
    // 异步执行入口
    QFuture<void> execAsync();
    
private:
    // Python asyncio 运行在独立线程
    QThread* m_pythonThread;
    // 结果通过信号槽回调
    Q_SIGNAL void nodeCompleted(DAAbstractNode* node, const QVariant& result);
};
```

**验收**: 完成《Qt-asyncio 桥接设计文档》，包含：
- 线程模型图
- 信号槽通信机制
- GIL 管理策略

#### Step 3: 设计异步节点接口
```cpp
// DAAbstractNode 扩展
class DAAbstractNode {
public:
    // 新增异步执行接口
    virtual QFuture<QVariant> execAsync() {
        // 默认调用同步版本
        return QtConcurrent::run([this]() { return exec(); });
    }
    
    // 判断节点是否支持异步
    virtual bool supportsAsync() const { return false; }
};
```

**验收**: 接口设计文档完成，包含使用示例

#### Step 4: 设计 Agent 节点异步状态机
```cpp
class DAAgentNode : public DAAbstractNode {
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(QString goal READ goal WRITE setGoal NOTIFY goalChanged)
    Q_PROPERTY(int maxIterations READ maxIterations WRITE setMaxIterations)
    
public:
    QFuture<QVariant> execAsync() override;
    bool supportsAsync() const override { return true; }
    
private:
    // asyncio 任务状态跟踪
    QFutureWatcher<QVariant>* m_executionWatcher;
};
```

**验收**: 状态机设计文档完成

---

### 任务 0.4: 现有代码兼容性验证

**目标**: 验证现有功能在 Python 3.10 下正常运行

**涉及文件**:
- `tst/` - 测试目录
- `src/PyScripts/` - Python 脚本

**子步骤**:

#### Step 1: 收集现有 Python 测试用例
- 查找所有 Python 测试脚本
- 整理测试清单

**验收**: 输出《Python 测试用例清单》

#### Step 2: 在 Python 3.10 下运行所有测试
```bash
# 使用 Python 3.10 执行测试
python3.10 -m pytest tst/python/ -v
```

**验收**: 所有测试通过，失败用例有详细记录

#### Step 3: 验证工作流功能
- 创建测试工作流（包含数据处理、可视化节点）
- 执行工作流，验证输出正确

**验收**: 工作流执行成功，输出符合预期

#### Step 4: 验证 pandas/numpy/scipy 功能
```python
# test_data_operations.py
import pandas as pd
import numpy as np

df = pd.DataFrame({'A': [1, 2, 3], 'B': [4, 5, 6]})
result = df.groupby('A').sum()
assert result.shape == (3, 1)
print("Data operations test passed")
```

**验收**: 数据处理测试通过

---

## 验收标准

### 阶段准入 Phase 1 的条件（全部必须满足）:
- [ ] Python 3.10 版本验证通过
- [ ] 所有 AI 依赖可正常导入
- [ ] 异步架构设计文档完成并通过审查
- [ ] 现有功能在 Python 3.10 下兼容性验证通过
- [ ] 《预研阶段总结报告》完成

### 交付物:
1. `docs/design/async-architecture.md` - 异步架构设计文档
2. `docs/research/python3.10-compatibility.md` - Python 3.10 兼容性报告
3. `docs/research/ai-dependencies.md` - AI 依赖版本兼容表
4. `requirements.txt` - 更新后的依赖清单

---

## 技术风险

| 风险 ID | 风险描述 | 缓解方案 | 验证方法 |
|---------|----------|----------|----------|
| R10 | Python 3.10 与现有代码不兼容 | 提前验证所有 Python 脚本，准备迁移方案 | 运行全量测试套件 |
| R1 | asyncio 与 Qt 事件循环冲突 | 严格隔离线程，Python 线程专用 asyncio | 压力测试，多线程并发场景 |
| R2 | crewAI/LangChain 依赖冲突 | 锁定兼容版本，使用虚拟环境隔离 | `pip install` 无冲突 |
| R3 | pybind11 绑定在 3.10 下异常 | 升级 pybind11 到 2.10+，逐模块验证 | 导入测试 + 功能测试 |

---

## 依赖关系

```
任务 0.1 (Python 升级) ─┬─> 任务 0.2 (依赖预检)
                        └─> 任务 0.4 (兼容性验证)

任务 0.3 (异步设计) ────> Phase 1 任务 1.3 (执行引擎重构)
```

**前置依赖**: 无（本阶段为起始阶段）  
**后置依赖**: Phase 1 所有任务依赖本阶段完成

---

## 时间规划

| 周次 | 任务 | 预计工时 |
|------|------|----------|
| Week 1 | 任务 0.1 + 任务 0.2 | 40 小时 |
| Week 2 | 任务 0.3 + 任务 0.4 | 40 小时 |

**里程碑**: Week 2 结束前完成所有预研任务，输出完整设计文档
