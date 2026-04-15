# 贡献指南

感谢您对 DAWorkBench 项目的关注！本指南说明如何参与项目开发，包括 Bug 报告、功能建议、代码贡献和文档改进。

## 主要功能特性

**特性**

- ✅ **多种贡献方式**：Bug 报告、功能建议、代码贡献、文档改进
- ✅ **开发环境准备**：必要工具清单和仓库克隆步骤
- ✅ **代码规范**：命名规范、注释规范、头文件组织
- ✅ **提交规范**：Commit 信息格式和 Type 类型说明
- ✅ **PR 流程**：创建分支、开发测试、提交变更的完整流程
- ✅ **检查清单**：代码、功能、文档、测试的 PR 前检查项

## 贡献方式

| 方式 | 说明 |
|------|------|
| **提交 Bug 报告** | 在 GitHub Issues 报告问题 |
| **功能建议** | 在 GitHub Discussions 讨论 |
| **代码贡献** | 提交 Pull Request |
| **文档改进** | 改进文档内容或翻译 |

## 开发环境准备

### 必要工具

- Git
- Qt 5.14+ 或 Qt 6.x
- CMake 3.12+
- C++17 编译器
- Python 3.8+（可选）

### 克隆仓库

```bash
git clone https://github.com/czyt1988/data-workbench.git
cd data-workbench
git submodule update --init --recursive
```

### 创建开发分支

```bash
# 创建功能分支
git checkout -b feature/my-feature

# 或创建修复分支
git checkout -b fix/my-fix
```

## 代码规范

### 编码风格

遵循项目编码规范，保持与现有代码一致的风格。

下面的代码展示了项目的命名规范：

```cpp
// 类命名：PascalCase，DA 前缀 - 保持与项目风格一致
class MyPluginClass { };

// 函数命名：camelCase - 小驼峰命名
void processData();

// 变量命名：m_ 前缀 + camelCase（成员变量）
int m_counter;

// 常量命名：UPPER_CASE - 全大写加下划线
const int MAX_ITERATIONS = 1000;

// 枚举命名：PascalCase，值 camelCase - 类型大驼峰，值小驼峰
enum NodeStatus {
    StatusIdle,      // 空闲状态
    StatusRunning,   // 运行状态
    StatusFinished   // 完成状态
};
```

命名规范确保代码风格统一，便于阅读和维护。

### 注释规范

使用 Doxygen 格式注释，详细说明函数用途、参数和返回值。

下面的代码展示了标准的注释格式：

```cpp
/**
 * @brief 简短描述（一行） - 函数的简要说明
 * 
 * 详细描述（多行） - 函数的详细功能说明
 * 
 * @param param1 参数1说明 - 参数用途描述
 * @param param2 参数2说明 - 参数用途描述
 * @return 返回值说明 - 返回值的含义
 * 
 * @note 注意事项 - 使用时需要注意的问题
 * @warning 警告信息 - 可能导致错误的情况
 * 
 * @example
 * @code
 * // 使用示例代码 - 展示函数调用方式
 * processData("input", 100);
 * @endcode
 */
bool processData(const QString& param1, int param2);
```

Doxygen 注释支持自动生成 API 文档，建议在头文件中使用简短注释，详细注释放在 cpp 文件中。

### 头文件规范

头文件使用 `#pragma once` 防止重复包含，遵循 Qt 的编码风格。

下面的代码展示了标准头文件结构：

```cpp
#pragma once                     // 使用 pragma once 替代 #ifndef

#include <QObject>               // Qt 头文件
#include "DAAbstractNode.h"      // 项目头文件

/**
 * @brief 我的自定义节点类 - 类的简要说明
 */
class MyCustomNode : public DA::DAAbstractNode
{
    Q_OBJECT                     // Qt 元对象系统支持
public:
    // 构造函数 - 头文件中使用简短注释
    MyCustomNode(const DA::DANodeMetaData& meta, DA::DACoreInterface* core);
    
    // ... 其他方法声明
};
```

## 提交规范

### Commit 信息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type 类型

| Type | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档变更 |
| `style` | 代码风格（不影响功能） |
| `refactor` | 重构 |
| `test` | 测试相关 |
| `chore` | 构建/工具相关 |

### 示例

```
feat(plugin): add data validation node

Add a new node type for data validation with configurable rules.
- Support regex validation
- Support range validation
- Support custom Python script validation

Closes #123
```

## Pull Request 流程

### 1. 创建分支

```bash
git checkout -b feature/my-feature
```

### 2. 开发和测试

```bash
# 构建
cmake --build build

# 运行测试（如有）
ctest --test-dir build

# 运行程序验证
./bin/DAWorkbench
```

### 3. 提交变更

```bash
git add .
git commit -m "feat(plugin): my feature description"
```

### 4. 推送分支

```bash
git push origin feature/my-feature
```

### 5. 创建 Pull Request

在 GitHub 上创建 PR，填写：

- 标题：简洁描述变更
- 说明：详细描述变更内容和原因
- 关联 Issue：如有

### 6. 等待审核

- 响应审核意见
- 修改代码
- 直到审核通过

## PR 检查清单

提交 PR 前请确认：

### 代码检查

- [ ] 代码符合编码规范
- [ ] 无编译警告
- [ ] 无内存泄漏风险
- [ ] 线程安全已验证

### 功能检查

- [ ] 功能正常工作
- [ ] 边界情况已处理
- [ ] 错误处理完善

### 文档检查

- [ ] 新类有文档注释
- [ ] 公开接口有说明
- [ ] 用户文档已更新（如有）

### 测试检查

- [ ] 新功能有测试
- [ ] 现有测试通过
- [ ] Qt5 和 Qt6 兼容

## 文档贡献

### 文档格式

使用 Markdown 格式，遵循 MkDocs 规范：

```markdown
# 标题

段落内容。

## 二级标题

### 列表

- 项目1
- 项目2

### 代码块

```cpp
// 代码示例
```

### 提示框

!!! tip "提示"
    这是一个提示。

!!! warning "警告"
    这是一个警告。
```

### 文档位置

- 用户文档：`docs/zh/`
- 开发文档：`docs/zh/dev-guide/`
- 构建文档：`docs/zh/build/`

## 问题反馈

### Bug 报告

在 GitHub Issues 提交，包含：

- **标题**：简洁描述问题
- **环境信息**：
  - 操作系统
  - Qt 版本
  - 编译器版本
- **复现步骤**：详细的操作步骤
- **预期行为**：应该发生什么
- **实际行为**：实际发生了什么
- **日志/截图**：如有

### 功能建议

在 GitHub Discussions 讨论：

- 功能描述
- 使用场景
- 预期效果

## 社区准则

### 行为准则

- 尊重所有贡献者
- 使用友好和建设性的语言
- 接受不同的观点和经验
- 关注对社区最有利的事情

### 交流渠道

- GitHub Issues：问题追踪
- GitHub Discussions：功能讨论
- Pull Requests：代码审查

## 许可证

贡献的代码将采用 LGPL 3.0 许可证，与项目保持一致。

提交 PR 即表示同意：

1. 您的贡献将按 LGPL 3.0 许可
2. 您有权利贡献这些代码
3. 您同意代码被包含在项目中

## 联系方式

- 项目地址：https://github.com/czyt1988/data-workbench
- 作者：Chen ZongYan

感谢您的贡献！