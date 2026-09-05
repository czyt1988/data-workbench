# Agent 聊天历史重放性能回归测试

诊断与防回归工具：用**真实会话 jsonl** 驱动 headless Chromium 加载项目内**真实的
`src/DAGui/Agent/resources/chat.js`**（markdown-it / highlight.js / KaTeX 均为线上同款），
分段计时重放耗时并断言分段懒加载的正确性。

## 背景

修复"切换大会话 UI 冻结近半分钟"问题时建立的反馈回路。当时根因：

- `loadHistory` 循环内**每个事件**都调 `scrollToBottom()`（读 `scrollHeight`
  写 `scrollTop` 强制同步 layout），340 个事件 ≈ 391 次 reflow，DOM 约 5000
  节点时实测占渲染总耗时 **97%**（O(n²)）；
- markdown / KaTeX / hljs 渲染本身只占几十毫秒，非瓶颈；
- Qt WebEngine 内置 Chromium 版本较旧时单次 reflow 更贵，放大到用户可感知的半分钟级冻结。

修复见 `chat.js`：批量重放窗口内 `suppressAutoScroll` 抑制逐事件滚动 +
`HISTORY_CHUNK_SIZE` 分段渲染（顶部哨兵"加载更早"，滚动到顶自动 prepend）。

## 用法

```powershell
.\run.ps1 -SessionJsonl "C:\Users\<user>\AppData\Roaming\DAWorkBench\sessions\<id>.jsonl"
```

依赖：`node`（PATH 中）+ Edge/Chrome（自动探测常用安装路径）。

## 产出指标

| 指标 | 含义 |
|------|------|
| `initialLoadMs` | 首次 loadHistory（分段，仅尾部 chunk）耗时 |
| `drainMs` | 同步加载完全部剩余 chunk 的耗时 |
| `steadyReplayMs` | 稳态下重复切换会话的重放耗时 |
| `initialScrollMs` / `initialScrollCalls` | scrollToBottom 累计耗时/次数（reflow 开销） |
| `EQUIV_*` | 分段全加载后 DOM 与一次性全量渲染的等价断言 |
| `firstUserMatches` | 最早 user 事件是否渲染在 DOM 顶部（顺序断言） |
| `sentinelAfterInitial/Drain`, `pendingAfterDrain` | 哨兵状态机断言 |

失败判定阈值：`initialLoadMs > 2000`、`steadyReplayMs > 500` 或任一等价/顺序断言为假。

## 文件说明

- `jsonl-to-events.js` — 会话 jsonl → UI 事件数组（忠实复刻
  `DAAgentWebChannel::loadHistory` 的 C++ 合并逻辑，含 tool_call/tool_result 配对）。
- `harness.html` — mock QWebChannel 环境加载真实 chat.js，双轮计时 + 断言，
  结果写入 `<pre id="perf-result">` 由 `--dump-dom` 提取。
- `run.ps1` — 一键入口（转换 → headless 运行 → 解析断言）。
- `events.js` / `dom.html` — 运行生成物（已 gitignore，勿提交）。
