# 会话持久化

Agent 对话历史以 JSONL 格式持久化到磁盘，支持多会话管理、工程内会话导出/导入、自动清理过期会话。本文档描述会话持久化的设计要点、文件格式、生命周期管理和崩溃安全机制。

---

## 设计要点

| 原则 | 说明 |
|------|------|
| **C++ 主导持久化** | C++ 负责全部 JSONL 读写 / 索引 / 清理，Python 子进程只负责推理与 state 重建 |
| **双存储** | 自由会话存配置目录 `sessions/`；保存工程时活跃会话复制进 zip 的 `agent_sessions/` |
| **不重启切换** | 切换会话通过 `load_session` 下发历史重建 LangGraph state，不重启子进程（避免 ~16s 冷启动） |
| **崩溃安全** | JSONL append-only + 每条即写 flush，崩溃后最多丢最后一两行 |
| **自动恢复** | 启动程序自动填充上次活跃会话下拉；打开工程自动加载工程内会话 |

---

## 文件布局

```
<appData>/sessions/
├── <sessionId>.jsonl          # 每个会话一个 JSONL 文件 (append-only)
├── sessions_index.json        # 全局索引 (原子写 tmp+rename)
└── last_active.json           # 上次活跃会话指针 (sessionId + projectPath)
```

> Windows 路径展开为 `%APPDATA%/DAWorkBench/DAWorkBench/sessions/`。路径由 `DADir::getAppDataPath("sessions")` 决定。

### 会话文件（JSONL）

每行一条 JSON 记录，append-only 追加：

```json
{
    "uuid": "550e8400-e29b-41d4-a716-446655440000",
    "parent_uuid": null,
    "session_id": "session-uuid-xxxx",
    "timestamp": "2026-08-10T14:30:00.000Z",
    "type": "user",
    "message": {
        "role": "human",
        "content": "帮我分析这组数据"
    }
}
```

### 记录类型

| type | message.role | 说明 | 含 tool_calls | 含 tool_call_id |
|------|-------------|------|:---:|:---:|
| `user` | `human` | 用户消息 | — | — |
| `assistant` | `ai` | LLM 回复 | 可能 | — |
| `tool_result` | `tool` | 工具执行结果 | — | 是 |
| `usage` | — | Token 用量记录 | — | — |

!!! note "question/answer 复用 tool_call/tool_result 语义"
    HITL 提问（`ask_user` 工具）在 JSONL 中存储为 `assistant` 类型（含 `tool_calls`），用户回答存储为 `tool_result` 类型。一期不单独区分 question/answer 记录类型。

### 全局索引（sessions_index.json）

```json
[
    {
        "id": "uuid-xxxx",
        "title": "数据分析对话",
        "createdAt": "2026-08-10T14:00:00.000Z",
        "updatedAt": "2026-08-10T14:30:00.000Z",
        "messageCount": 10,
        "projectPath": ""
    }
]
```

索引文件通过 **tmp + rename** 原子写入，避免写入中途崩溃导致索引损坏。

### 上次活跃指针（last_active.json）

```json
{
    "sessionId": "uuid-xxxx",
    "projectPath": ""
}
```

`projectPath` 为空表示自由会话，非空表示工程绑定会话。`lastActiveSession()` 按工程路径精确匹配返回。

---

## 会话生命周期

### 创建会话

```mermaid
sequenceDiagram
    participant UI as DockWidget
    participant IF as DAAgentInterface
    participant M as DAAgentModule
    participant S as SessionStore

    UI->>IF: sessionCreateRequested()
    IF->>M: newSession()
    M->>S: createSession()
    S->>S: 生成 UUID4
    S->>S: 创建空 .jsonl 文件
    S->>S: 更新 sessions_index.json
    S-->>M: 返回 sessionId
    M->>M: emit sessionCreated(sessionId)
    M->>IF: emit sessionCreated(sessionId)
    IF->>UI: onSessionCreated() → clearChat()
```

!!! note "newSession() vs createSession()"
    `newSession()` 会 emit `sessionCreated` 信号触发 UI clearChat，用于用户点击"+"按钮。`createSession()` 不 emit 信号，用于 `sendMessage()` 自动创建会话——避免清除刚显示的用户消息。

### 切换会话

切换会话**不重启子进程**，通过 `load_session` 下发历史重建 LangGraph state：

```mermaid
sequenceDiagram
    participant UI as DockWidget
    participant M as DAAgentModule
    participant S as SessionStore
    participant B as DAAgentBridge
    participant Py as Python 子进程

    UI->>M: switchSession(id)
    M->>S: readMessagesForLoad(id)
    S-->>M: messages[] (过滤 usage, 只返回 user/assistant/tool_result)
    M->>B: sendLoadSession(id, messages)
    B->>Py: {"type":"load_session", "session_id":"...", "messages":[...]}

    Note over Py: 重建 LangGraph (新 MemorySaver)
    Note over Py: graph.aupdate_state() 注入历史

    Py-->>B: {"type":"session_loaded", "session_id":"..."}
    B-->>M: emit agentSessionLoaded(id)
    M->>M: emit sessionSwitched(id, allRecords)
    M->>UI: onSessionSwitched() → 重放历史到 UI

    Note over UI: 恢复输入框可用态
    Note over UI: 此后方可发下一轮 user_msg
```

!!! danger "收到 session_loaded 前禁止发 user_msg"
    在收到 `session_loaded` 之前发送 `user_msg` 会导致 state 未重建完毕，历史消息丢失。`DAAgentBridge` 收到 `session_loaded` 后发 `agentSessionLoaded` 信号，UI 据此恢复输入框。

### 删除会话

```
deleteSession(id)
  → SessionStore: 删除 <id>.jsonl 文件
  → SessionStore: 从 sessions_index.json 移除
  → 如果删的是当前会话: emit sessionCleared()
  → emit sessionListChanged() (刷新下拉)
```

### 重命名会话

```
renameSession(id, title)
  → SessionStore: 更新 sessions_index.json 的 title + updatedAt
  → emit sessionListChanged()
```

### 自动标题

`ensureTitle(sessionId)` 在用户首次发送消息后，取消息内容前 30 个字符作为会话标题。如果会话已有非空标题则不覆盖。

---

## 工程导入/导出

### 保存工程时导出

```mermaid
graph LR
    A["DAAppProject::executeSave"] --> B["agentMod->exportActiveSessions()"]
    B --> C["收集活跃会话的 JSONL 字节"]
    C --> D["appendByteSaveTask()"]
    D --> E["写入 zip 的 agent_sessions/<id>.jsonl"]
```

`exportActiveSessions()` 返回 `QHash<QString, QByteArray>`——每个活跃会话的 ID 到 JSONL 字节内容的映射。在主线程收集（不碰 UI），子线程写入 zip。

### 打开工程时导入

```mermaid
graph LR
    A["DAAppProject 加载任务"] --> B["解压 agent_sessions/*.jsonl"]
    B --> C["agentMod->loadSessionsFromProject(files, projectPath)"]
    C --> D["写入 sessions/ 目录"]
    D --> E["更新 sessions_index.json (标记 projectPath)"]
    E --> F["agentMod->setCurrentProjectPath(path)"]
    F --> G["restoreLastActiveSession()"]
```

---

## 自动清理

`DAAgentModule::cleanupSessions()` 在程序启动时执行，基于两个维度清理自由会话（不影响工程绑定会话）：

| 清理维度 | 配置键 | 默认值 | 逻辑 |
|---------|--------|--------|------|
| 数量上限 | `max_sessions` | 20 | 超出时按 `updatedAt` 倒序删除最旧的 |
| 时间上限 | `session_retention_days` | 30 | 早于此天数的会话被删除 |

```cpp
// cleanupOldSessions 入口已加 qMax 防护
int maxCount = qMax(1, config.maxSessions);       // 防止 ini 手改为 0
int retentionDays = qMax(0, config.retentionDays); // 防止负值
```

!!! warning "保护活跃会话"
    `cleanupOldSessions` 跳过当前活跃会话（`skipSessionId`）和 `lastActive` 指针指向的会话，避免清理正在使用的会话。

---

## 崩溃安全

### JSONL 写入

- **Append-only**：每条记录追加到文件末尾，不修改已有内容
- **每条即写 flush**：`appendRecord()` 写入后立即 flush，崩溃后最多丢最后一两行
- **容忍损坏行**：`parseLineTolerant` 跳过损坏行/空行，不整体丢弃会话

### 索引原子写

`sessions_index.json` 通过 tmp + rename 原子写入：

```cpp
// 1. 写入临时文件
QFile tmp(sessionsDir + "/sessions_index.json.tmp");
tmp.write(json);
tmp.flush();
tmp.close();

// 2. 原子 rename
QFile::rename(sessionsDir + "/sessions_index.json.tmp",
              sessionsDir + "/sessions_index.json");
```

### last_active 精确匹配

`lastActiveSession(filter)` 按 `projectPath` 精确匹配：

- 空 filter：只返回 `projectPath` 为空的自由会话
- 非空 filter：精确匹配工程路径

避免启动恢复时把工程绑定会话当自由会话恢复。

---

## 会话恢复时序

### 启动程序

```
DAAppController::initialize()
  → agentMod->cleanupSessions()           // 清理超限/过期会话
  → restoreLastActiveSession()            // 填充会话下拉
    → listSessions() → 填充 UI 下拉
    → 不自动恢复（始终以全新对话开始）
    → 用户可通过下拉手动切换到历史会话
```

### 打开工程

```
DAAppProject 加载任务
  → 解压 agent_sessions/*.jsonl
  → agentMod->loadSessionsFromProject(files, projectPath)
  → agentMod->setCurrentProjectPath(path)
  → restoreLastActiveSession()
    → listSessions(projectPath) → 填充工程会话下拉
    → 不自动恢复（始终以全新对话开始）
```

### 保存工程

```
DAAppProject::executeSave
  → agentMod->setCurrentProjectPath(path)
  → agentMod->setSessionProjectPathForCurrent(path)
  → agentMod->exportActiveSessions() → 收集活跃会话字节
  → 写入 zip 的 agent_sessions/
```

### SaveAs 工程

```
setCurrentProjectPath(newPath)       // 同步新路径
setSessionProjectPath(newPath)       // 更新 index 中的 projectPath
setLastActive(sessionId, newPath)    // 更新指针的 projectPath
```

---

## 配置参数

| 参数 | 配置键 | 默认值 | 范围 | 说明 |
|------|--------|--------|------|------|
| 最大会话数 | `max_sessions` | 20 | 5-200 | 自由会话保留上限 |
| 保留天数 | `session_retention_days` | 30 | 1-365 | 自由会话保留天数 |

!!! note "三处默认值须一致"
    `max_sessions` / `session_retention_days` 的默认值在以下三处须保持一致：
    1. 设置页 spin range/setValue（`DAAgentSettingsWidget`）
    2. `DAAgentModule::getLLMConfig` / `setLLMConfig` 的默认值
    3. `DAAgentModule::cleanupSessions` 的 QSettings 读取

---

## 单元测试

`src/tst/DAAgentSessionStoreTest/` 提供 5 个 C++ 测试用例（不链接 Python）：

| 用例 | 覆盖内容 |
|------|---------|
| `testAppendAndRead` | appendRecord + readMessagesForLoad（usage 被过滤，messageCount 只计对话消息） |
| `testParseTolerant` | JSONL 含损坏行/空行时跳过坏行返回有效记录 |
| `testIndexAtomicWrite` | listSessions 倒序，renameSession 更新 index + title |
| `testCleanup` | 数量上限删最旧、时间上限删超期、skipSessionId 保护活跃、qMax 边界防护 |
| `testLastActive` | last_active 往返 + projectPath 精确匹配 |

测试隔离：`main()` 起手 `QStandardPaths::setTestModeEnabled(true)` 重定向 AppData 到临时目录，不污染真实 `%APPDATA%`。

```powershell
# 构建
cmake -S . -B build -D DA_ENABLE_TESTING=ON
cmake --build build --target DAAgentSessionStoreTest --config Release

# 运行 (Windows Qt Test stdout 不可见，须用 -o)
.\build\bin\DAAgentSessionStoreTest.exe -o result.txt
```

---

## 参见

- [崩溃恢复与重连](crash-recovery.md) — 崩溃后通过 SessionStore 恢复会话
- [通信协议](protocol.md) — load_session / session_loaded 协议消息
- [架构设计](architecture.md) — DAAgentSessionStore 在架构中的位置
- `src/DAAgent/DAAgentSessionStore.h/.cpp` — 持久化层实现
- `src/tst/DAAgentSessionStoreTest/main.cpp` — 单元测试
