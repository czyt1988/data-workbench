<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>AppMainWindow</name>
    <message>
        <location filename="../APP/AppMainWindow.ui" line="+14"/>
        <source>DA-Workflow</source>
        <translation>DA-Workflow</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Open</source>
        <translation>打开</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>saveAs</source>
        <translation>另存为</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>redo</source>
        <translation>重做</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>undo</source>
        <translation>撤销</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>plugin</source>
        <translation>插件</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Show Work Flow Area</source>
        <translation>工作流区域</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Show Chart Area</source>
        <translation>绘图区域</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Show Table Area</source>
        <translation>表格区域</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Show Infomation Window</source>
        <translation>信息窗口</translation>
    </message>
</context>
<context>
    <name>DA::AppMainWindow</name>
    <message>
        <location filename="../APP/AppMainWindow.cpp" line="+78"/>
        <source>Initializing core interface...</source>
        <translation>正在初始化核心接口...</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Creating user interface...</source>
        <translation>正在创建用户界面...</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Restore UI state</source>
        <translation>加载界面状态信息</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Loading plugins...</source>
        <translation>正在加载插件...</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Preparing interface...</source>
        <translation>正在准备界面...</translation>
    </message>
    <message>
        <location line="+52"/>
        <source>Question</source>
        <translation>疑问</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Do you need to save the project?</source>
        <translation>是否保存工程？</translation>
    </message>
    <message>
        <location line="+84"/>
        <source>Untitled</source>
        <translation>未命名</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Topology execution completed</source>
        <translation>拓扑执行完成</translation>
    </message>
    <message>
        <location line="+0"/>
        <location line="+2"/>
        <source>Information</source>
        <translation>信息</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Topology execution failed</source>
        <translation>拓扑执行失败</translation>
    </message>
    <message>
        <location line="+131"/>
        <source>failed to restore UI state</source>
        <translation>恢复界面状态过程中出错</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>cannot read UI state file %1: %2</source>
        <translation>无法读取界面状态文件%1，原因：%2</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>UI state has been reset, the default layout will be applied on next launch</source>
        <translation>界面状态已重置，默认布局将在下次启动时应用</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Default layout snapshot is not ready</source>
        <translation>默认布局快照尚未就绪</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Default layout restored</source>
        <translation>已恢复默认布局</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Project auto-saved</source>
        <translation>工程已自动保存</translation>
    </message>
</context>
<context>
    <name>DA::DAActionsInterface</name>
    <message>
        <location filename="../DAInterface/DAActionsInterface.cpp" line="+119"/>
        <source>DAActionsInterface::recordAction received a null action</source>
        <translation>DAActionsInterface::recordAction 收到空 action</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>DAActionsInterface::recordAction(QAction objname=%1) received a duplicate object name, the previous record will be overwritten</source>
        <translation>DAActionsInterface::recordAction(QAction objname=%1) 收到重复的对象名，之前的记录将被覆盖</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentBridge</name>
    <message>
        <location filename="../DAAgent/DAAgentBridge.cpp" line="+181"/>
        <source>Agent process startup timed out</source>
        <translation>Agent 进程启动超时</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Agent subprocess not ready within %1 ms, initialization may have failed, check logs</source>
        <translation>Agent 子进程启动后 %1 毫秒内未就绪，初始化可能失败，请查看日志排查</translation>
    </message>
    <message>
        <location line="+297"/>
        <source>Failed to write to agent subprocess stdin</source>
        <translation>写入 agent 子进程 stdin 失败</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Failed to parse JSON line from agent stdout: %1, error: %2</source>
        <translation>解析 agent 标准输出的 JSON 行失败：%1，错误：%2</translation>
    </message>
    <message>
        <location line="+442"/>
        <source>Failed to parse trailing JSON line from agent stdout: %1, error: %2</source>
        <translation>解析 agent 标准输出的末尾 JSON 行失败：%1，错误：%2</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>Agent process crashed (exit code %1), recovering... (%2/%3)</source>
        <translation>Agent 进程异常退出（代码 %1），正在恢复... (%2/%3)</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Agent process crashed repeatedly (%1 times), please restart the application</source>
        <translation>Agent 进程多次异常退出（%1 次），请重启程序</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Agent response timeout (no activity for %1 minutes)</source>
        <translation>Agent 响应超时（%1 分钟无活动）</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentDockWidget</name>
    <message>
        <location filename="../DAGui/Agent/DAAgentDockWidget.cpp" line="+247"/>
        <source>Ready</source>
        <translation>就绪</translation>
    </message>
    <message>
        <location line="-2"/>
        <source>Send</source>
        <translation>发送</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Stopping...</source>
        <translation>终止中...</translation>
    </message>
    <message>
        <location line="-1"/>
        <source>Agent thinking...</source>
        <translation>Agent 思考中...</translation>
    </message>
    <message>
        <location line="-3"/>
        <source>Stop</source>
        <translation>终止</translation>
    </message>
    <message>
        <location line="-109"/>
        <source>Session Manager</source>
        <translation>会话管理</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>New Session</source>
        <translation>新建会话</translation>
    </message>
    <message>
        <location line="+796"/>
        <source>tokens: ~%1 / %2</source>
        <translation>token: ~%1 / %2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>tokens: %1 / %2</source>
        <translation>token: %1 / %2</translation>
    </message>
    <message>
        <location line="-690"/>
        <source>tokens: -</source>
        <translation>token: -</translation>
    </message>
    <message>
        <location line="+654"/>
        <source>(untitled)</source>
        <translation>（未命名）</translation>
    </message>
    <message>
        <location line="-653"/>
        <source>input: %1</source>
        <translation>输入：%1</translation>
    </message>
    <message>
        <location line="-5"/>
        <source>Agent starting...</source>
        <translation>Agent 启动中...</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Type a message...</source>
        <translation>输入消息...</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>output: %1</source>
        <translation>输出：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>total: %1</source>
        <translation>总计：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>window: %1</source>
        <translation>窗口：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>source: %1</source>
        <translation>来源：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>unknown</source>
        <translation>未知</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No model</source>
        <translation>无模型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Select LLM model</source>
        <translation>选择 LLM 模型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Providers</source>
        <translation>供应商</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Back</source>
        <translation>返回</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Details</source>
        <translation>详细信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copied</source>
        <translation>已复制</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>[truncated]</source>
        <translation>[已截断]</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Permission mode</source>
        <translation>权限模式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Full Auto</source>
        <translation>全自动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Auto</source>
        <translation>自动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ask Every Time</source>
        <translation>每次询问</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Run everything without asking (system directories still blocked)</source>
        <translation>全部直接执行不再询问（系统目录仍拦截）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reads and chart edits pass; file writes and code execution judged by rules</source>
        <translation>读取与图表编辑放行；文件写入与代码执行按规则判定</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>File writes and code execution need approval every time</source>
        <translation>文件写入与代码执行每次都需批准</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Switch to Full Auto mode? Code execution and file writes will no longer ask for confirmation.</source>
        <translation>切换到全自动模式？代码执行与文件写入将不再请求确认。</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Switch</source>
        <translation>切换</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>needs your approval</source>
        <translation>需要你的批准</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Approve</source>
        <translation>批准</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Deny</source>
        <translation>拒绝</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Approve &amp;&amp; remember for this session</source>
        <translation>批准并本会话记住</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Approved</source>
        <translation>已批准</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Denied</source>
        <translation>已拒绝</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Approved (remembered for this session)</source>
        <translation>已批准（本会话已记住）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 more lines</source>
        <translation>还有 %1 行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>From subagent: %1</source>
        <translation>来自子 Agent：%1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>%1 subagent task(s)</source>
        <translation>%1 个子 Agent 任务</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1/%2 done</source>
        <translation>%1/%2 已完成</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>completed</source>
        <translation>已完成</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>queued</source>
        <translation>排队中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>running</source>
        <translation>运行中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>done</source>
        <translation>完成</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>failed</source>
        <translation>失败</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>timeout</source>
        <translation>超时</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>stopped</source>
        <translation>已停止</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>The permission mode is Full Auto from last session. Code execution and file writes will run without asking. Keep Full Auto mode?</source>
        <translation>上次会话留在全自动权限模式。代码执行与文件写入将不再询问直接执行。是否保持全自动模式？</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Keep Full Auto</source>
        <translation>保持全自动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Switch to Auto</source>
        <translation>切换为自动</translation>
    </message>
    <message>
        <location line="+636"/>
        <source>API quota exhausted, please check account balance or change API key</source>
        <translation>API 配额已耗尽，请检查账户余额或更换 API Key</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>API key invalid or expired, please check settings</source>
        <translation>API Key 无效或已过期，请在设置中检查配置</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Failed after %1 retries: rate limited</source>
        <translation>重试 %1 次后仍失败：服务限流</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Failed after %1 retries: network error</source>
        <translation>重试 %1 次后仍失败：网络错误</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Failed after %1 retries: server error</source>
        <translation>重试 %1 次后仍失败：服务器错误</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Request format error: %1</source>
        <translation>请求格式错误：%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Context window exceeded and compaction failed</source>
        <translation>上下文窗口超限且压缩失败</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Agent response timeout (no activity for %1 minutes)</source>
        <translation>Agent 响应超时（%1 分钟无活动）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Agent process crashed, recovering... (%1/3)</source>
        <translation>Agent 进程异常退出，正在恢复... (%1/3)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Agent process crashed repeatedly, unable to recover</source>
        <translation>Agent 进程多次崩溃，无法恢复</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Failed to switch model, keeping current model</source>
        <translation>模型切换失败，已保留当前模型</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Agent error: %1</source>
        <translation>Agent 错误：%1</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentEditorDialog</name>
    <message>
        <location filename="../APP/Dialog/DAAgentEditorDialog.cpp" line="+27"/>
        <source>New Agent</source>
        <translation>新增 Agent</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Edit Agent</source>
        <translation>编辑 Agent</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Title:</source>
        <translation>标题：</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enter agent title</source>
        <translation>请输入 Agent 标题</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Prompt content:</source>
        <translation>提示词内容：</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Write prompt content here (Markdown supported)</source>
        <translation>在此编写提示词内容（支持 Markdown）</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+24"/>
        <location line="+8"/>
        <source>Tip</source>
        <translation>提示</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>Title cannot be empty</source>
        <translation>标题不能为空</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>An agent named &quot;%1&quot; already exists, please choose another title</source>
        <translation>已存在同名 Agent「%1」，请更换标题</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentManagerDialog</name>
    <message>
        <location filename="../APP/Dialog/DAAgentManagerDialog.cpp" line="+25"/>
        <source>Agent Manager</source>
        <translation>Agent 管理</translation>
    </message>
    <message>
        <location line="+29"/>
        <location line="+46"/>
        <source>Add</source>
        <translation>添加</translation>
    </message>
    <message>
        <location line="-45"/>
        <location line="+46"/>
        <source>Edit</source>
        <translation>编辑</translation>
    </message>
    <message>
        <location line="-45"/>
        <location line="+46"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="-40"/>
        <source>Prompt Library</source>
        <translation>提示词库</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Subagent definitions available for dispatch. Edit to change tools or instructions.</source>
        <translation>可派发的子 Agent 定义，编辑可调整工具白名单与提示词</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Subagents</source>
        <translation>子 Agent</translation>
    </message>
    <message>
        <location line="+70"/>
        <location line="+40"/>
        <location line="+28"/>
        <location line="+81"/>
        <location line="+45"/>
        <location line="+29"/>
        <source>Tip</source>
        <translation>提示</translation>
    </message>
    <message>
        <location line="-222"/>
        <location line="+40"/>
        <location line="+109"/>
        <location line="+45"/>
        <source>Save failed</source>
        <translation>保存失败</translation>
    </message>
    <message>
        <location line="-138"/>
        <location line="+155"/>
        <source>Confirm Delete</source>
        <translation>确认删除</translation>
    </message>
    <message>
        <location line="-154"/>
        <source>Are you sure to delete agent &quot;%1&quot;?</source>
        <translation>确定删除 Agent「%1」吗？</translation>
    </message>
    <message>
        <location line="+11"/>
        <location line="+155"/>
        <source>Delete failed</source>
        <translation>删除失败</translation>
    </message>
    <message>
        <location line="-11"/>
        <source>Are you sure to delete subagent &quot;%1&quot;?</source>
        <translation>确定删除子 Agent「%1」吗？</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentModule</name>
    <message>
        <location filename="../DAAgent/DAAgentModule.cpp" line="+160"/>
        <source>The agent ended this turn after %1 tool calls, but its last message looks like an unfinished plan (e.g. announcing a next step without executing it). Send a message such as &quot;continue&quot; to let it finish.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+134"/>
        <source>Agent system prompt file is empty, fallback to built-in default: %1</source>
        <translation>Agent 系统提示词文件为空，回退到内置默认提示词：%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Failed to read Agent system prompt file, fallback to built-in default: %1</source>
        <translation>读取 Agent 系统提示词文件失败，回退到内置默认提示词：%1</translation>
    </message>
    <message>
        <location line="+141"/>
        <source>LLM is not configured, cannot start agent. Please configure LLM in settings first.</source>
        <translation>LLM 未配置，无法启动 Agent，请先在设置中配置 LLM</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+732"/>
        <source>LLM is not configured. Please configure LLM in settings first.</source>
        <translation>LLM 未配置，请先在设置中配置 LLM</translation>
    </message>
    <message>
        <location line="-721"/>
        <source>Cannot find Python interpreter path, please configure it in settings</source>
        <translation>无法找到 Python 解释器路径，请在设置页配置 Python 解释器</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Cannot find agent_runner.py path: %1</source>
        <translation>无法找到 agent_runner.py 路径: %1</translation>
    </message>
    <message>
        <location line="+715"/>
        <source>LLM is not configured, skip agent analysis. Please configure LLM in settings first.</source>
        <translation>LLM 未配置，跳过 Agent 分析，请先在设置中配置 LLM</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentPermissionSettingsWidget</name>
    <message>
        <location filename="../APP/SettingPages/DAAgentPermissionSettingsWidget.cpp" line="+116"/>
        <source>Default Permission Mode</source>
        <translation>默认权限模式</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Auto (rules + judge)</source>
        <translation>自动（规则+判官）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Manual (ask every write/code)</source>
        <translation>手动（写入/代码每次询问）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Full Auto (yolo)</source>
        <translation>全自动（yolo）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Mode:</source>
        <translation>模式：</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Approval</source>
        <translation>审批</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Max wait for user approval on file writes / code execution</source>
        <translation>文件写入/代码执行等待用户批准的最长时间</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Approval timeout:</source>
        <translation>审批超时：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>In manual mode, also ask before in-app chart edits</source>
        <translation>手动模式下，应用内图表修改也需询问</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Code Judge (optional)</source>
        <translation>代码判官（可选）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Leave empty to disable the judge</source>
        <translation>留空则不启用判官</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Judge model:</source>
        <translation>判官模型：</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Judge timeout:</source>
        <translation>判官超时：</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>When the judge model is not configured, Auto mode still asks for approval on every code execution (even if no dangerous pattern matches). Configuring a judge enables automatic allow/deny for gray-area code, and code matching no pattern is then allowed silently.</source>
        <translation>未配置判官模型时，自动模式对代码执行仍会逐次询问（即使未命中任何危险模式）；配置判官后可对灰区代码自动放行/拒绝，未命中模式的代码将静默放行。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>File Path Rules</source>
        <translation>文件路径规则</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Evaluated in order, first match wins; file writes matching no rule ask for approval. Variables: ${workspace}, ${project}, ${data}, ${exe}, ${home}. Global deny rows (tool = *, action = deny) are hard safety rules and cannot be edited or removed.</source>
        <translation>按顺序求值、首条命中生效；未命中规则的写入将征求确认。可用变量：${workspace}、${project}、${data}、${exe}、${home}。全局拒绝行（工具 = *、动作 = deny）为硬性安全规则，不可编辑或删除。</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+60"/>
        <source>Tool</source>
        <translation>工具</translation>
    </message>
    <message>
        <location line="-59"/>
        <source>Scope (glob)</source>
        <translation>范围（glob）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Action</source>
        <translation>动作</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Add Rule</source>
        <translation>新增规则</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+59"/>
        <source>Remove Selected</source>
        <translation>删除选中</translation>
    </message>
    <message>
        <location line="-51"/>
        <source>Code Danger Patterns (Auto mode)</source>
        <translation>代码危险模式（自动模式）</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>One regular expression per line; invalid expressions are skipped. Deny patterns reject code outright; Escalate patterns are referred to the judge model. Note: this judging layer is an advisory defense, not a security boundary — static patterns can be bypassed by obfuscation (e.g. indirect attribute access, importing a malicious module from a clean entry script, rewriting the script after judging). For strict scenarios use Manual mode.</source>
        <translation>每行一条正则表达式；非法表达式将被跳过。拒绝（deny）模式直接拒绝代码；升级（escalate）模式交由判官模型裁决。注意：判定层是咨询性防线而非安全边界——静态模式可被混淆绕过（如间接属性访问、经干净入口脚本导入恶意模块、判定后改写脚本等）。严格场景请使用手动（manual）模式。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Deny patterns (matched =&gt; reject):</source>
        <translation>拒绝模式（命中即拒绝）：</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+5"/>
        <source>One regex per line</source>
        <translation>每行一条正则</translation>
    </message>
    <message>
        <location line="-2"/>
        <source>Escalate patterns (matched =&gt; consult judge):</source>
        <translation>升级模式（命中交判官裁决）：</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Tool Tier Overrides</source>
        <translation>工具分级覆盖</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Explicitly assign a risk tier to a tool (typically plugin tools). Unlisted unknown plugin tools default to the &quot;unknown&quot; tier, which asks for approval in Auto/Manual modes; e.g. map a known-reversible chart plugin tool to inapp_mutate to let it pass silently.</source>
        <translation>为工具显式指定风险分级（通常用于插件工具）。未列出的未知插件工具默认归入 &quot;unknown&quot; 分级，在自动/手动模式下会征求确认；例如可把已知可逆的图表类插件工具归入 inapp_mutate 使其静默放行。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Tier</source>
        <translation>分级</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Add Override</source>
        <translation>新增覆盖</translation>
    </message>
    <message>
        <location line="+115"/>
        <source>Hard safety rule: cannot be edited or removed</source>
        <translation>硬性安全规则：不可编辑或删除</translation>
    </message>
    <message>
        <location filename="../APP/SettingPages/DAAgentPermissionSettingsWidget.h" line="+35"/>
        <source>Agent Permission Settings</source>
        <translation>Agent 权限设置</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentSettingsWidget</name>
    <message>
        <location filename="../APP/SettingPages/DAAgentSettingsWidget.cpp" line="+152"/>
        <location line="+5"/>
        <source> s</source>
        <translation>秒</translation>
    </message>
    <message>
        <location line="+35"/>
        <source> d</source>
        <translation>天</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+18"/>
        <source> times</source>
        <translation>次</translation>
    </message>
    <message>
        <location line="-12"/>
        <location line="+6"/>
        <location line="+35"/>
        <source> sec</source>
        <translation>秒</translation>
    </message>
    <message>
        <location line="-134"/>
        <source>Base URL</source>
        <translation>基础地址</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>API Key</source>
        <translation>API 密钥</translation>
    </message>
    <message>
        <location line="+112"/>
        <location line="+6"/>
        <location line="+21"/>
        <source>No limit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-26"/>
        <source>Max graph reasoning steps per turn (each tool-call cycle consumes 3 steps). Check &apos;No limit&apos; to disable. Recommended: 150.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Unlimited reasoning steps per turn. Loop protection still applies: repeated identical tool calls are terminated automatically.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Max reasoning steps for each subagent task. Set to -1 for no limit. Recommended: 60.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Ready Timeout</source>
        <translation>就绪超时</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Stop Timeout</source>
        <translation>停止超时</translation>
    </message>
    <message>
        <location line="-185"/>
        <source>Add Provider</source>
        <translation>新增供应商</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Edit Provider</source>
        <translation>修改供应商</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Remove Provider</source>
        <translation>删除供应商</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Configured LLM providers. Select to view details; edit via the buttons above.</source>
        <translation>已配置的 LLM 供应商，选中查看详情，通过上方按钮编辑</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Models</source>
        <translation>模型</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Model Name</source>
        <translation>模型名</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Context Size</source>
        <translation>上下文大小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max Output Tokens</source>
        <translation>最大输出 token</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Model Providers</source>
        <translation>模型供应商</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Waiting time (seconds) for the agent subprocess to become ready. Recommended: 60.</source>
        <translation>子进程就绪等待超时(秒)。建议 60。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Waiting time (seconds) for the subprocess to exit on stop. Recommended: 5.</source>
        <translation>停止时等待子进程退出超时(秒)。建议 5。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Compaction trigger ratio of the context window (per active model). 0.85 = compact at 85%.</source>
        <translation>压缩触发比例(相对激活模型上下文窗口)。0.85=85% 时触发。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Messages retained verbatim after compaction. Recommended: 10.</source>
        <translation>压缩后保留为原文的最近消息条数。建议 10。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Tool output truncation threshold (chars). Recommended: 20000.</source>
        <translation>工具输出截断阈值(字符)。建议 20000。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Preview length (chars) of truncated tool output. Recommended: 2000.</source>
        <translation>工具输出截断预览长度(字符)。建议 2000。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Max free sessions retained. Recommended: 20.</source>
        <translation>保留的自由会话最大数量。建议 20。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Free sessions older than this are deleted on startup. Recommended: 30.</source>
        <translation>早于此天数的自由会话启动时删除。建议 30。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Max automatic retries on transient LLM errors. 0 disables. Recommended: 7.</source>
        <translation>临时错误自动重试次数。0 不重试。建议 7。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Timeout (seconds) for a single LLM request. Recommended: 120.</source>
        <translation>单次 LLM 请求超时(秒)。建议 120。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Watchdog timeout: stop subprocess if no message within this period. Recommended: 240.</source>
        <translation>看门狗超时:此时间内无消息则停止子进程。建议 240。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Max auto restarts after subprocess crash. 0 disables. Recommended: 3.</source>
        <translation>子进程崩溃后自动重启最大次数。0 不重启。建议 3。</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Wall-clock timeout (seconds) for each subagent task. Waiting for approval counts towards this limit. Recommended: 600.</source>
        <translation>单个子 Agent 任务的墙钟超时(秒)。等待用户批准也计入该时限。建议 600。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Prestart the agent subprocess on launch. Disable to save memory.</source>
        <translation>启动时预启动 agent 子进程。关闭可节省内存。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Compaction Threshold</source>
        <translation>压缩阈值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max Recent Messages</source>
        <translation>保留最近消息数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tool Result Max Chars</source>
        <translation>工具结果截断阈值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tool Result Preview Chars</source>
        <translation>工具结果预览长度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max Sessions</source>
        <translation>最大会话数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Session Retention Days</source>
        <translation>会话保留天数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max retries</source>
        <translation>最大重试次数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Request timeout</source>
        <translation>请求超时</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Inactivity timeout</source>
        <translation>无活动超时</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max process restarts</source>
        <translation>最大进程重启次数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Reasoning iteration limit</source>
        <translation>推理迭代上限</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Subagent Timeout</source>
        <translation>子 Agent 超时</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Subagent Reasoning Limit</source>
        <translation>子 Agent 推理上限</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Auto prestart on launch</source>
        <translation>启动时自动预热</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Agent Settings</source>
        <translation>Agent 设置</translation>
    </message>
    <message>
        <location line="+100"/>
        <source>(unnamed)</source>
        <translation>（未命名）</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>not set</source>
        <translation>未设置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>set (hidden)</source>
        <translation>已设置(隐藏)</translation>
    </message>
    <message>
        <location filename="../APP/SettingPages/DAAgentSettingsWidget.h" line="+39"/>
        <source>Agent LLM Settings</source>
        <translation>Agent LLM 设置</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentSubagentEditDialog</name>
    <message>
        <location filename="../APP/Dialog/DAAgentSubagentEditDialog.cpp" line="+61"/>
        <source>Unregistered tool (kept in definition)</source>
        <translation>未注册的工具（保留在定义中）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Edit Subagent</source>
        <translation>编辑子 Agent</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>New Subagent</source>
        <translation>新增子 Agent</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>snake_case name used by dispatch</source>
        <translation>snake_case 名称（派发时引用）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Name:</source>
        <translation>名称：</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Short description shown to the main agent</source>
        <translation>展示给主 Agent 的简短描述</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Description:</source>
        <translation>描述：</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Allowed tools:</source>
        <translation>工具白名单：</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Only checked tools can be called by this subagent. Permission rules still apply on every call.</source>
        <translation>子 Agent 只能调用勾选的工具；每次调用仍受权限规则约束</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>System prompt:</source>
        <translation>系统提示词：</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Write the subagent instructions here (Markdown supported). It should finish autonomously and end with a structured summary.</source>
        <translation>在此编写子 Agent 指引（支持 Markdown），应自主完成任务并输出结构化总结</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+20"/>
        <location line="+6"/>
        <source>Tip</source>
        <translation>提示</translation>
    </message>
    <message>
        <location line="-5"/>
        <source>Name cannot be empty</source>
        <translation>名称不能为空</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>A subagent named &quot;%1&quot; already exists, please choose another name</source>
        <translation>已存在同名子 Agent「%1」，请更换名称</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentToolsPlugin</name>
    <message>
        <location filename="../../plugins/DAAgentTools/DAAgentToolsPlugin.h" line="+31"/>
        <source>Platform built-in agent tools</source>
        <translation>平台内置 agent 工具</translation>
    </message>
</context>
<context>
    <name>DA::DAAgentWebChannel</name>
    <message>
        <location filename="../DAGui/Agent/DAAgentWebChannel.cpp" line="+185"/>
        <source>Submit</source>
        <translation>提交</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Type your own answer...</source>
        <translation>输入自定义回答...</translation>
    </message>
</context>
<context>
    <name>DA::DAAppActions</name>
    <message>
        <location filename="../APP/DAAppActions.cpp" line="+416"/>
        <source>Open</source>
        <translation>打开</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open file or project</source>
        <translation>打开文件或项目</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save file or project</source>
        <translation>保存文件或项目</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save As</source>
        <translation>另存为</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save file or project as</source>
        <translation>保存文件或项目为</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Append To Project</source>
        <translation>追加到工程</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Append file or project to current project</source>
        <translation>附加文件或项目到当前项目</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Rename Columns</source>
        <translation>重命名列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rename columns in the selected table</source>
        <translation>重命名选中表格的列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Setting</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Setting for the application</source>
        <translation>应用程序设置</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>About</source>
        <translation>关于</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>About the application</source>
        <translation>关于应用程序</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Add 
Data</source>
        <translation>添加
数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add data to the table</source>
        <translation>添加数据到表格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove 
Data</source>
        <translation>移除
数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove data from the table</source>
        <translation>从表格中移除数据</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Add 
Figure</source>
        <translation>添加
绘图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a figure to the workspace</source>
        <translation>添加绘图到工作区</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Resize 
Chart</source>
        <translation>绘图
尺寸</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Resize the sub-chart</source>
        <translation>调整子图尺寸</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>New 
XY Axis</source>
        <translation>新建
坐标系</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a new XY axis to the figure</source>
        <translation>新建坐标系</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Curve</source>
        <translation>曲线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a curve to the chart</source>
        <translation>添加折线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Scatter</source>
        <translation>散点图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a scatter plot to the chart</source>
        <translation>添加散点图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Error Bar</source>
        <translation>误差棒图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add an error bar to the chart</source>
        <translation>添加误差棒图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Box Plot</source>
        <translation>箱线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a box plot to the chart</source>
        <translation>添加箱线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Bar</source>
        <translation>柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a bar chart to the chart</source>
        <translation>添加柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
MultiBar</source>
        <translation>多重柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a multi-bar chart to the chart</source>
        <translation>添加多重柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Histogram</source>
        <translation>分布图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a histogram to the chart</source>
        <translation>添加分布图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Contour Map</source>
        <translation>等高线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a contour map to the chart</source>
        <translation>添加等高线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Cloud Map</source>
        <translation>云图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a cloud map to the chart</source>
        <translation>添加云图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Vector Field</source>
        <translation>向量场图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a vector field to the chart</source>
        <translation>添加向量场图</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Apply All Charts</source>
        <translation>应用到
所有绘图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>When this feature is selected, operations on the figure will apply to all plots, not just the currently selected one</source>
        <translation>此功能选中后，绘图面板上的操作将应用到所有绘图，而不仅仅是当前选中的绘图</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enable Grid</source>
        <translation>网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable grid in the chart</source>
        <translation>启用或禁用图表中的网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>X Grid</source>
        <translation>横向网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable X grid in the chart</source>
        <translation>启用或禁用图表中的横向网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y Grid</source>
        <translation>纵向网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable Y grid in the chart</source>
        <translation>启用或禁用图表中的纵向网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Xmin Grid</source>
        <translation>横向密集网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable Xmin grid in the chart</source>
        <translation>启用或禁用图表中的横向密集网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ymin Grid</source>
        <translation>纵向密集网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable Ymin grid in the chart</source>
        <translation>启用或禁用图表中的纵向密集网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom</source>
        <translation>缩放</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable zoom in the chart</source>
        <translation>启用或禁用图表中的缩放</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom In</source>
        <translation>放大</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom in on the chart</source>
        <translation>放大</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom Out</source>
        <translation>缩小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom out of the chart</source>
        <translation>缩小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
All</source>
        <translation>显示
全部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom to show all data in the chart</source>
        <translation>缩放以显示所有数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pan</source>
        <translation>拖动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable pan in the chart</source>
        <translation>启用或禁用图表中的拖动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cross</source>
        <translation>十字标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable cross picker in the chart</source>
        <translation>启用或禁用图表中的十字标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y Picker</source>
        <translation>y值拾取</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>At Canvas Left Top</source>
        <translation>在画布左上</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Set picker text at canvas left top corner</source>
        <translation>设置拾取文本在画布左上角</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>At Canvas Left Bottom</source>
        <translation>在画布左下</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Set picker text at canvas left bottom corner</source>
        <translation>设置拾取文本在画布左下角</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>At Canvas Right Top</source>
        <translation>在画布右上</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Set picker text at canvas right top corner</source>
        <translation>设置拾取文本在画布右上角</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y Picker Show X Value</source>
        <translation>y拾取显示x值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable X value display for Y picker</source>
        <translation>启用或禁用y拾取显示x值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>At Canvas Right Bottom</source>
        <translation>在画布右下</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Set picker text at canvas right bottom corner</source>
        <translation>设置拾取文本在画布右下角</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Follow Mouse</source>
        <translation>跟随鼠标</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Set picker text to follow mouse cursor</source>
        <translation>设置拾取文本跟随鼠标</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>XY Picker</source>
        <translation>点拾取</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable XY picker in the chart</source>
        <translation>启用或禁用图表中的点拾取</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Link All Picker</source>
        <translation>联动
拾取</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable all picker linked</source>
        <translation>启用或禁用所有拾取联动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Legend</source>
        <translation>图例</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>To Numeric</source>
        <translation>转换为数值类型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cast to numeric type</source>
        <translation>转换为数值类型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>To String</source>
        <translation>转换为字符串类型</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>To Datetime</source>
        <translation>转换为日期类型</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Grouping</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Ungroup</source>
        <translation>取消分组</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>Show Information Window</source>
        <translation>信息窗口</translation>
    </message>
    <message>
        <location line="-137"/>
        <source>Enable or disable legend in the chart</source>
        <translation>启用或禁用图表中的图例</translation>
    </message>
    <message>
        <location line="-126"/>
        <source>Open Markdown</source>
        <translation>打开 Markdown</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open a Markdown file and display it in the central area</source>
        <translation>打开 Markdown 文件并在中央区显示</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Rename 
Data</source>
        <translation>重命名
            数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rename the selected dataset</source>
        <translation>重命名选中的数据集</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export 
Data</source>
        <translation>导出
            数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export the selected data to a file</source>
        <translation>导出选中的数据到文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export CSV</source>
        <translation>导出 CSV</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export the selected data to a CSV file</source>
        <translation>导出选中的数据为 CSV 文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export Excel</source>
        <translation>导出 Excel</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export the selected data to an Excel file</source>
        <translation>导出选中的数据为 Excel 文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export Pickle</source>
        <translation>导出 Pickle</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export the selected data to a pickle file</source>
        <translation>导出选中的数据为 Pickle 文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export Parquet</source>
        <translation>导出 Parquet</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export the selected data to a parquet file</source>
        <translation>导出选中的数据为 Parquet 文件</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Add 
Surface 3D</source>
        <translation>3D曲面图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a 3D surface plot to the chart</source>
        <translation>添加3D曲面图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Bar 3D</source>
        <translation>3D柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a 3D bar chart to the chart</source>
        <translation>添加3D柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Line 3D</source>
        <translation>3D线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a 3D line plot to the chart</source>
        <translation>添加3D线图</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Histplot</source>
        <translation>直方图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Histogram with optional KDE overlay</source>
        <translation>直方图（可叠加核密度曲线）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>KDE 1D</source>
        <translation>一维核密度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>1D Kernel Density Estimation plot</source>
        <translation>一维核密度图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>KDE 2D</source>
        <translation>二维核密度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>2D Kernel Density Estimation plot</source>
        <translation>二维核密度图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Boxplot</source>
        <translation>箱线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Box plot with statistics</source>
        <translation>统计箱线图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Heatmap</source>
        <translation>热力图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Heatmap matrix plot</source>
        <translation>热力图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scatter</source>
        <translation>散点图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scatter plot with grouping</source>
        <translation>统计散点图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Barplot</source>
        <translation>柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bar plot with aggregation</source>
        <translation>统计柱状图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Regplot</source>
        <translation>回归图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Regression plot with CI</source>
        <translation>回归图（含置信区间）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ECDF</source>
        <translation>累积分布</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Empirical Cumulative Distribution Function</source>
        <translation>经验累积分布图</translation>
    </message>
    <message>
        <location line="+47"/>
        <source>Copy To Clipboard</source>
        <translation>复制到
剪切板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy the figure to the clipboard</source>
        <translation>将绘图复制到剪切板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Legend Top</source>
        <translation>图例在上</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Place the legend at the top of the chart</source>
        <translation>图例置于图表上方</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Legend Bottom</source>
        <translation>图例在下</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Place the legend at the bottom of the chart</source>
        <translation>图例置于图表下方</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Legend Left</source>
        <translation>图例在左</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Place the legend at the left of the chart</source>
        <translation>图例置于图表左侧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Legend Right</source>
        <translation>图例在右</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Place the legend at the right of the chart</source>
        <translation>图例置于图表右侧</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pointer</source>
        <translation>指针</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Select chart elements by clicking, drag to move, Delete to remove</source>
        <translation>点击选择绘图元素，拖动移动，Delete删除</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Rect</source>
        <translation>添加矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a rectangle to the chart</source>
        <translation>添加矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Ellipse</source>
        <translation>添加椭圆</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add an ellipse to the chart</source>
        <translation>添加椭圆</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Polygon</source>
        <translation>添加多边形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a polygon to the chart</source>
        <translation>添加多边形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Cross Marker</source>
        <translation>添加十字</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a cross marker to the chart</source>
        <translation>添加十字标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add H Line Marker</source>
        <translation>添加水平线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add an H line marker to the chart</source>
        <translation>添加水平线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add V Line Marker</source>
        <translation>添加垂直线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a V line marker to the chart</source>
        <translation>添加垂直标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Arrow Marker</source>
        <translation>添加箭头</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add an arrow marker to the chart</source>
        <translation>添加箭头标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Text</source>
        <translation>添加文本</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a text marker to the chart</source>
        <translation>添加文本标注</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Horizontal Probe</source>
        <translation>添加水平数据探针标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a horizontal plot probe marker to the chart</source>
        <translation>水平数据探针</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add Vertical Probe</source>
        <translation>垂直数据探针</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a vertical plot probe marker to the chart</source>
        <translation>添加垂直数据探针标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data Picker Setting</source>
        <translation>数据拾取设置</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Configure data picker properties</source>
        <translation>配置数据拾取器属性</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Remove Row</source>
        <translation>删除行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove a row from the table</source>
        <translation>从表中删除行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove Column</source>
        <translation>删除列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove a column from the table</source>
        <translation>从表中删除列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove Cell</source>
        <translation>删除单元格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove a cell from the table</source>
        <translation>从表中移除单元格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert 
Row</source>
        <translation>插入
行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert a row into the table</source>
        <translation>插入行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert Row(Above)</source>
        <translation>插入行(上)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert a row above the current row</source>
        <translation>插入行(上)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert 
Column</source>
        <translation>插入
列(右)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert a column to the right of the current column</source>
        <translation>插入列(右)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert Column(Left)</source>
        <translation>插入列(左)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Insert a column to the left of the current column</source>
        <translation>插入列(左)</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Cast to string type</source>
        <translation>转换为字符串类型</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cast to datetime type</source>
        <translation>转换为日期类型</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>To Index</source>
        <translation>转换为索引</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cast to index type</source>
        <translation>转换为索引类型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rename Column</source>
        <translation>重命名此列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rename the column under the cursor</source>
        <translation>重命名光标所在列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy Column Name</source>
        <translation>复制列名</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy the column name to clipboard</source>
        <translation>复制列名到剪贴板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Goto Max</source>
        <translation>跳转到最大值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scroll to the maximum value of this column</source>
        <translation>滚动到此列最大值处</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Goto Min</source>
        <translation>跳转到最小值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scroll to the minimum value of this column</source>
        <translation>滚动到此列最小值处</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Statistics</source>
        <translation>显示统计信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show statistics of this column via pandas describe</source>
        <translation>通过pandas describe显示此列的统计信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clear Selected Style</source>
        <translation>清除选中样式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clear styles in selected cells</source>
        <translation>清除选中单元格的样式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clear All Style</source>
        <translation>清除所有样式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clear all styles in current table</source>
        <translation>清除当前表格所有样式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Format Cells...</source>
        <translation>设置单元格格式...</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Set number/date display format of the selected column</source>
        <translation>设置选中列的数值/日期显示格式</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>New 
Workflow</source>
        <translation>新建
工作流</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Create a new workflow</source>
        <translation>创建新工作流</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Linkage 
Move</source>
        <translation>联动</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>When moving elements, other elements linked to this element follow the movement</source>
        <translation>允许移动图元时，其它和此图元链接起来的图元跟随移动</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Group selected elements</source>
        <translation>对选中的元素进行分组</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Ungroup selected elements</source>
        <translation>对选中的元素进行取消分组</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Draw 
Rect</source>
        <translation>绘制
矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Draw a rectangle on the workflow scene</source>
        <translation>在工作流场景中绘制矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Draw 
Text</source>
        <translation>绘制
文本</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Draw text on the workflow scene</source>
        <translation>在工作流场景中绘制文本</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
Grid</source>
        <translation>显示
网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show grid on the workflow scene</source>
        <translation>在工作流场景中显示网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Lock 
View</source>
        <translation>锁定
视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Lock the workflow view</source>
        <translation>锁定工作流视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Run 
Workflow</source>
        <translation>运行
工作流</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Run the workflow</source>
        <translation>运行工作流</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Terminate 
Workflow</source>
        <translation>结束</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Terminate the workflow</source>
        <translation>停止工作流</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Link</source>
        <translation>连线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enable or disable link between elements</source>
        <translation>启用或禁用元素之间连线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add 
Background</source>
        <translation>添加
背景</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Add a background pixmap to the workflow scene</source>
        <translation>在工作流场景中添加背景图片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Lock Background</source>
        <translation>锁定背景</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Lock the background pixmap in the workflow scene</source>
        <translation>锁定工作流场景中的背景图片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Move With Background</source>
        <translation>元件随背景移动</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable or disable item move with background pixmap</source>
        <translation>启用或禁用元件随背景移动</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Export To Image</source>
        <translation>导出为图片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export the workflow scene to an image</source>
        <translation>导出工作流场景为图片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export To PNG</source>
        <translation>导出PNG</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export the workflow scene to a PNG image</source>
        <translation>导出工作流场景为PNG图片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Marker</source>
        <translation>显示标记</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show marker on the workflow scene</source>
        <translation>在工作流场景中显示标记</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Show 
Workflow Area</source>
        <translation>工作流
操作视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the workflow area</source>
        <translation>显示工作流区域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
Workflow Manager</source>
        <translation>工作流
管理视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the workflow manager area</source>
        <translation>显示工作流管理区域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
Chart Area</source>
        <translation>绘图
操作视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the chart area</source>
        <translation>显示绘图区域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
Chart Manager</source>
        <translation>绘图
管理视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the chart manager area</source>
        <translation>显示绘图管理区域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
Table Area</source>
        <translation>数据
操作区域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the data area</source>
        <translation>显示数据区域</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
Data Manager</source>
        <translation>数据
管理视图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the data manager area</source>
        <translation>显示数据管理区域</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show the message log window</source>
        <translation>显示信息窗口</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Setting Window</source>
        <translation>设置窗口</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the setting window</source>
        <translation>显示设置窗口</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Left 
Side Bar</source>
        <translation>左侧边栏</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the left side bar</source>
        <translation>显示左侧边栏</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Right 
Side Bar</source>
        <translation>右侧边栏</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the right side bar</source>
        <translation>显示右侧边栏</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show 
Agent Area</source>
        <translation>Agent
助手</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the agent assistant area</source>
        <translation>显示 Agent 助手区域</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Plugin 
Config</source>
        <translation>插件
设置</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show the plugin manager</source>
        <translation>显示插件管理器</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cut</source>
        <translation>剪切</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cut the selection to the clipboard</source>
        <translation>剪切选中内容到剪贴板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy the selection to the clipboard</source>
        <translation>复制选中内容到剪贴板</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Paste</source>
        <translation>粘贴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Paste from the clipboard</source>
        <translation>从剪贴板粘贴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Delete the selection</source>
        <translation>删除选中内容</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Select All</source>
        <translation>全选</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Select all content</source>
        <translation>全选内容</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reset 
Layout</source>
        <translation>恢复
默认布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Restore the default window layout (confirmation required)</source>
        <translation>恢复默认窗口布局（需确认）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Manage 
Layouts</source>
        <translation>布局
管理</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Open the layout manager to save, apply or remove layout schemes</source>
        <translation>打开布局管理对话框，保存、应用或删除布局方案</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Redo</source>
        <translation>重做</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Redo the last action</source>
        <translation>重做</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Undo</source>
        <translation>撤销</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Undo the last action</source>
        <translation>撤销</translation>
    </message>
</context>
<context>
    <name>DA::DAAppController</name>
    <message>
        <location filename="../APP/DAAppController.cpp" line="+739"/>
        <source>Save Project</source>
        <translation>保存工程</translation>
    </message>
    <message>
        <location line="+19"/>
        <location line="+847"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-836"/>
        <source>Failed to save project! Path: %1</source>
        <translation>工程保存失败！路径为:%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+614"/>
        <source>Project saved successfully, path: %1</source>
        <translation>工程保存成功，路径为:%1</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Project loaded successfully, path: %1</source>
        <translation>工程加载成功，路径为:%1</translation>
    </message>
    <message>
        <location line="+162"/>
        <source>Image saved successfully to %1</source>
        <translation>图片保存成功：%1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Failed to save image to %1</source>
        <translation>图片保存失败：%1</translation>
    </message>
    <message>
        <location line="+110"/>
        <source>Reset Layout</source>
        <translation>恢复默认布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>This will restore the default window layout. Continue?</source>
        <translation>将恢复默认窗口布局，是否继续？</translation>
    </message>
    <message>
        <location line="+383"/>
        <source>Please select a dataset to rename</source>
        <translation>请先选中要重命名的数据集</translation>
    </message>
    <message>
        <location line="+59"/>
        <source>Please select a dataset to export</source>
        <translation>请先选中要导出的数据</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>CSV File</source>
        <translation>CSV 文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Excel File</source>
        <translation>Excel 文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pickle File</source>
        <translation>Pickle 文件</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Parquet File</source>
        <translation>Parquet 文件</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Export Data</source>
        <translation>导出数据</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Cannot determine export format, please select a file suffix</source>
        <translation>无法确定导出格式，请选择带后缀的文件名</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Data exported successfully, path: %1</source>
        <translation>数据导出成功，路径:%1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Data export failed, path: %1, reason: %2</source>
        <translation>数据导出失败，路径:%1，原因:%2</translation>
    </message>
    <message>
        <location line="+75"/>
        <source>Before creating a new coordinate, you need to create a figure</source>
        <translation>在创建一个坐标系之前，需要先创建一个绘图窗口</translation>
    </message>
    <message>
        <location line="-1489"/>
        <source>Please select the data operation window</source>
        <translation>请选中数据操作窗口</translation>
    </message>
    <message>
        <location line="-50"/>
        <location line="+523"/>
        <location line="+73"/>
        <source>Project File</source>
        <translation>工程文件</translation>
    </message>
    <message>
        <location line="-578"/>
        <source>Whether to overwrite the file: %1</source>
        <translation>是否覆盖文件:%1</translation>
    </message>
    <message>
        <location line="+465"/>
        <location line="+19"/>
        <source>Question</source>
        <translation>疑问</translation>
    </message>
    <message>
        <location line="-18"/>
        <source>The current project has unsaved changes. Do you want to save before opening another project?</source>
        <translation>当前工程有未保存的更改，是否在打开其他工程之前保存？</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Another project already exists. Do you want to replace it?</source>
        <translation>已存在其他工程，是否要替换？</translation>
    </message>
    <message>
        <location line="+67"/>
        <location line="+43"/>
        <source>Failed to load project file: %1</source>
        <translation>加载工程文件失败:%1</translation>
    </message>
    <message>
        <location line="+147"/>
        <source>Image files</source>
        <translation>图片文件</translation>
    </message>
    <message>
        <location line="-212"/>
        <location line="+213"/>
        <source>Any files</source>
        <translation>任意文件</translation>
    </message>
    <message>
        <location line="-214"/>
        <source>Markdown files</source>
        <translation>Markdown 文件</translation>
    </message>
    <message>
        <location line="+311"/>
        <source>Received null project interface</source>
        <translation>获取到空工程接口</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Windows 7</source>
        <translation>Windows 7</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2013</source>
        <translation>Office 2013</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2016 Blue</source>
        <translation>Office 2016 蓝色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2016 Green</source>
        <translation>Office 2016 绿色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2016 Dark</source>
        <translation>Office 2016 深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2021 Blue</source>
        <translation>Office 2021 蓝色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2021 Green</source>
        <translation>Office 2021 绿色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2021 Dark</source>
        <translation>Office 2021 深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark</source>
        <translation>深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark 2</source>
        <translation>深色2</translation>
    </message>
    <message>
        <location line="+96"/>
        <source>No cells selected to copy</source>
        <translation>没有选中可复制的单元格</translation>
    </message>
    <message>
        <location line="+321"/>
        <source>Cannot rename a series, please select a dataset</source>
        <translation>无法重命名series，请选中数据集</translation>
    </message>
    <message>
        <location line="+204"/>
        <source>Figure &apos;%1&apos; not found, it may have been closed or renamed</source>
        <translation>未找到绘图&quot;%1&quot;，可能已关闭或被重命名</translation>
    </message>
    <message>
        <location line="+731"/>
        <source>Rename Column</source>
        <translation>重命名此列</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>New column name:</source>
        <translation>新列名：</translation>
    </message>
    <message>
        <location line="+65"/>
        <source>Cannot find the maximum value in this column (empty, all-NaN, or incomparable types)</source>
        <translation>此列无法找到最大值（空列、全为NaN或类型不可比较）</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Column [%1] maximum value: %2, row: %3</source>
        <translation>列[%1] 最大值: %2, 第 %3 行</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Column [%1] maximum value at row %2</source>
        <translation>列[%1] 最大值位于第 %2 行</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Cannot find the minimum value in this column (empty, all-NaN, or incomparable types)</source>
        <translation>此列无法找到最小值（空列、全为NaN或类型不可比较）</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Column [%1] minimum value: %2, row: %3</source>
        <translation>列[%1] 最小值: %2, 第 %3 行</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Column [%1] minimum value at row %2</source>
        <translation>列[%1] 最小值位于第 %2 行</translation>
    </message>
    <message>
        <location line="+187"/>
        <source>Export</source>
        <translation>导出</translation>
    </message>
    <message>
        <location line="+156"/>
        <source>New workflow name</source>
        <translation>新工作流名称</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>New workflow name:</source>
        <translation>新工作流名称</translation>
    </message>
    <message>
        <location line="+177"/>
        <source>Please select a valid column</source>
        <translation>请选择正确的列</translation>
    </message>
    <message>
        <location line="+55"/>
        <location line="+35"/>
        <source>No figure/chart available for statistical plot</source>
        <translation>没有可用于统计绘图的图表窗口</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>No data column selected</source>
        <translation>未选择数据列</translation>
    </message>
    <message>
        <location line="-58"/>
        <location line="+36"/>
        <source>Cannot resolve the data source for statistical plot; please ensure a dataframe is selected in the settings window</source>
        <translation>无法确定统计绘图的数据源，请确保在设置窗口中已选择数据表</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>The selected data source is empty</source>
        <translation>选中的数据源为空</translation>
    </message>
    <message>
        <location line="-2178"/>
        <source>Before running the workflow, you need to save the project</source>
        <translation>在运行工作流之前，需要先保存工程</translation>
    </message>
</context>
<context>
    <name>DA::DAAppDataManager</name>
    <message>
        <location filename="../APP/DAAppDataManager.cpp" line="+35"/>
        <source>Begin importing file: %1</source>
        <translation>开始导入文件:%1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Python scripts not initialized, cannot import file: %1</source>
        <translation>Python脚本未初始化，无法导入文件:%1</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Only DataFrame data can be exported</source>
        <translation>仅支持导出 DataFrame 数据</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Python scripts not initialized</source>
        <translation>Python 脚本未初始化</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Python scripts not initialized, cannot export file: %1</source>
        <translation>Python脚本未初始化，无法导出文件:%1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data is empty, cannot export</source>
        <translation>数据为空，无法导出</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Unsupported export format: %1</source>
        <translation>不支持的导出格式:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAAppDockingArea</name>
    <message>
        <location filename="../APP/DAAppDockingArea.cpp" line="+70"/>
        <source>Workflow Node</source>
        <translation>节点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Charts Manager</source>
        <translation>绘图管理</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data Manager</source>
        <translation>数据管理</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Workflow Operate</source>
        <translation>工作流操作</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Chart Operate</source>
        <translation>绘图操作</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data Operate</source>
        <translation>数据操作</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Setting</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Log</source>
        <translation>消息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Agent Assistant</source>
        <translation>Agent 助手</translation>
    </message>
    <message>
        <location line="+453"/>
        <source>Markdown Viewer</source>
        <translation>Markdown 查看器</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Failed to open markdown file: %1</source>
        <translation>打开 Markdown 文件失败:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAAppLayoutManager</name>
    <message>
        <location filename="../APP/DAAppLayoutManager.cpp" line="+93"/>
        <source>Layout scheme &apos;%1&apos; not found</source>
        <translation>未找到布局方案&quot;%1&quot;</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Failed to restore layout scheme &apos;%1&apos;, it may be saved by an incompatible version</source>
        <translation>布局方案&quot;%1&quot;恢复失败，可能由不兼容的版本保存</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Preset layout schemes cannot be removed</source>
        <translation>预置布局方案不可删除</translation>
    </message>
</context>
<context>
    <name>DA::DAAppPluginManager</name>
    <message>
        <location filename="../APP/DAAppPluginManager.cpp" line="+210"/>
        <source>Python interpreter not initialized, skip Python node discovery</source>
        <translation>Python解释器未初始化，跳过Python节点发现</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Python node discovery failed</source>
        <translation>Python节点发现失败</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Python node factory initialization failed: %1</source>
        <translation>Python节点工厂初始化失败:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAAppProject</name>
    <message>
        <location filename="../APP/DAAppProject.cpp" line="+826"/>
        <source>Saving project</source>
        <translation>正在保存工程</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>The file %1 is not a valid project file</source>
        <translation>文件%1不是正确的工程文件</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Loading project</source>
        <translation>正在加载工程</translation>
    </message>
    <message>
        <location line="+47"/>
        <source>Save Project</source>
        <translation>保存工程</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Project Files</source>
        <translation>工程文件</translation>
    </message>
    <message>
        <location line="-56"/>
        <source>Creating project snapshot</source>
        <translation>正在创建工程快照</translation>
    </message>
    <message>
        <location line="-9"/>
        <source>Loading project %1 cancelled by user</source>
        <translation>用户取消了工程%1的加载</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Failed to backup current project</source>
        <translation>无法备份当前工程</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Failed to back up the current project before loading %1</source>
        <translation>加载%1前备份当前工程失败</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Restoring previous project</source>
        <translation>正在恢复之前的工程</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Failed to load project, restored previous project</source>
        <translation>工程加载失败，已恢复之前的工程</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Failed to load project and failed to restore previous project</source>
        <translation>工程加载失败，且恢复之前的工程失败</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Failed to restore previous project from snapshot</source>
        <translation>从快照恢复之前的工程失败</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Failed to save project! Path: %1</source>
        <translation>工程保存失败！路径为:%1</translation>
    </message>
    <message>
        <location line="+168"/>
        <source>Load script workspace</source>
        <translation>加载脚本工作区</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Extract workspace/ to local cache directory</source>
        <translation>解压工程内脚本工作区到本地缓存目录</translation>
    </message>
    <message>
        <location line="+148"/>
        <source>Script Workspace Conflict</source>
        <translation>脚本工作区冲突</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The local script workspace of this project differs from the version stored in the project file. Which version do you want to keep?</source>
        <translation>本工程的本地脚本工作区与工程文件内保存的版本不一致，请选择保留哪个版本</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Local workspace: %1</source>
        <translation>本地工作区：%1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Keep Local</source>
        <translation>保留本地</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Overwrite With Project Version</source>
        <translation>用工程内版本覆盖</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Save System Info</source>
        <translation>保存系统信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save system information</source>
        <translation>保存系统信息</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Failed to serialize workflow &apos;%1&apos; to XML</source>
        <translation>序列化工作流&apos;%1&apos;到XML失败</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Save workflow data</source>
        <translation>保存工作流数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save Python workflow logic data (nodes, parameters, connections)</source>
        <translation>保存Python工作流逻辑数据（节点、参数、连接关系）</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Save workflow information</source>
        <translation>保存工作流信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save workflow information, including the hierarchical relationships and rendering effects of workflow graphics elements</source>
        <translation>保存工作流信息，包括工作流图元的层级关系渲染效果</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>An exception occurred while serializing the dataframe named %1 to %2</source>
        <translation>把名称为%1的dataframe序列化到%2时出现异常</translation>
    </message>
    <message>
        <location line="+148"/>
        <source>Save data operate layout</source>
        <translation>保存数据操作布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save opened data pages and dock layout</source>
        <translation>保存已打开数据页与停靠布局</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Save agent session</source>
        <translation>保存Agent会话</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save agent chat session history</source>
        <translation>保存Agent聊天会话历史</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Save script workspace</source>
        <translation>保存脚本工作区</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pack local script workspace into the project file</source>
        <translation>把本地脚本工作区打包进工程文件</translation>
    </message>
    <message>
        <location line="+81"/>
        <source>Loading workflow</source>
        <translation>正在加载工作流</translation>
    </message>
    <message>
        <location line="+66"/>
        <source>This project file was saved with an older version (%1). Saving it will upgrade to %2 and enable the script workspace feature</source>
        <translation>该工程文件由旧版本(%1)保存，保存后将升级到%2并启用脚本工作区功能</translation>
    </message>
    <message>
        <location line="+295"/>
        <source>Table style for data &apos;%1&apos; has no matching data, skipped</source>
        <translation>数据&apos;%1&apos;的表格样式未找到匹配数据，已跳过</translation>
    </message>
    <message>
        <location line="+92"/>
        <source>Data operate page &apos;%1&apos; has no matching data, skipped</source>
        <translation>数据操作页&apos;%1&apos;未找到匹配数据，已跳过</translation>
    </message>
    <message>
        <location line="-703"/>
        <source>Save data information, including data names and data organization formats</source>
        <translation>保存数据信息，包括数据的名称数据的组织形式</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Save charts information</source>
        <translation>保存绘图的基本信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save charts information, including chart name and chart organization formats</source>
        <translation>保存绘图信息，包括绘图的名称绘图的组织形式</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+1"/>
        <source>Save chart items information</source>
        <translation>保存绘图元素的基本信息</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Save table styles</source>
        <translation>保存表格样式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save table cell styles, including background, font, foreground</source>
        <translation>保存表格单元格样式</translation>
    </message>
    <message>
        <location line="+162"/>
        <source>Begin saving archive to %1</source>
        <translation>开始保存档案到%1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Begin loading archive from %1</source>
        <translation>开始加载%1</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Loading data</source>
        <translation>正在加载数据</translation>
    </message>
    <message>
        <location line="+260"/>
        <source>Unable to serialize file %1 into a DataFrame</source>
        <translation>无法把文件%1序列化为DataFrame</translation>
    </message>
    <message>
        <location line="-241"/>
        <source>Project saved successfully</source>
        <translation>成功保存工程</translation>
    </message>
    <message>
        <location line="-877"/>
        <location line="+880"/>
        <source>Failed to save project</source>
        <translation>无法保存工程</translation>
    </message>
    <message>
        <location line="-890"/>
        <location line="+34"/>
        <source>The current project is busy</source>
        <translation>当前工程正繁忙</translation>
    </message>
    <message>
        <location line="+580"/>
        <source>Save data information</source>
        <translation>保存数据信息</translation>
    </message>
    <message>
        <location line="+297"/>
        <source>Project loaded successfully</source>
        <translation>成功加载工程</translation>
    </message>
    <message>
        <location line="-834"/>
        <location line="+838"/>
        <source>Failed to load project</source>
        <translation>无法加载工程</translation>
    </message>
    <message>
        <location line="-29"/>
        <source>Successfully saved archive: %1</source>
        <translation>成功保存工程:%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Failed to save archive: %1</source>
        <translation>无法保存工程:%1</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Successfully loaded archive: %1</source>
        <translation>成功加载工程:%1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Failed to load archive: %1</source>
        <translation>无法加载工程:%1</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Failed to parse workflow-data.xml</source>
        <translation>解析workflow-data.xml失败</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Failed to create workflow tab: %1</source>
        <translation>创建工作流标签页失败:%1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Empty Python workflow data for tab: %1</source>
        <translation>工作流标签页%1的Python数据为空</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Failed to deserialize Python workflow: %1</source>
        <translation>反序列化Python工作流失败:%1</translation>
    </message>
    <message>
        <location line="+68"/>
        <source>appendWorkflowView: tab &apos;%1&apos; not found, skipping view load</source>
        <translation>appendWorkflowView: 未找到标签页&apos;%1&apos;，跳过视图加载</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Missing data content</source>
        <translation>保存</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Python script is not initialized</source>
        <translation>脚本没有初始化</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Unable to find the temporary file corresponding to %1</source>
        <translation>无法在找到%1对应的临时文件</translation>
    </message>
</context>
<context>
    <name>DA::DAAppRibbonArea</name>
    <message>
        <location filename="../APP/DAAppRibbonArea.cpp" line="+190"/>
        <location line="+3"/>
        <source>File</source>
        <translation>文件</translation>
    </message>
    <message>
        <location line="-1"/>
        <source>Main</source>
        <translation>主页</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Config</source>
        <translation>配置</translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+346"/>
        <source>Workflow</source>
        <translation>工作流</translation>
    </message>
    <message>
        <location line="-369"/>
        <source>Data Operation</source>
        <translation>数据操作</translation>
    </message>
    <message>
        <location line="-1"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="-2"/>
        <source>Create</source>
        <translation>创建</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Export Format</source>
        <translation>导出格式</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+28"/>
        <location line="+413"/>
        <source>View</source>
        <translation>视图</translation>
    </message>
    <message>
        <location line="-440"/>
        <source>Display</source>
        <translation>视图显示</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Layout</source>
        <translation>布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+231"/>
        <source>DataFrame</source>
        <translation>DataFrame</translation>
    </message>
    <message>
        <location line="-230"/>
        <location line="+232"/>
        <source>Operate</source>
        <translation>操作</translation>
    </message>
    <message>
        <location line="-231"/>
        <location line="+234"/>
        <source>Axes</source>
        <translation>Axes</translation>
    </message>
    <message>
        <location line="-233"/>
        <location line="+246"/>
        <source>Column</source>
        <translation>列</translation>
    </message>
    <message>
        <location line="-245"/>
        <location line="+6"/>
        <location line="+249"/>
        <location line="+5"/>
        <source>Type</source>
        <translation>类型</translation>
    </message>
    <message>
        <location line="-259"/>
        <location line="+6"/>
        <location line="+264"/>
        <location line="+4"/>
        <source>Format</source>
        <translation>格式</translation>
    </message>
    <message>
        <location line="-273"/>
        <location line="+279"/>
        <source>Table Style</source>
        <translation>表格样式</translation>
    </message>
    <message>
        <location line="-278"/>
        <location line="+282"/>
        <source>Fill</source>
        <translation>底色</translation>
    </message>
    <message>
        <location line="-281"/>
        <location line="+289"/>
        <source>Font</source>
        <translation>字体</translation>
    </message>
    <message>
        <location line="-288"/>
        <location line="+294"/>
        <source>Clear</source>
        <translation>清除</translation>
    </message>
    <message>
        <location line="-288"/>
        <source>Fill Color</source>
        <translation>填充颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+356"/>
        <source>Workflow Edit</source>
        <translation>编辑</translation>
    </message>
    <message>
        <location line="-387"/>
        <location line="+32"/>
        <location line="+360"/>
        <source>Clipboard</source>
        <translation>剪切板</translation>
    </message>
    <message>
        <location line="-346"/>
        <location line="+465"/>
        <source>Chart Style</source>
        <translation>图表样式</translation>
    </message>
    <message>
        <location line="+376"/>
        <location line="+2"/>
        <source>AI Agent</source>
        <translation>AI智能体</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Agent Manager</source>
        <translation>agent管理</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Manage agents: add, edit, delete prompts</source>
        <translation>管理 agent：新增、修改、删除提示词</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Run Agent</source>
        <translation>执行agent</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Run AI analysis with the selected agent prompt</source>
        <translation>使用当前选中的 agent 提示词执行 AI 分析</translation>
    </message>
    <message>
        <location line="+56"/>
        <source>Agent</source>
        <translation>Agent</translation>
    </message>
    <message>
        <location line="+59"/>
        <location line="+7"/>
        <source>Tip</source>
        <translation>提示</translation>
    </message>
    <message>
        <location line="-6"/>
        <source>Please select an agent in the gallery first</source>
        <translation>请先在 gallery 中选择一个 agent</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Agent module is not ready</source>
        <translation>Agent 模块未就绪</translation>
    </message>
    <message>
        <location line="-993"/>
        <location line="+364"/>
        <source>Item</source>
        <translation>图元</translation>
    </message>
    <message>
        <location line="-363"/>
        <location line="+372"/>
        <source>Text</source>
        <translation>文本</translation>
    </message>
    <message>
        <location line="-371"/>
        <location line="+376"/>
        <source>Background</source>
        <translation>背景</translation>
    </message>
    <message>
        <location line="-373"/>
        <location line="+454"/>
        <source>Workflow Run</source>
        <translation>运行</translation>
    </message>
    <message>
        <location line="-453"/>
        <location line="+456"/>
        <source>Run</source>
        <translation>运行</translation>
    </message>
    <message>
        <location line="-454"/>
        <location line="+289"/>
        <source>Figure</source>
        <translation>绘图</translation>
    </message>
    <message>
        <location line="-283"/>
        <source>Figure Setting</source>
        <translation>绘图设置</translation>
    </message>
    <message>
        <location line="-5"/>
        <source>Add Chart</source>
        <translation>添加绘图</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Chart</source>
        <translation>绘图</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+514"/>
        <source>Chart Edit</source>
        <translation>绘图编辑</translation>
    </message>
    <message>
        <location line="-516"/>
        <source>Chart Setting</source>
        <translation>绘图设置</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Figure Theme</source>
        <translation>主题</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+523"/>
        <source>Select Tool</source>
        <translation>选区工具</translation>
    </message>
    <message>
        <location line="-522"/>
        <source>Chart Assist Tool</source>
        <translation>图表辅助工具</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+1"/>
        <source>View Marker</source>
        <translation>视图标记</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Export Image</source>
        <translation>导出为图片</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export Workflow Graphics Scene To Image</source>
        <translation>把工作流的场景导出为图片</translation>
    </message>
    <message>
        <location line="-15"/>
        <location line="+311"/>
        <source>Stats Plot</source>
        <translation>统计绘图</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Group</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Workflow View</source>
        <translation>显示</translation>
    </message>
    <message>
        <location line="-441"/>
        <location line="+477"/>
        <source>Export</source>
        <translation>导出</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Chart Operate</source>
        <translation>绘图操作</translation>
    </message>
    <message>
        <location line="-497"/>
        <location line="+539"/>
        <source>Theme</source>
        <translation>主题</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Assist Tools</source>
        <translation>辅助工具</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Recent Files</source>
        <translation>最近打开的文件</translation>
    </message>
</context>
<context>
    <name>DA::DAAxObjectExcelWrapper</name>
    <message>
        <location filename="../DAAxOfficeWrapper/DAAxObjectExcelWrapper.cpp" line="+913"/>
        <location line="+32"/>
        <source>The local computer does not have Excel or WPS installed</source>
        <translation>当前计算机中没有安装excel或者wps</translation>
    </message>
    <message>
        <location line="-26"/>
        <source>cannot open Excel</source>
        <translation>无法打开Excel</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>cannot get or create sheet</source>
        <translation>无法获取或创建工作表</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>failed to write table to sheet</source>
        <translation>写入表格数据到sheet失败</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>failed to save Excel file</source>
        <translation>保存Excel文件失败</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DAxisSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DAxisSettingPanel.cpp" line="+127"/>
        <source>Axis Selector</source>
        <translation>轴选择</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Axis</source>
        <translation>坐标轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Label</source>
        <translation>标签</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Text</source>
        <translation>标签文本</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Font</source>
        <translation>标签字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Color</source>
        <translation>标签颜色</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Numbers</source>
        <translation>数字</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Number Font</source>
        <translation>数字字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Number Color</source>
        <translation>数字颜色</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Range</source>
        <translation>范围</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Min Range</source>
        <translation>最小范围</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Range</source>
        <translation>最大范围</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Major Count</source>
        <translation>主刻度数</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Minor Count</source>
        <translation>次刻度数</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Line Width</source>
        <translation>线宽</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tick Position</source>
        <translation>刻度位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bottom</source>
        <translation>底部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Top</source>
        <translation>顶部</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Smooth Line</source>
        <translation>平滑线</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Grid</source>
        <translation>网格</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Major Grid</source>
        <translation>主网格线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Minor Grid</source>
        <translation>次网格线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Grid Color</source>
        <translation>网格颜色</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DBarSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DBarSettingPanel.cpp" line="+35"/>
        <source>General</source>
        <translation>通用</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bar Style</source>
        <translation>柱样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Filled</source>
        <translation>填充</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Filled Mesh</source>
        <translation>填充网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Wireframe</source>
        <translation>线框</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bar Width</source>
        <translation>柱宽</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Bar Depth</source>
        <translation>柱深</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Baseline</source>
        <translation>基线</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Colormap</source>
        <translation>色图</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Mesh Color</source>
        <translation>网格颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Mesh Line Width</source>
        <translation>网格线宽</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DColorLegendSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DColorLegendSettingPanel.cpp" line="+69"/>
        <source>Display</source>
        <translation>视图显示</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Visible</source>
        <translation>可见</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2"/>
        <source>Position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Top Left</source>
        <translation>左上</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Top Center</source>
        <translation>上中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Top Right</source>
        <translation>右上</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Left Center</source>
        <translation>左中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Center</source>
        <translation>居中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Right Center</source>
        <translation>右中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bottom Left</source>
        <translation>左下</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bottom Center</source>
        <translation>下中</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bottom Right</source>
        <translation>右下</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Custom</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Absolute X</source>
        <translation>绝对位置X</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Absolute Y</source>
        <translation>绝对位置Y</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Absolute Width</source>
        <translation>绝对宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Absolute Height</source>
        <translation>绝对高度</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DCoordSysSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DCoordSysSettingPanel.cpp" line="+70"/>
        <location line="+2"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Box</source>
        <translation>盒形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Frame</source>
        <translation>框架</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Colors</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Axes Color</source>
        <translation>轴颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Number Color</source>
        <translation>数字颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Color</source>
        <translation>标签颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Grid Lines Color</source>
        <translation>网格线颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Interior Grid Color</source>
        <translation>内部网格颜色</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Fonts</source>
        <translation>字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Number Font</source>
        <translation>数字字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Font</source>
        <translation>标签字体</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Ticks</source>
        <translation>刻度</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tic Length</source>
        <translation>刻度长度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tic Length Scale</source>
        <translation>刻度缩放</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Auto Scale</source>
        <translation>自动缩放</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Auto Decoration</source>
        <translation>自动装饰</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tick Position</source>
        <translation>刻度位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bottom</source>
        <translation>底部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Top</source>
        <translation>顶部</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Interior Grid Width</source>
        <translation>内部网格线宽</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Interior Major Width</source>
        <translation>内部主网格线宽</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Interior Minor Width</source>
        <translation>内部次网格线宽</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Line</source>
        <translation>线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Line Smooth</source>
        <translation>线平滑</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DItemSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DItemSettingPanel.cpp" line="+72"/>
        <source>No Plot</source>
        <translation>无绘图</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+99"/>
        <source>Wireframe</source>
        <translation>线框</translation>
    </message>
    <message>
        <location line="-98"/>
        <source>Hidden Line</source>
        <translation>隐藏线</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+95"/>
        <source>Filled</source>
        <translation>填充</translation>
    </message>
    <message>
        <location line="-94"/>
        <location line="+95"/>
        <source>Filled Mesh</source>
        <translation>填充网格</translation>
    </message>
    <message>
        <location line="-94"/>
        <source>Points</source>
        <translation>点</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Flat</source>
        <translation>平面着色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Gouraud</source>
        <translation>平滑着色</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>No Floor</source>
        <translation>无投影</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Isoline</source>
        <translation>等高线投影</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>No Coord</source>
        <translation>无坐标系</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Box</source>
        <translation>盒形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Frame</source>
        <translation>框架</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Linear</source>
        <translation>线性</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Log10</source>
        <translation>对数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>User</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Lines</source>
        <translation>线条</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tube</source>
        <translation>管状</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dots</source>
        <translation>点</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Dot</source>
        <translation>圆点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cube</source>
        <translation>立方体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tetrahedron</source>
        <translation>四面体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Octahedron</source>
        <translation>八面体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sphere</source>
        <translation>球体</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Default</source>
        <translation>默认</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark</source>
        <translation>深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scientific</source>
        <translation>科学</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Warm</source>
        <translation>暖色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cool</source>
        <translation>冷色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Matplotlib</source>
        <translation>Matplotlib</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Earth Tones</source>
        <translation>大地色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ocean</source>
        <translation>海洋</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>High Contrast</source>
        <translation>高对比</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Presentation</source>
        <translation>演示</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>No Lighting</source>
        <translation>无光照</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Flat Light</source>
        <translation>平面光</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Studio</source>
        <translation>工作室</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Outdoor</source>
        <translation>户外</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Soft</source>
        <translation>柔和</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DLineSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DLineSettingPanel.cpp" line="+35"/>
        <source>General</source>
        <translation>通用</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Line</source>
        <translation>线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Line Style</source>
        <translation>线样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Lines</source>
        <translation>线条</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tube</source>
        <translation>管状</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dots</source>
        <translation>点</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Line Width</source>
        <translation>线宽</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tube Radius</source>
        <translation>管半径</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tube Segments</source>
        <translation>管段数</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Points</source>
        <translation>点</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show Points</source>
        <translation>显示点</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Point Size</source>
        <translation>点大小</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Point Shape</source>
        <translation>点形状</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Dot</source>
        <translation>圆点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cube</source>
        <translation>立方体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tetrahedron</source>
        <translation>四面体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Octahedron</source>
        <translation>八面体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sphere</source>
        <translation>球体</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Use Data Color</source>
        <translation>使用数据着色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Solid Color</source>
        <translation>纯色</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Colormap</source>
        <translation>色图</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DPlotSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DPlotSettingPanel.cpp" line="+70"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title Text</source>
        <translation>标题文本</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title Font</source>
        <translation>标题字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title Color</source>
        <translation>标题颜色</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Background</source>
        <translation>背景</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Background Color</source>
        <translation>背景色</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2"/>
        <source>Projection</source>
        <translation>投影</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Perspective</source>
        <translation>透视</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Orthographic</source>
        <translation>正交</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Aspect Ratio</source>
        <translation>纵横比</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Auto Fill</source>
        <translation>自动填充</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data Ratio</source>
        <translation>数据比例</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Lighting</source>
        <translation>光照</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Lighting</source>
        <translation>启用光照</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Lighting Preset</source>
        <translation>光照预设</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No Lighting</source>
        <translation>无光照</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Flat Light</source>
        <translation>平面光</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Studio</source>
        <translation>工作室</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Outdoor</source>
        <translation>户外</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Soft</source>
        <translation>柔和</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Shininess</source>
        <translation>光泽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Specular Intensity</source>
        <translation>镜面强度</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+2"/>
        <source>Theme</source>
        <translation>主题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Default</source>
        <translation>默认</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark</source>
        <translation>深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scientific</source>
        <translation>科学</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Warm</source>
        <translation>暖色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cool</source>
        <translation>冷色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Matplotlib</source>
        <translation>Matplotlib</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Earth Tones</source>
        <translation>大地色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ocean</source>
        <translation>海洋</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>High Contrast</source>
        <translation>高对比</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Presentation</source>
        <translation>演示</translation>
    </message>
    <message>
        <location line="+14"/>
        <location line="+5"/>
        <source>Reset View</source>
        <translation>重置视图</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DSettingWidget</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DSettingWidget.cpp" line="+177"/>
        <source>3D Chart Area</source>
        <translation>3D图表区</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Coordinate System</source>
        <translation>坐标系</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Z Axis</source>
        <translation>Z轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Color Legend</source>
        <translation>颜色图例</translation>
    </message>
    <message>
        <location line="+211"/>
        <source>Unnamed Item</source>
        <translation>未命名项</translation>
    </message>
</context>
<context>
    <name>DA::DAChart3DSurfaceSettingPanel</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DSurfaceSettingPanel.cpp" line="+37"/>
        <source>General</source>
        <translation>通用</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Plot Style</source>
        <translation>绘图样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Wireframe</source>
        <translation>线框</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Hidden Line</source>
        <translation>隐藏线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Filled</source>
        <translation>填充</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Filled Mesh</source>
        <translation>填充网格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Points</source>
        <translation>点</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Shading</source>
        <translation>着色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Flat</source>
        <translation>平面着色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Gouraud</source>
        <translation>平滑着色</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Polygon Offset</source>
        <translation>多边形偏移</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Resolution</source>
        <translation>分辨率</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Colormap</source>
        <translation>色图</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Mesh Color</source>
        <translation>网格颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Mesh Line Width</source>
        <translation>网格线宽</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Isolines</source>
        <translation>等值线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Isolines Count</source>
        <translation>等值线数量</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Smooth Mesh</source>
        <translation>平滑网格</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Floor</source>
        <translation>底面投影</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Floor Style</source>
        <translation>底面样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Iso</source>
        <translation>等值线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Normals</source>
        <translation>法线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show Normals</source>
        <translation>显示法线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Normal Length</source>
        <translation>法线长度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Normal Quality</source>
        <translation>法线质量</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAdd3DBarWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAdd3DBarWidget.cpp" line="+110"/>
        <source>Bar3D</source>
        <translation>3D柱状</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAdd3DLineWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAdd3DLineWidget.cpp" line="+58"/>
        <source>Line3D</source>
        <translation>3D线图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAdd3DSurfaceWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAdd3DSurfaceWidget.cpp" line="+122"/>
        <source>Surface3D</source>
        <translation>3D曲面</translation>
    </message>
    <message>
        <location line="+124"/>
        <source>scipy is not available, cannot perform scatter interpolation. Please install scipy: pip install scipy</source>
        <translation>scipy 不可用，无法进行散点插值。请安装 scipy：pip install scipy</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddBoxChartWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddBoxChartWidget.cpp" line="+77"/>
        <location line="+18"/>
        <location line="+28"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-45"/>
        <source>Please select a dataframe</source>
        <translation>请选择一个数据框</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Please select at least one column</source>
        <translation>请至少选择一列</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Exception occurred during box chart data extraction:%1</source>
        <translation>箱线图数据提取过程中出现异常:%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Failed to extract data</source>
        <translation>数据提取失败</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddContourWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddContourWidget.cpp" line="+18"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Value</source>
        <translation>值</translation>
    </message>
    <message>
        <location line="+31"/>
        <location line="+9"/>
        <location line="+9"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-17"/>
        <source>X, Y and Value must be series</source>
        <translation>X、Y和Value必须是序列</translation>
    </message>
    <message>
        <location line="+9"/>
        <location line="+9"/>
        <source>The selected data cannot be converted to a series</source>
        <translation>所选数据无法转换为序列</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Exception occurred during extracting contour data:%1</source>
        <translation>提取等高线数据过程中出现异常:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddErrorBarWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddErrorBarWidget.cpp" line="+22"/>
        <source>Error Bar</source>
        <translation>误差棒</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddHistogramWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddHistogramWidget.cpp" line="+18"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+39"/>
        <location line="+11"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-10"/>
        <location line="+11"/>
        <source>Please select a series</source>
        <translation>请选择一个序列</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Exception occurred during extracting series:%1</source>
        <translation>提取序列过程中出现异常:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddMultiBarWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddMultiBarWidget.cpp" line="+84"/>
        <location line="+9"/>
        <location line="+26"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-34"/>
        <source>The initial value and step of x auto increment must be floating-point numbers</source>
        <translation>x自增序列的初始值和步长必须为浮点数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Please drag a series into the X list</source>
        <translation>警告 / 请把一个序列拖入X列表</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Please drag at least one series into the Y list</source>
        <translation>警告 / 请至少把一个序列拖入Y列表</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Exception occurred during extracting y series:%1</source>
        <translation>提取y序列过程中出现异常:%1</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>Exception occurred during building multi-bar samples:%1</source>
        <translation>构建多重柱状图样本过程中出现异常:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddOHLCSeriesWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddOHLCSeriesWidget.cpp" line="+36"/>
        <location line="+9"/>
        <source>Time</source>
        <translation>时间</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>Open</source>
        <translation>开盘</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>High</source>
        <translation>最高</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>Low</source>
        <translation>最低</translation>
    </message>
    <message>
        <location line="-8"/>
        <location line="+9"/>
        <source>Close</source>
        <translation>收盘</translation>
    </message>
    <message>
        <location line="+166"/>
        <location line="+8"/>
        <location line="+43"/>
        <location line="+6"/>
        <location line="+6"/>
        <location line="+6"/>
        <location line="+27"/>
        <location line="+14"/>
        <location line="+7"/>
        <location line="+6"/>
        <location line="+6"/>
        <location line="+6"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-134"/>
        <source>The initial value of t auto increment series must be a floating-point arithmetic number</source>
        <translation>t自增序列的初始值必须为浮点数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The step value of t auto increment series must be a floating-point arithmetic number</source>
        <translation>t自增序列的步长必须为浮点数</translation>
    </message>
    <message>
        <location line="+43"/>
        <location line="+66"/>
        <source>open value must be a series</source>
        <translation>开盘值必须是序列</translation>
    </message>
    <message>
        <location line="-60"/>
        <location line="+66"/>
        <source>high value must be a series</source>
        <translation>最高值必须是序列</translation>
    </message>
    <message>
        <location line="-60"/>
        <location line="+66"/>
        <source>low value must be a series</source>
        <translation>最低值必须是序列</translation>
    </message>
    <message>
        <location line="-60"/>
        <location line="+66"/>
        <source>close value must be a series</source>
        <translation>收盘值必须是序列</translation>
    </message>
    <message>
        <location line="-44"/>
        <location line="+73"/>
        <source>Exception occurred during extracting from pandas.Series to double vector:%1</source>
        <translation>从pandas.Series提取为double vector过程中出现异常:%1</translation>
    </message>
    <message>
        <location line="-68"/>
        <source>Exception occurred during extracting from pandas.Series to double vector</source>
        <translation>从pandas.Series提取为double vector过程中出现异常</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>time value must be a series</source>
        <translation>时间必须是序列</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsBarplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsBarplotWidget.cpp" line="+20"/>
        <source>X axis (categorical)</source>
        <translation>X 轴（分类）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Y axis (numeric, optional)</source>
        <translation>Y 轴（数值，可选）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Hue</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+67"/>
        <source>Please select an X-axis data column before plotting</source>
        <translation>请先选择 X 轴数据列再绘图</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>Barplot Settings</source>
        <translation>柱状图设置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Countplot Settings</source>
        <translation>计数图设置</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsBoxplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsBoxplotWidget.cpp" line="+21"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Hue</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+42"/>
        <location line="+18"/>
        <source>Please select one or more data columns before plotting</source>
        <translation>请先选择数据列再绘图</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Please select a data column before plotting</source>
        <translation>请先选择数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsEcdfplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsEcdfplotWidget.cpp" line="+20"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Hue</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Weights (optional)</source>
        <translation>权重（可选）</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>Please select a data column before plotting</source>
        <translation>请先选择数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsHeatmapWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsHeatmapWidget.cpp" line="+20"/>
        <source>X axis</source>
        <translation>X 轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Y axis</source>
        <translation>Y 轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Value</source>
        <translation>值</translation>
    </message>
    <message>
        <location line="+76"/>
        <location line="+4"/>
        <source>Please select an X-axis data column before plotting</source>
        <translation>请先选择 X 轴数据列再绘图</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+4"/>
        <source>Please select a Y-axis data column before plotting</source>
        <translation>请先选择 Y 轴数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsHistplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsHistplotWidget.cpp" line="+21"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Hue</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+68"/>
        <location line="+4"/>
        <source>Please select a data column before plotting</source>
        <translation>请先选择数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsKdeplot1dWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsKdeplot1dWidget.cpp" line="+21"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Hue</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+77"/>
        <location line="+4"/>
        <source>Please select a data column before plotting</source>
        <translation>请先选择数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsKdeplot2dWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsKdeplot2dWidget.cpp" line="+21"/>
        <source>X axis</source>
        <translation>X 轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Y axis</source>
        <translation>Y 轴</translation>
    </message>
    <message>
        <location line="+54"/>
        <location line="+4"/>
        <source>Please select an X-axis data column before plotting</source>
        <translation>请先选择 X 轴数据列再绘图</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+4"/>
        <source>Please select a Y-axis data column before plotting</source>
        <translation>请先选择 Y 轴数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsRegplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsRegplotWidget.cpp" line="+20"/>
        <source>X axis</source>
        <translation>X 轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Y axis</source>
        <translation>Y 轴</translation>
    </message>
    <message>
        <location line="+76"/>
        <source>Please select an X-axis data column before plotting</source>
        <translation>请先选择 X 轴数据列再绘图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Please select a Y-axis data column before plotting</source>
        <translation>请先选择 Y 轴数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddStatsScatterplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsScatterplotWidget.cpp" line="+20"/>
        <source>X axis</source>
        <translation>X 轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Y axis</source>
        <translation>Y 轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Hue</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+39"/>
        <location line="+4"/>
        <source>Please select an X-axis data column before plotting</source>
        <translation>请先选择 X 轴数据列再绘图</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+4"/>
        <source>Please select a Y-axis data column before plotting</source>
        <translation>请先选择 Y 轴数据列再绘图</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddVectorFieldWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddVectorFieldWidget.cpp" line="+16"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>U</source>
        <translation>U</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>V</source>
        <translation>V</translation>
    </message>
    <message>
        <location line="+34"/>
        <location line="+10"/>
        <location line="+10"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-19"/>
        <source>X, Y, U, V must be series</source>
        <translation>X、Y、U、V必须是序列</translation>
    </message>
    <message>
        <location line="+10"/>
        <location line="+10"/>
        <source>The selected data cannot be converted to a series</source>
        <translation>所选数据无法转换为序列</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Exception occurred during extracting vector field data:%1</source>
        <translation>提取向量场数据过程中出现异常:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddXYESeriesWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddXYESeriesWidget.cpp" line="+20"/>
        <source>x</source>
        <translation>x</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>y</source>
        <translation>y</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>error</source>
        <translation>误差</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location line="+154"/>
        <location line="+8"/>
        <location line="+23"/>
        <location line="+9"/>
        <location line="+23"/>
        <location line="+26"/>
        <location line="+23"/>
        <location line="+15"/>
        <location line="+7"/>
        <location line="+23"/>
        <location line="+12"/>
        <location line="+7"/>
        <location line="+6"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-181"/>
        <source>The initial value of x auto increment series must be a floating-point arithmetic number</source>
        <translation>x自增序列的初始值必须为浮点数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The step value of x auto increment series must be a floating-point arithmetic number</source>
        <translation>x自增序列的步长必须为浮点数</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>The initial value of y auto increment series must be a floating-point arithmetic number</source>
        <translation>y自增序列的初始值必须为浮点数</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The step value of y auto increment series must be a floating-point arithmetic number</source>
        <translation>y自增序列的步长必须为浮点数</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>x and y cannot be set to autoincrement at the same time</source>
        <translation>x和y无法同时设置为自增</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>y - value/error value must be a series</source>
        <translation>y必须是序列</translation>
    </message>
    <message>
        <location line="+18"/>
        <location line="+45"/>
        <location line="+54"/>
        <source>Exception occurred during extracting from pandas.Series to double vector:%1</source>
        <translation>从pandas.Series提取为double vector过程中出现异常:%1</translation>
    </message>
    <message>
        <location line="-94"/>
        <location line="+45"/>
        <source>Exception occurred during extracting from pandas.Series to double vector</source>
        <translation>从pandas.Series提取为double vector过程中出现异常</translation>
    </message>
    <message>
        <location line="-30"/>
        <location line="+42"/>
        <source>x must be a series</source>
        <translation>x必须是序列</translation>
    </message>
    <message>
        <location line="-35"/>
        <source>The None value cannot be converted to a series</source>
        <translation>None值无法转换为序列</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>y must be a series</source>
        <translation>y必须是序列</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>error must be a series</source>
        <translation>误差必须是序列</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddXYSeriesWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddXYSeriesWidget.cpp" line="+23"/>
        <source>x</source>
        <translation>x</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>y</source>
        <translation>y</translation>
    </message>
    <message>
        <location line="+242"/>
        <location line="+8"/>
        <location line="+23"/>
        <location line="+9"/>
        <location line="+23"/>
        <location line="+13"/>
        <location line="+19"/>
        <location line="+14"/>
        <location line="+19"/>
        <location line="+11"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="-138"/>
        <source>The initial value of x auto increment series must be a floating-point arithmetic number</source>
        <translation>x自增序列的初始值必须为浮点数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The step value of x auto increment series must be a floating-point arithmetic number</source>
        <translation>x自增序列的步长必须为浮点数</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>The initial value of y auto increment series must be a floating-point arithmetic number</source>
        <translation>y自增序列的初始值必须为浮点数</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The step value of y auto increment series must be a floating-point arithmetic number</source>
        <translation>y自增序列的步长必须为浮点数</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>x and y cannot be set to autoincrement at the same time</source>
        <translation>x和y无法同时设置为自增</translation>
    </message>
    <message>
        <location line="+13"/>
        <location line="+33"/>
        <location line="+30"/>
        <source>The None value cannot be converted to a series</source>
        <translation>None值无法转换为序列</translation>
    </message>
    <message>
        <location line="-49"/>
        <location line="+33"/>
        <location line="+35"/>
        <source>Exception occurred during extracting from pandas.Series to double vector:%1</source>
        <translation>从pandas.Series提取为double vector过程中出现异常:%1</translation>
    </message>
    <message>
        <location line="-63"/>
        <location line="+33"/>
        <source>Exception occurred during extracting from pandas.Series to double vector</source>
        <translation>从pandas.Series提取为double vector过程中出现异常</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAddtGridRasterDataWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddtGridRasterDataWidget.cpp" line="+178"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The data dimensions are incorrect. The length of x should be equal to the number of columns in value, and the length of y should be equal to the number of rows in value.</source>
        <translation>数据维度不正确，要求x长度和value的列数相等，y的长度和value的行数相等</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Failed to set data: %1</source>
        <translation>设置数据失败:%1</translation>
    </message>
</context>
<context>
    <name>DA::DAChartArrowMarkerSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartArrowMarkerSettingPanel.cpp" line="+51"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Position Mode</source>
        <translation>定位模式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Explicit Points</source>
        <translation>显式起止点</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Start Length Angle</source>
        <translation>起点长度角度</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Start X</source>
        <translation>起点X</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Start Y</source>
        <translation>起点Y</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>End X</source>
        <translation>终点X</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>End Y</source>
        <translation>终点Y</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Length</source>
        <translation>长度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Angle</source>
        <translation>角度</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Line</source>
        <translation>线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Line Pen</source>
        <translation>线条画笔</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Head</source>
        <translation>头部</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Head Style</source>
        <translation>头部样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+30"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="-29"/>
        <location line="+30"/>
        <source>Arrow Head</source>
        <translation>箭头</translation>
    </message>
    <message>
        <location line="-29"/>
        <location line="+30"/>
        <source>Circle</source>
        <translation>圆形</translation>
    </message>
    <message>
        <location line="-29"/>
        <location line="+30"/>
        <source>Square</source>
        <translation>方形</translation>
    </message>
    <message>
        <location line="-29"/>
        <location line="+30"/>
        <source>Diamond</source>
        <translation>菱形</translation>
    </message>
    <message>
        <location line="-29"/>
        <location line="+30"/>
        <source>Triangle</source>
        <translation>三角形</translation>
    </message>
    <message>
        <location line="-29"/>
        <location line="+30"/>
        <source>Custom</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="-21"/>
        <source>Head Size</source>
        <translation>头部尺寸</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Head Pen</source>
        <translation>头部画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Head Brush</source>
        <translation>头部画刷</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Tail</source>
        <translation>尾部</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tail Style</source>
        <translation>尾部样式</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Tail Size</source>
        <translation>尾部尺寸</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tail Pen</source>
        <translation>尾部画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tail Brush</source>
        <translation>尾部画刷</translation>
    </message>
</context>
<context>
    <name>DA::DAChartAxisSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartAxisSettingPanel.cpp" line="+191"/>
        <source>Enable</source>
        <translation>启用</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Axis</source>
        <translation>启用坐标轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Label</source>
        <translation>标签</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Text</source>
        <translation>标签文本</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Font</source>
        <translation>标签字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Font Color</source>
        <translation>标签字体颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Alignment</source>
        <translation>标签对齐</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Rotation</source>
        <translation>标签旋转</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Scale</source>
        <translation>刻度</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Margin</source>
        <translation>边距</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Min Scale</source>
        <translation>最小刻度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Scale</source>
        <translation>最大刻度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ticks Inside</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Normal</source>
        <translation>普通</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>DateTime</source>
        <translation>日期时间</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Scale Style</source>
        <translation>刻度样式</translation>
    </message>
</context>
<context>
    <name>DA::DAChartBarSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartBarSettingPanel.cpp" line="+51"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Axis</source>
        <translation>坐标轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Legend</source>
        <translation>图例</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Legend Mode</source>
        <translation>图例模式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Chart Mode</source>
        <translation>图表模式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bar Mode</source>
        <translation>柱状模式</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Fill</source>
        <translation>底色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Fill</source>
        <translation>启用填充</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fill Brush</source>
        <translation>填充画刷</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Edge</source>
        <translation>边框</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Edge</source>
        <translation>启用边框</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Edge Pen</source>
        <translation>边框画笔</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+2"/>
        <source>Baseline</source>
        <translation>基线</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Layout</source>
        <translation>布局</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Layout Policy</source>
        <translation>布局策略</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Auto Adjust Samples</source>
        <translation>自动调整采样</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scale Samples To Axes</source>
        <translation>采样缩放至坐标轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scale Sample To Canvas</source>
        <translation>采样缩放至画布</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Fixed Sample Size</source>
        <translation>固定采样尺寸</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Layout Hint</source>
        <translation>布局提示</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Spacing</source>
        <translation>间距</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Margin</source>
        <translation>边距</translation>
    </message>
</context>
<context>
    <name>DA::DAChartBoxChartSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartBoxChartSettingPanel.cpp" line="+51"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Box</source>
        <translation>盒形</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Box Style</source>
        <translation>箱体样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No Box</source>
        <translation>无箱体</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rectangle</source>
        <translation>矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Diamond</source>
        <translation>菱形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Notched</source>
        <translation>带凹槽</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Box Extent</source>
        <translation>箱体宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Min Box Width</source>
        <translation>最小宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Box Width</source>
        <translation>最大宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Pen</source>
        <translation>轮廓画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Brush</source>
        <translation>填充画刷</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Whisker</source>
        <translation>须线</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Whisker Style</source>
        <translation>须线样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No Whiskers</source>
        <translation>无须线</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Standard (T-bar)</source>
        <translation>标准(T形)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Min-Max Line</source>
        <translation>极值线</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Median</source>
        <translation>中位数线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Median Visible</source>
        <translation>中位数线可见</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Median Pen</source>
        <translation>中位数线画笔</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Mean</source>
        <translation>均值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Mean Visible</source>
        <translation>均值标记可见</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Outliers</source>
        <translation>离群点</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Outlier Jitter</source>
        <translation>离群点抖动</translation>
    </message>
</context>
<context>
    <name>DA::DAChartCanvasSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartCanvasSettingPanel.cpp" line="+130"/>
        <source>Background</source>
        <translation>背景</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Background Brush</source>
        <translation>背景画刷</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Border</source>
        <translation>边框</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Border Width</source>
        <translation>边框宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Border Pen</source>
        <translation>边框画笔</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No Frame</source>
        <translation>无边框</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Box</source>
        <translation>方框</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Panel</source>
        <translation>面板</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Styled Panel</source>
        <translation>样式面板</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Win Panel</source>
        <translation>窗口面板</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Frame Shape</source>
        <translation>边框形状</translation>
    </message>
</context>
<context>
    <name>DA::DAChartCurveSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartCurveSettingPanel.cpp" line="+45"/>
        <source>General</source>
        <translation>通用</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Curve Style</source>
        <translation>曲线样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2"/>
        <source>Pen</source>
        <translation>画笔</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Marker</source>
        <translation>标记</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Marker</source>
        <translation>启用标记</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Symbol</source>
        <translation>符号</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Attributes</source>
        <translation>属性</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fitted</source>
        <translation>拟合</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Inverted</source>
        <translation>反转</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Legend</source>
        <translation>图例</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show Line</source>
        <translation>显示线条</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show Symbol</source>
        <translation>显示符号</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Show Brush</source>
        <translation>显示画刷</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Fill</source>
        <translation>底色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Fill</source>
        <translation>启用填充</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fill Brush</source>
        <translation>填充画刷</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2"/>
        <source>Baseline</source>
        <translation>基线</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
</context>
<context>
    <name>DA::DAChartDataPickerSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartDataPickerSettingPanel.cpp" line="+115"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Picker Mode</source>
        <translation>拾取模式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Off</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y Value</source>
        <translation>Y值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>XY Value</source>
        <translation>XY值拾取</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Show X Value</source>
        <translation>显示X值</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Text Placement</source>
        <translation>文字位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Auto</source>
        <translation>自动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Follow Top</source>
        <translation>跟随顶部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Follow Bottom</source>
        <translation>跟随底部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Follow Mouse</source>
        <translation>跟随鼠标</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Canvas Top Right</source>
        <translation>画布右上</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Canvas Top Left</source>
        <translation>画布左上</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Canvas Bottom Right</source>
        <translation>画布右下</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Canvas Bottom Left</source>
        <translation>画布左下</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Canvas Top Auto</source>
        <translation>画布顶部自动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Canvas Bottom Auto</source>
        <translation>画布底部自动</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Interpolation</source>
        <translation>插值模式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Linear</source>
        <translation>线性</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Feature Point</source>
        <translation>特征点</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Draw Feature Point</source>
        <translation>绘制特征点</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Feature Point Size</source>
        <translation>特征点大小</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Nearest Search Window</source>
        <translation>搜索窗口大小</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Text Style</source>
        <translation>文字样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Background Brush</source>
        <translation>背景画刷</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Text Alignment</source>
        <translation>文字对齐</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Text Offset X</source>
        <translation>文字偏移X</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Text Offset Y</source>
        <translation>文字偏移Y</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Linkage</source>
        <translation>联动</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Picker Group Enabled</source>
        <translation>拾取器联动</translation>
    </message>
</context>
<context>
    <name>DA::DAChartDataProbeMarkerSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartDataProbeMarkerSettingPanel.cpp" line="+48"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Probe Value</source>
        <translation>探针值</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Label</source>
        <translation>标签</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Visible</source>
        <translation>标签可见</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Label Position</source>
        <translation>标签位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Top / Left</source>
        <translation>上/左</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bottom / Right</source>
        <translation>下/右</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Label Style</source>
        <translation>标签样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Plain Text</source>
        <translation>纯文本</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rounded Rect</source>
        <translation>圆角矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rectangle</source>
        <translation>矩形</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ellipse</source>
        <translation>椭圆</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Probe Color</source>
        <translation>探针颜色</translation>
    </message>
</context>
<context>
    <name>DA::DAChartErrorBarSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartErrorBarSettingPanel.cpp" line="+49"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Axis</source>
        <translation>坐标轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Error Bar</source>
        <translation>误差棒</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Error Bar</source>
        <translation>启用误差棒</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Error Bar Style</source>
        <translation>误差棒样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bar</source>
        <translation>柱状</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Box</source>
        <translation>方框</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Error Bar Pen</source>
        <translation>误差棒画笔</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Fill</source>
        <translation>底色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enable Fill</source>
        <translation>启用填充</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fill Brush</source>
        <translation>填充画刷</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Curve</source>
        <translation>曲线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Curve Pen</source>
        <translation>曲线画笔</translation>
    </message>
</context>
<context>
    <name>DA::DAChartGraphicSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartGraphicSettingPanel.cpp" line="+42"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Render</source>
        <translation>渲染</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Antialiased</source>
        <translation>抗锯齿</translation>
    </message>
</context>
<context>
    <name>DA::DAChartGridSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartGridSettingPanel.cpp" line="+44"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Axis</source>
        <translation>坐标轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Line Style</source>
        <translation>线条样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Major Pen</source>
        <translation>主笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Minor Pen</source>
        <translation>次笔</translation>
    </message>
</context>
<context>
    <name>DA::DAChartHistogramSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartHistogramSettingPanel.cpp" line="+46"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Histogram Style</source>
        <translation>直方图样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Outline</source>
        <translation>轮廓</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Columns</source>
        <translation>柱状</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Lines</source>
        <translation>线条</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Pen</source>
        <translation>轮廓画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Brush</source>
        <translation>填充画刷</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Baseline</source>
        <translation>基线</translation>
    </message>
</context>
<context>
    <name>DA::DAChartItemSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartItemSettingPanel.cpp" line="+74"/>
        <source>Lines</source>
        <translation>线条</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Sticks</source>
        <translation>棒状</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Steps</source>
        <translation>阶梯</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Dots</source>
        <translation>点</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No Curve</source>
        <translation>无曲线</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Horizontal</source>
        <translation>水平</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Vertical</source>
        <translation>垂直</translation>
    </message>
    <message>
        <location line="+47"/>
        <source>Y Left</source>
        <translation>Y左轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Y Right</source>
        <translation>Y右轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>X Bottom</source>
        <translation>X底轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Top</source>
        <translation>X顶轴</translation>
    </message>
    <message>
        <location line="+58"/>
        <source>Normal</source>
        <translation>普通</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>DateTime</source>
        <translation>日期时间</translation>
    </message>
</context>
<context>
    <name>DA::DAChartItemTableModel</name>
    <message>
        <location filename="../DAFigure/Models/DAChartItemTableModel.cpp" line="+593"/>
        <location line="+29"/>
        <location line="+30"/>
        <location line="+11"/>
        <source>x</source>
        <translation>x</translation>
    </message>
    <message>
        <location line="-68"/>
        <location line="+29"/>
        <location line="+30"/>
        <location line="+11"/>
        <source>y</source>
        <translation>y</translation>
    </message>
    <message>
        <location line="-62"/>
        <location line="+11"/>
        <location line="+63"/>
        <location line="+27"/>
        <source>value</source>
        <translation>值</translation>
    </message>
    <message>
        <location line="-99"/>
        <location line="+74"/>
        <source>min</source>
        <translation>最小值</translation>
    </message>
    <message>
        <location line="-72"/>
        <location line="+74"/>
        <source>max</source>
        <translation>最大值</translation>
    </message>
    <message>
        <location line="-65"/>
        <location line="+90"/>
        <source>set %1</source>
        <translation>集合%1</translation>
    </message>
    <message>
        <location line="-80"/>
        <location line="+41"/>
        <source>z</source>
        <translation>z</translation>
    </message>
    <message>
        <location line="-33"/>
        <location line="+56"/>
        <source>time</source>
        <translation>时间</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+56"/>
        <source>open</source>
        <translation>开盘</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+56"/>
        <source>high</source>
        <translation>最高</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+56"/>
        <source>low</source>
        <translation>最低</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+56"/>
        <source>close</source>
        <translation>收盘</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>whisker-lower</source>
        <translation>下须</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Q1</source>
        <translation>下四分位</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>median</source>
        <translation>中位数</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Q3</source>
        <translation>上四分位</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>whisker-upper</source>
        <translation>上须</translation>
    </message>
</context>
<context>
    <name>DA::DAChartLegendSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartLegendSettingPanel.cpp" line="+52"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Alignment</source>
        <translation>对齐</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Horizontal Offset</source>
        <translation>水平偏移</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Vertical Offset</source>
        <translation>垂直偏移</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+5"/>
        <source>Spacing</source>
        <translation>间距</translation>
    </message>
    <message>
        <location line="-3"/>
        <source>Margin</source>
        <translation>边距</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Item Margin</source>
        <translation>项边距</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Item Spacing</source>
        <translation>项间距</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Columns</source>
        <translation>最大列数</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Border Radius</source>
        <translation>边框圆角</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Border Pen</source>
        <translation>边框画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Font</source>
        <translation>字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Font Color</source>
        <translation>字体颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Background Brush</source>
        <translation>背景画刷</translation>
    </message>
</context>
<context>
    <name>DA::DAChartManageWidget</name>
    <message>
        <location filename="../DAGui/Chart/DAChartManageWidget.cpp" line="+92"/>
        <location line="+543"/>
        <location line="+17"/>
        <location line="+20"/>
        <location line="+22"/>
        <location line="+19"/>
        <location line="+17"/>
        <source>Rename</source>
        <translation>重命名</translation>
    </message>
    <message>
        <location line="-637"/>
        <source>Visible</source>
        <translation>可见</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+736"/>
        <location line="+24"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="-758"/>
        <source>Setting</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+327"/>
        <source>received figure create signal, but cannot find figure</source>
        <translation>获取了绘图创建的信号，但无法找到绘图</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>received figure close signal, but cannot find figure index</source>
        <translation>获取了绘图关闭的信号，但无法找到绘图的索引</translation>
    </message>
    <message>
        <location line="+182"/>
        <source>chart</source>
        <translation>绘图</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+17"/>
        <location line="+20"/>
        <location line="+22"/>
        <location line="+19"/>
        <location line="+17"/>
        <source>New name:</source>
        <translation>新名称:</translation>
    </message>
    <message>
        <location line="-41"/>
        <source>3D Chart</source>
        <translation>3D绘图</translation>
    </message>
    <message>
        <location line="+142"/>
        <location line="+24"/>
        <source>Are you sure to delete &quot;%1&quot;?</source>
        <translation>确认删除&quot;%1&quot;吗?</translation>
    </message>
</context>
<context>
    <name>DA::DAChartMarkerSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartMarkerSettingPanel.cpp" line="+48"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Value</source>
        <translation>X值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Value</source>
        <translation>Y值</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Line</source>
        <translation>线</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Line Style</source>
        <translation>线样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No Line</source>
        <translation>无线</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+23"/>
        <source>Horizontal</source>
        <translation>水平</translation>
    </message>
    <message>
        <location line="-22"/>
        <location line="+23"/>
        <source>Vertical</source>
        <translation>垂直</translation>
    </message>
    <message>
        <location line="-22"/>
        <source>Cross</source>
        <translation>十字标记</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Line Pen</source>
        <translation>线条画笔</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2"/>
        <source>Label</source>
        <translation>标签</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Label Alignment</source>
        <translation>标签对齐</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Label Orientation</source>
        <translation>标签方向</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Spacing</source>
        <translation>间距</translation>
    </message>
</context>
<context>
    <name>DA::DAChartMultiBarSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartMultiBarSettingPanel.cpp" line="+47"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Chart Style</source>
        <translation>图表样式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Grouped</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Stacked</source>
        <translation>堆叠</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Baseline</source>
        <translation>基线</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Layout</source>
        <translation>布局</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Layout Policy</source>
        <translation>布局策略</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Auto Adjust</source>
        <translation>自动调整</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scale To Axes</source>
        <translation>缩放到坐标轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scale To Canvas</source>
        <translation>缩放到画布</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Fixed Sample Size</source>
        <translation>固定采样尺寸</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Layout Hint</source>
        <translation>布局提示</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Spacing</source>
        <translation>间距</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Margin</source>
        <translation>边距</translation>
    </message>
</context>
<context>
    <name>DA::DAChartOperateWidget</name>
    <message>
        <location filename="../DAGui/Chart/DAChartOperateWidget.cpp" line="+166"/>
        <source>figure-%1</source>
        <translation>图-%1</translation>
    </message>
    <message>
        <location line="+362"/>
        <source>Question</source>
        <translation>疑问</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Whether to close the figure widget</source>
        <translation>是否关闭绘图窗口</translation>
    </message>
</context>
<context>
    <name>DA::DAChartPlotSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartPlotSettingPanel.cpp" line="+114"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title Text</source>
        <translation>标题文本</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title Font</source>
        <translation>标题字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title Color</source>
        <translation>标题颜色</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Footer</source>
        <translation>脚注</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Footer Text</source>
        <translation>脚注文本</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Footer Font</source>
        <translation>脚注字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Footer Color</source>
        <translation>脚注颜色</translation>
    </message>
</context>
<context>
    <name>DA::DAChartScaleSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartScaleSettingPanel.cpp" line="+47"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Scale</source>
        <translation>刻度</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Alignment</source>
        <translation>对齐</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bottom</source>
        <translation>底部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Top</source>
        <translation>顶部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Left</source>
        <translation>左侧</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Right</source>
        <translation>右侧</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Border Distance</source>
        <translation>边框距离</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Sync Scale From Axis</source>
        <translation>同步坐标轴刻度</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+2"/>
        <source>Font</source>
        <translation>字体</translation>
    </message>
</context>
<context>
    <name>DA::DAChartSeriesPickerWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartSeriesPickerWidget.cpp" line="+253"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cannot resolve expression &apos;%1&apos;, expected format: data[&apos;column&apos;]</source>
        <translation>无法解析表达式&apos;%1&apos;，期望格式: data[&apos;列名&apos;]</translation>
    </message>
    <message>
        <location line="+83"/>
        <source>Select Series</source>
        <translation>选择序列</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Return to add chart</source>
        <translation>回到添加绘图</translation>
    </message>
    <message>
        <location line="-199"/>
        <source>X:</source>
        <translation>X:</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Y:</source>
        <translation>Y:</translation>
    </message>
</context>
<context>
    <name>DA::DAChartSettingWidget</name>
    <message>
        <location filename="../DAGui/Chart/DAChartSettingWidget.cpp" line="+202"/>
        <source>Chart Area</source>
        <translation>图表区</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Canvas Area</source>
        <translation>绘图区</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y Left Scale</source>
        <translation>左Y轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>X Bottom Scale</source>
        <translation>下X轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Y Right Scale</source>
        <translation>右Y轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>X Top Scale</source>
        <translation>上X轴</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data Picker</source>
        <translation>数据拾取</translation>
    </message>
</context>
<context>
    <name>DA::DAChartShapeSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartShapeSettingPanel.cpp" line="+46"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pen</source>
        <translation>轮廓画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Brush</source>
        <translation>填充画刷</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Legend Mode</source>
        <translation>图例模式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Shape</source>
        <translation>形状</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Render</source>
        <translation>渲染</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Render Tolerance</source>
        <translation>渲染容差</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Clip Polygons</source>
        <translation>裁剪多边形</translation>
    </message>
</context>
<context>
    <name>DA::DAChartSpectroCurveSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartSpectroCurveSettingPanel.cpp" line="+46"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Drawing</source>
        <translation>绘制</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pen Width</source>
        <translation>笔宽</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Clip Points</source>
        <translation>裁剪点</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Color Map</source>
        <translation>颜色映射</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Color Range Min</source>
        <translation>颜色范围最小值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Color Range Max</source>
        <translation>颜色范围最大值</translation>
    </message>
</context>
<context>
    <name>DA::DAChartSpectrogramSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartSpectrogramSettingPanel.cpp" line="+46"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Axis</source>
        <translation>坐标轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Display</source>
        <translation>视图显示</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Display Mode</source>
        <translation>显示模式</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Image Mode</source>
        <translation>图像模式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Contour Mode</source>
        <translation>等值线模式</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>From Color</source>
        <translation>起始颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>To Color</source>
        <translation>终止颜色</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Contour</source>
        <translation>等值线</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Contour Pen</source>
        <translation>等值线画笔</translation>
    </message>
</context>
<context>
    <name>DA::DAChartSymbolComboBox</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartSymbolComboBox.cpp" line="+38"/>
        <source>No Symbol</source>
        <translation>无符号</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Ellipse</source>
        <translation>椭圆</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Rectangle</source>
        <translation>矩形</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Diamond</source>
        <translation>菱形</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Triangle</source>
        <translation>三角形</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Down Triangle</source>
        <translation>下三角形</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Up Triangle</source>
        <translation>上三角形</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Left Triangle</source>
        <translation>左三角形</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Right Triangle</source>
        <translation>右三角形</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Cross</source>
        <translation>十字标记</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Diagonal Cross</source>
        <translation>斜十字</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Horizontal Line</source>
        <translation>水平线</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Vertical Line</source>
        <translation>垂直线</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Star 1</source>
        <translation>星形1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Star 2</source>
        <translation>星形2</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Hexagon</source>
        <translation>六边形</translation>
    </message>
</context>
<context>
    <name>DA::DAChartTextLabelSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartTextLabelSettingPanel.cpp" line="+47"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Text</source>
        <translation>文本</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Text Content</source>
        <translation>文本内容</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Font</source>
        <translation>字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Text Color</source>
        <translation>文字颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Alignment</source>
        <translation>对齐</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Margin</source>
        <translation>边距</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Background</source>
        <translation>背景</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Border Radius</source>
        <translation>边框圆角</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Background Brush</source>
        <translation>背景画刷</translation>
    </message>
</context>
<context>
    <name>DA::DAChartTextMarkerSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartTextMarkerSettingPanel.cpp" line="+54"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z 值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X 轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y 轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Anchor X</source>
        <translation>锚点 X</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Anchor Y</source>
        <translation>锚点 Y</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Text</source>
        <translation>文本</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Edit Rich Text...</source>
        <translation>编辑富文本...</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Content</source>
        <translation>内容</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Font</source>
        <translation>字体</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Text Color</source>
        <translation>文字颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Alignment</source>
        <translation>对齐</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Background</source>
        <translation>背景</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Border Radius</source>
        <translation>圆角</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Background Brush</source>
        <translation>背景画刷</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Layout</source>
        <translation>布局</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Spacing</source>
        <translation>间距</translation>
    </message>
</context>
<context>
    <name>DA::DAChartTradingCurveSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartTradingCurveSettingPanel.cpp" line="+48"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Axis</source>
        <translation>坐标轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Symbol</source>
        <translation>符号</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Symbol Attribute</source>
        <translation>符号属性</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bar</source>
        <translation>柱状</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Candlestick</source>
        <translation>K线</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Increasing Brush</source>
        <translation>上涨画刷</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Decreasing Brush</source>
        <translation>下跌画刷</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Direction</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Symbol Extent</source>
        <translation>符号范围</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Min Symbol Width</source>
        <translation>最小符号宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Symbol Width</source>
        <translation>最大符号宽度</translation>
    </message>
</context>
<context>
    <name>DA::DAChartVectorFieldSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartVectorFieldSettingPanel.cpp" line="+49"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pen</source>
        <translation>轮廓画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Brush</source>
        <translation>填充画刷</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Indicator Origin</source>
        <translation>箭头原点</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Head</source>
        <translation>头部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Tail</source>
        <translation>尾部</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Center</source>
        <translation>居中</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Magnitude</source>
        <translation>幅值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Magnitude As Color</source>
        <translation>幅值映射颜色</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Magnitude As Length</source>
        <translation>幅值映射长度</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Min Arrow Length</source>
        <translation>最小箭头长度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Arrow Length</source>
        <translation>最大箭头长度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Magnitude Scale Factor</source>
        <translation>幅值缩放因子</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Filter</source>
        <translation>过滤</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Filter Vectors</source>
        <translation>过滤矢量</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Raster Width</source>
        <translation>栅格宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Raster Height</source>
        <translation>栅格高度</translation>
    </message>
</context>
<context>
    <name>DA::DAChartZoneSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartZoneSettingPanel.cpp" line="+45"/>
        <source>Basic</source>
        <translation>基础</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Z Value</source>
        <translation>Z值</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Axis</source>
        <translation>X轴</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Zone</source>
        <translation>区间</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Interval Min</source>
        <translation>区间最小值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Interval Max</source>
        <translation>区间最大值</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pen</source>
        <translation>轮廓画笔</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Brush</source>
        <translation>填充画刷</translation>
    </message>
</context>
<context>
    <name>DA::DACoreInterface</name>
    <message>
        <location filename="../DAInterface/DACoreInterface.cpp" line="+59"/>
        <source>Python interpreter is not initialized</source>
        <translation>Python 解释器未初始化</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Python scripts path is %1</source>
        <translation>Python 脚本路径为 %1</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Failed to initialize scripts</source>
        <translation>脚本初始化失败</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Failed to initialize script runner, script execution will be unavailable</source>
        <translation>脚本执行引擎初始化失败，脚本执行功能将不可用</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Failed to initialize Python environment: %1</source>
        <translation>初始化 Python 环境失败：%1</translation>
    </message>
</context>
<context>
    <name>DA::DADataManageTableView</name>
    <message>
        <location filename="../DAGui/DADataManageTableView.cpp" line="+89"/>
        <source>An item is selected in the data management table, but the corresponding data cannot be obtained</source>
        <translation>在数据管理表中选中了条目，但无法获取对应数据</translation>
    </message>
</context>
<context>
    <name>DA::DADataManageWidget</name>
    <message>
        <location filename="../DAGui/DADataManageWidget.cpp" line="+83"/>
        <source>Please select the data item to remove</source>
        <translation>请选择需要删除的数据条目</translation>
    </message>
</context>
<context>
    <name>DA::DADataManager</name>
    <message>
        <location filename="../DAData/DADataManager.cpp" line="+68"/>
        <source>data &apos;%1&apos; has been added</source>
        <translation>数据 &apos;%1&apos; 已被添加过</translation>
    </message>
    <message>
        <location line="+58"/>
        <source>add datas</source>
        <translation>批量添加数据</translation>
    </message>
    <message>
        <location line="+66"/>
        <source>remove datas</source>
        <translation>批量移除数据</translation>
    </message>
</context>
<context>
    <name>DA::DADataManagerTableModel</name>
    <message>
        <location filename="../DAGui/Models/DADataManagerTableModel.cpp" line="+33"/>
        <source>name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>type</source>
        <translation>类型</translation>
    </message>
</context>
<context>
    <name>DA::DADataManagerTreeModel</name>
    <message>
        <location filename="../DAGui/Models/DADataManagerTreeModel.cpp" line="+289"/>
        <location line="+2"/>
        <location line="+87"/>
        <location line="+3"/>
        <location line="+97"/>
        <location line="+2"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="-189"/>
        <location line="+90"/>
        <location line="+99"/>
        <source>Properties</source>
        <translation>属性</translation>
    </message>
    <message>
        <location line="+117"/>
        <source>The dataset name cannot be empty</source>
        <translation>数据集名称不能为空</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>The dataset name &quot;%1&quot; already exists</source>
        <translation>数据集名称&quot;%1&quot;已存在</translation>
    </message>
</context>
<context>
    <name>DA::DADataManagerTreeWidget</name>
    <message>
        <location filename="../DAGui/DADataManagerTreeWidget.cpp" line="+438"/>
        <source>Search...</source>
        <translation>搜索</translation>
    </message>
</context>
<context>
    <name>DA::DADataOperateOfDataFrameWidget</name>
    <message>
        <location filename="../DAGui/DADataOperateOfDataFrameWidget.cpp" line="+241"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The name of the new column to be inserted must be specified</source>
        <translation>必须指定列的名字</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>Please select valid data cells</source>
        <translation>请选择正确的行</translation>
    </message>
    <message>
        <location line="+42"/>
        <location line="+298"/>
        <location line="+35"/>
        <location line="+41"/>
        <location line="+42"/>
        <location line="+765"/>
        <location line="+24"/>
        <source>Please select a valid column</source>
        <translation>请选择正确的列</translation>
    </message>
    <message>
        <location line="-1163"/>
        <location line="+828"/>
        <location line="+61"/>
        <location line="+48"/>
        <source>Please select a valid cell</source>
        <translation>请选择正确的单元格</translation>
    </message>
    <message>
        <location line="-889"/>
        <source>Clipboard is empty</source>
        <translation>剪贴板为空</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Please select a cell to paste into</source>
        <translation>请先选中要粘贴的起始单元格</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Nothing to paste: the clipboard content exceeds the table boundary</source>
        <translation>无可粘贴内容：剪贴板内容超出表格边界</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Paste partially dropped: %1 row(s) and %2 cell(s) outside the table were ignored</source>
        <translation>粘贴部分丢弃：%1 行与 %2 个单元格超出表格范围被忽略</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Paste failed: the content does not match the column data type</source>
        <translation>粘贴失败：内容与列数据类型不匹配</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>No cells selected to cut</source>
        <translation>没有选中可剪切的单元格</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Table has no columns</source>
        <translation>表格没有列</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Column name cannot be empty</source>
        <translation>列名不能为空</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Column name &quot;%1&quot; already exists, please use another name</source>
        <translation>列名&quot;%1&quot;已存在，请使用其他名称</translation>
    </message>
    <message>
        <location line="+203"/>
        <location line="+4"/>
        <source>Unable to get statistics for this column</source>
        <translation>无法获取此列的统计信息</translation>
    </message>
</context>
<context>
    <name>DA::DADataOperateWidget</name>
    <message>
        <location filename="../DAGui/DADataOperateWidget.cpp" line="+362"/>
        <source>removing a widget that does not exist in the dock</source>
        <translation>正在移除一个不存在的窗口</translation>
    </message>
    <message>
        <location line="+79"/>
        <source>[deleted]</source>
        <translation>[已删除]</translation>
    </message>
    <message>
        <location line="+81"/>
        <source>Question</source>
        <translation>询问</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Whether to close the data table widget</source>
        <translation>是否关闭数据表窗口</translation>
    </message>
</context>
<context>
    <name>DA::DADataTableView</name>
    <message>
        <location filename="../DAGui/DADataTableView.cpp" line="+29"/>
        <source>DADataTableView requires a model to be set first</source>
        <translation>你需要先设置模型</translation>
    </message>
</context>
<context>
    <name>DA::DADataframeToVectorPointWidget</name>
    <message>
        <location filename="../DAGui/DADataframeToVectorPointWidget.cpp" line="+15"/>
        <source>x</source>
        <translation>x</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>y</source>
        <translation>y</translation>
    </message>
    <message>
        <location line="+60"/>
        <source>Exception occurred during extraction from pandas.Series to double vector: %1</source>
        <translation>从pandas.Series提取为double vector过程中出现异常:%1</translation>
    </message>
</context>
<context>
    <name>DA::DADialogAgentSessionManager</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogAgentSessionManager.cpp" line="+48"/>
        <source>Session Manager</source>
        <translation>会话管理</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Double-click a session to switch:</source>
        <translation>双击切换会话：</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Title</source>
        <translation>标题</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Messages</source>
        <translation>消息数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Updated</source>
        <translation>更新时间</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Switch</source>
        <translation>切换</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Rename</source>
        <translation>重命名</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+40"/>
        <location line="+135"/>
        <source>(untitled)</source>
        <translation>（未命名）</translation>
    </message>
    <message>
        <location line="-9"/>
        <source>Rename Session</source>
        <translation>重命名会话</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>New title:</source>
        <translation>新标题：</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Delete Session</source>
        <translation>删除会话</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Delete session &quot;%1&quot;? This cannot be undone.</source>
        <translation>删除会话「%1」？此操作不可撤销。</translation>
    </message>
</context>
<context>
    <name>DA::DADialogChartGuide</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogChartGuide.cpp" line="+115"/>
        <source>curve</source>
        <translation>曲线</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>scatter</source>
        <translation>散点</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>bar</source>
        <translation>柱状</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>error bar</source>
        <translation>误差棒</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>box</source>
        <translation>箱体</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>cloud map</source>
        <translation>云图</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>multi bar</source>
        <translation>多重柱状</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>histogram</source>
        <translation>直方图</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>contour</source>
        <translation>等高线</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>vector field</source>
        <translation>向量场</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>surface 3D</source>
        <translation>3D曲面</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>bar 3D</source>
        <translation>3D柱状</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>line 3D</source>
        <translation>3D线图</translation>
    </message>
</context>
<context>
    <name>DA::DADialogDataframeColumnDescribe</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogDataframeColumnDescribe.cpp" line="+29"/>
        <source>Column: %1</source>
        <translation>列：%1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Type: %1</source>
        <translation>类型：%1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Statistic</source>
        <translation>统计量</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Value</source>
        <translation>值</translation>
    </message>
</context>
<context>
    <name>DA::DADialogStatsChartGuide</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogStatsChartGuide.cpp" line="+109"/>
        <source>Histplot</source>
        <translation>直方图</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>KDE 1D</source>
        <translation>一维核密度</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>KDE 2D</source>
        <translation>二维核密度</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Boxplot</source>
        <translation>箱线图</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Heatmap</source>
        <translation>热力图</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Scatter</source>
        <translation>散点图</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Barplot</source>
        <translation>柱状图</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Regplot</source>
        <translation>回归图</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>ECDF</source>
        <translation>经验累积分布</translation>
    </message>
</context>
<context>
    <name>DA::DADialogTableDisplayFormat</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogTableDisplayFormat.cpp" line="+101"/>
        <source>Format Cells</source>
        <translation>设置单元格格式</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Preview</source>
        <translation>预览</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Category</source>
        <translation>类别</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Options</source>
        <translation>选项</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>No options (default display)</source>
        <translation>无选项（默认显示）</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Decimal places:</source>
        <translation>小数位数:</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>(custom)</source>
        <translation>(自定义)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Preset:</source>
        <translation>预设:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pattern:</source>
        <translation>格式串:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Seconds since epoch</source>
        <translation>epoch 秒</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Milliseconds since epoch</source>
        <translation>epoch 毫秒</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Display as plain text</source>
        <translation>按纯文本显示</translation>
    </message>
</context>
<context>
    <name>DA::DAExportToPngSettingDialog</name>
    <message>
        <location filename="../APP/Dialog/DAExportToPngSettingDialog.cpp" line="+60"/>
        <source>Images</source>
        <translation>图片</translation>
    </message>
</context>
<context>
    <name>DA::DAFigureDockWidgetTab</name>
    <message>
        <location filename="../DAGui/Chart/DAFigureDockWidgetTab.cpp" line="+44"/>
        <location line="+8"/>
        <source>Rename Figure</source>
        <translation>重命名绘图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Figure name:</source>
        <translation>绘图名称：</translation>
    </message>
</context>
<context>
    <name>DA::DAFigureTreeModel</name>
    <message>
        <location filename="../DAFigure/Models/DAFigureTreeModel.cpp" line="+184"/>
        <source>element</source>
        <translation>绘图元素</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>visible</source>
        <translation>可见性</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>property</source>
        <translation>属性</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>chart</source>
        <translation>绘图</translation>
    </message>
    <message>
        <location line="+63"/>
        <location line="+730"/>
        <source>Axis</source>
        <translation>坐标轴</translation>
    </message>
    <message>
        <location line="-705"/>
        <location line="+745"/>
        <source>plot item</source>
        <translation>图元</translation>
    </message>
    <message>
        <location line="-97"/>
        <source>3D Chart</source>
        <translation>3D绘图</translation>
    </message>
    <message>
        <location line="+75"/>
        <source>Axis %1</source>
        <translation>%1轴</translation>
    </message>
    <message>
        <location line="+211"/>
        <source>3D Item</source>
        <translation>3D图元</translation>
    </message>
</context>
<context>
    <name>DA::DAFigureWidget</name>
    <message>
        <location filename="../DAFigure/DAFigureWidget.cpp" line="+148"/>
        <source>Unexpected plotting operation: a chart that does not belong to the DAChartWidget type was added to the figure</source>
        <translation>意外的绘图操作：不属于 DAChartWidget 类型的图表被添加到了 figure 中</translation>
    </message>
    <message>
        <location line="+1188"/>
        <source>Unsupported chart editor type: %1</source>
        <translation>不支持的图表编辑器类型：%1</translation>
    </message>
</context>
<context>
    <name>DA::DAFigureWidgetSettingPanel</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAFigureWidgetSettingPanel.cpp" line="+123"/>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Min Width</source>
        <translation>最小宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Min Height</source>
        <translation>最小高度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Width</source>
        <translation>最大宽度</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Max Height</source>
        <translation>最大高度</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Background</source>
        <translation>背景</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Background Brush</source>
        <translation>背景画刷</translation>
    </message>
</context>
<context>
    <name>DA::DAGraphicsTextItem</name>
    <message>
        <location filename="../DAGraphicsView/DAGraphicsTextItem.cpp" line="+48"/>
        <location line="+11"/>
        <source>Text</source>
        <translation>文本</translation>
    </message>
</context>
<context>
    <name>DA::DALayoutManagerDialog</name>
    <message>
        <location filename="../APP/Dialog/DALayoutManagerDialog.cpp" line="+24"/>
        <source>Layout Manager</source>
        <translation>布局管理</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Double-click a scheme to apply it</source>
        <translation>双击方案可直接应用</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Save Current</source>
        <translation>保存当前布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Apply Selected</source>
        <translation>应用选中布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Delete Selected</source>
        <translation>删除选中布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reset Default</source>
        <translation>恢复默认布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Save the current window layout as a named scheme</source>
        <translation>把当前窗口布局保存为命名方案</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Apply the selected layout scheme</source>
        <translation>应用选中的布局方案</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Delete the selected custom layout scheme (presets cannot be deleted)</source>
        <translation>删除选中的自定义布局方案（预置方案不可删除）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Restore the default window layout</source>
        <translation>恢复默认窗口布局</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Default</source>
        <translation>默认布局</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Focus Analysis</source>
        <translation>专注分析</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>Save Layout</source>
        <translation>保存布局方案</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Layout scheme name:</source>
        <translation>布局方案名称：</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Cannot overwrite preset layout schemes, please use another name</source>
        <translation>不能覆盖预置布局方案，请换一个名称</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Layout scheme &apos;%1&apos; saved</source>
        <translation>布局方案&quot;%1&quot;已保存</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Remove Layout</source>
        <translation>删除布局方案</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove layout scheme &apos;%1&apos;?</source>
        <translation>确定删除布局方案&quot;%1&quot;吗？</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Reset Layout</source>
        <translation>恢复默认布局</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>This will restore the default window layout. Continue?</source>
        <translation>将恢复默认窗口布局，是否继续？</translation>
    </message>
    <message>
        <location line="-15"/>
        <source>Layout scheme &apos;%1&apos; removed</source>
        <translation>布局方案&quot;%1&quot;已删除</translation>
    </message>
</context>
<context>
    <name>DA::DAMarkdownView</name>
    <message>
        <location filename="../DAGui/MarkdownView/DAMarkdownView.cpp" line="+282"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Select All</source>
        <translation>全选</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>View Markdown Source</source>
        <translation>查看 Markdown 源码</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Save Markdown As...</source>
        <translation>保存 Markdown 为...</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Reload</source>
        <translation>重新加载</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Markdown Source</source>
        <translation>Markdown 源码</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Save Markdown</source>
        <translation>保存 Markdown</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Markdown Files (*.md);;Text Files (*.txt);;All Files (*)</source>
        <translation>Markdown 文件 (*.md);;文本文件 (*.txt);;所有文件 (*)</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+5"/>
        <source>Failed to save markdown: %1</source>
        <translation>保存 Markdown 失败：%1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Markdown saved to %1</source>
        <translation>Markdown 已保存到 %1</translation>
    </message>
</context>
<context>
    <name>DA::DAMessageLogViewWidget</name>
    <message>
        <location filename="../DAGui/DAMessageLogViewWidget.cpp" line="+293"/>
        <source>Info</source>
        <translation>信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Info Message</source>
        <translation>显示信息消息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Warning Message</source>
        <translation>显示警告消息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Critical</source>
        <translation>严重</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Show Critical Message</source>
        <translation>显示严重消息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clear</source>
        <translation>清除</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Clear All Messages</source>
        <translation>清空所有消息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Copy Selected Message</source>
        <translation>复制选中消息</translation>
    </message>
</context>
<context>
    <name>DA::DAMessageLogsModel</name>
    <message>
        <location filename="../DAGui/Models/DAMessageLogsModel.cpp" line="+66"/>
        <source>date time</source>
        <translation>日期时间</translation>
    </message>
    <message>
        <location line="+2"/>
        <location line="+5"/>
        <source>message</source>
        <translation>消息</translation>
    </message>
</context>
<context>
    <name>DA::DAModelEditDialog</name>
    <message>
        <location filename="../APP/Dialog/DAModelEditDialog.cpp" line="+38"/>
        <source>Model</source>
        <translation>模型</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>model id, e.g. gpt-4o</source>
        <translation>模型 id，如 gpt-4o</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+5"/>
        <source> tokens</source>
        <translation>token</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Model Id</source>
        <translation>模型 id</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Context Window</source>
        <translation>上下文窗口</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max Output Tokens</source>
        <translation>最大输出 token</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>DA::DAModelFetchDialog</name>
    <message>
        <location filename="../APP/Dialog/DAModelFetchDialog.cpp" line="+33"/>
        <source>Available Models</source>
        <translation>可用模型</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Select models to add:</source>
        <translation>选择要添加的模型：</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Select All</source>
        <translation>全选</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Deselect All</source>
        <translation>全不选</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>DA::DANodeItemSettingWidget</name>
    <message>
        <location filename="../DAGui/DANodeItemSettingWidget.cpp" line="+84"/>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Width</source>
        <translation>宽度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Height</source>
        <translation>高度</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Lock aspect ratio</source>
        <translation>锁定纵横比</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Rotation</source>
        <translation>旋转</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Properties</source>
        <translation>属性</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Movable</source>
        <translation>可移动</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Resizable</source>
        <translation>可缩放</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tooltip</source>
        <translation>提示</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Link point position</source>
        <translation>连接点位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Direction</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Input direction</source>
        <translation>输入方向</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Output direction</source>
        <translation>输出方向</translation>
    </message>
</context>
<context>
    <name>DA::DANodeLinkItemSettingWidget</name>
    <message>
        <location filename="../DAGui/DANodeLinkItemSettingWidget.cpp" line="+33"/>
        <source>pen</source>
        <translation>画笔</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>link style</source>
        <translation>连线样式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Knuckle</source>
        <translation>折线</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Straight</source>
        <translation>直线</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Bezier</source>
        <translation>贝塞尔</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>end point size</source>
        <translation>端点大小</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>front style</source>
        <translation>前端点样式</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>end style</source>
        <translation>后端点样式</translation>
    </message>
    <message>
        <location line="+135"/>
        <source>None</source>
        <translation>无</translation>
    </message>
</context>
<context>
    <name>DA::DANodeParamSettingPanel</name>
    <message>
        <location filename="../DAGui/NodeSetting/DANodeParamSettingPanel.cpp" line="+42"/>
        <source>No configurable parameters</source>
        <translation>无可配置参数</translation>
    </message>
</context>
<context>
    <name>DA::DANodeParamSettingPanelWidget</name>
    <message>
        <location filename="../DAGui/NodeSetting/DANodeParamSettingPanelWidget.cpp" line="+47"/>
        <source>No node selected</source>
        <translation>未选中节点</translation>
    </message>
</context>
<context>
    <name>DA::DANodeSettingWidget</name>
    <message>
        <location filename="../DAGui/DANodeSettingWidget.cpp" line="+126"/>
        <source>Metadata</source>
        <translation>元数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Prototype</source>
        <translation>原型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Group</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
</context>
<context>
    <name>DA::DANodeTreeWidget</name>
    <message>
        <location filename="../DAGui/DANodeTreeWidget.cpp" line="+164"/>
        <source>Favorite</source>
        <translation>收藏</translation>
    </message>
</context>
<context>
    <name>DA::DAPluginManager</name>
    <message>
        <location filename="../DAPluginSupport/DAPluginManager.cpp" line="+57"/>
        <source>The file .pluginignore exists, but failed to read due to the following reason: %1</source>
        <translation>.pluginignore文件存在，但由于以下原因读取失败：%1</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Will ignore plugin:</source>
        <translation>将忽略以下插件</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>No plugins ignore files, a %1 file will be automatically generated</source>
        <translation>缺少插件忽略文件，将自动生成%1文件</translation>
    </message>
    <message>
        <location line="+59"/>
        <source>Plugins have already been loaded, skipping duplicate load.</source>
        <translation>插件已加载，跳过重复加载</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Plugin directory does not exist: %1</source>
        <translation>插件目录不存在：%1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>No plugin files found in: %1</source>
        <translation>插件目录中未找到插件文件：%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>plugin directory is: %1</source>
        <translation>插件目录为：%1</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>ignoring plugin %1</source>
        <translation>忽略插件 %1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>ignoring invalid file: %1</source>
        <translation>忽略无效文件：%1</translation>
    </message>
    <message>
        <location line="+95"/>
        <source>Plugin %1 refused to finalize, skip unload.</source>
        <translation>插件 %1 拒绝清理，跳过卸载</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Plugin Manager Info: is loaded=%1, plugin counts=%2</source>
        <translation>插件管理器信息：已加载=%1，插件数量=%2</translation>
    </message>
    <message>
        <location line="-66"/>
        <source>Plugin %1 refused to finalize, unload cancelled.</source>
        <translation>插件 %1 拒绝完成清理，卸载已取消</translation>
    </message>
    <message>
        <location line="-209"/>
        <source>Failed to create plugin directory: %1</source>
        <translation>创建插件目录失败：%1</translation>
    </message>
    <message>
        <location line="+148"/>
        <source>cannot load plugin: %1</source>
        <translation>无法加载插件：%1</translation>
    </message>
    <message>
        <location line="+70"/>
        <location line="+27"/>
        <source>Failed to unload plugin library for %1.</source>
        <translation>无法卸载插件 %1 的库</translation>
    </message>
    <message>
        <location line="-22"/>
        <source>Plugin %1 not found for unloading.</source>
        <translation>未找到要卸载的插件 %1</translation>
    </message>
</context>
<context>
    <name>DA::DAPluginManagerDialog</name>
    <message>
        <location filename="../APP/DAPluginManagerDialog.cpp" line="+44"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Version</source>
        <translation>版本</translation>
    </message>
    <message>
        <location line="+0"/>
        <location line="+12"/>
        <source>Is Loaded</source>
        <translation>已加载</translation>
    </message>
    <message>
        <location line="-12"/>
        <source>Description</source>
        <translation>描述</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Node Plugin</source>
        <translation>节点插件</translation>
    </message>
</context>
<context>
    <name>DA::DAProviderEditDialog</name>
    <message>
        <location filename="../APP/Dialog/DAProviderEditDialog.cpp" line="+48"/>
        <source>Provider</source>
        <translation>供应商</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>provider name, e.g. OpenAI</source>
        <translation>供应商名称，如 OpenAI</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>https://api.openai.com/v1</source>
        <translation>https://api.openai.com/v1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Base URL</source>
        <translation>基础地址</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>API Key</source>
        <translation>API 密钥</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Models</source>
        <translation>模型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Fetch Available Models</source>
        <translation>获取可用模型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>+ Add Model</source>
        <translation>+ 新增模型</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>- Remove</source>
        <translation>- 删除</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Model Name</source>
        <translation>模型名</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Context Size</source>
        <translation>上下文大小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Max Output Tokens</source>
        <translation>最大输出 token</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>Base URL is required to fetch models</source>
        <translation>获取模型需要先填写基础地址</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Fetching...</source>
        <translation>获取中...</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>✗ Fetch failed: %1</source>
        <translation>✗ 获取失败: %1</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>✗ No models returned</source>
        <translation>✗ 未返回任何模型</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>✓ %1 models fetched</source>
        <translation>✓ 获取到 %1 个模型</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Added %1 models</source>
        <translation>已添加 %1 个模型</translation>
    </message>
    <message>
        <location line="+65"/>
        <source>Provider name already exists</source>
        <translation>供应商名称已存在</translation>
    </message>
</context>
<context>
    <name>DA::DAPyDTypeComboBox</name>
    <message>
        <location filename="../DAPyCommonWidgets/DAPyDTypeComboBox.cpp" line="+50"/>
        <source>float64</source>
        <translation>float64</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>float32</source>
        <translation>float32</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>float16</source>
        <translation>float16</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>int64</source>
        <translation>int64</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>uint64</source>
        <translation>uint64</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>int32</source>
        <translation>int32</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>uint32</source>
        <translation>uint32</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>int16</source>
        <translation>int16</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>uint16</source>
        <translation>uint16</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>int8</source>
        <translation>int8</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>uint8</source>
        <translation>uint8</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>str</source>
        <translation>str</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>bool</source>
        <translation>bool</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>complex64</source>
        <translation>complex64</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>complex128</source>
        <translation>complex128</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>datetime64</source>
        <translation>datetime64</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>timedelta64</source>
        <translation>timedelta64</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>bytes</source>
        <translation>bytes</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>object</source>
        <translation>对象</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Int64 (nullable)</source>
        <translation>Int64（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Int32 (nullable)</source>
        <translation>Int32（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Int16 (nullable)</source>
        <translation>Int16（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Int8 (nullable)</source>
        <translation>Int8（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>UInt64 (nullable)</source>
        <translation>UInt64（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>UInt32 (nullable)</source>
        <translation>UInt32（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>UInt16 (nullable)</source>
        <translation>UInt16（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>UInt8 (nullable)</source>
        <translation>UInt8（可空）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>boolean (nullable)</source>
        <translation>boolean（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>string (nullable)</source>
        <translation>string（可空）</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>category</source>
        <translation>category</translation>
    </message>
</context>
<context>
    <name>DA::DAPyDataFrameTableView</name>
    <message>
        <location filename="../DAGui/DAPyDataFrameTableView.cpp" line="+32"/>
        <source>DataFrameTableView requires a model to be set first</source>
        <translation>你需要先设置模型</translation>
    </message>
</context>
<context>
    <name>DA::DAPyWorkFlowEditWidget</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowEditWidget.cpp" line="+130"/>
        <source>no workflow has been set</source>
        <translation>未设置工作流</translation>
    </message>
</context>
<context>
    <name>DA::DAPyWorkFlowGraphicsView</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowGraphicsView.cpp" line="+225"/>
        <source>cut</source>
        <translation>剪切</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Unrecognized mime formats: %1, paste failed</source>
        <translation>无法识别的mime类型:%1,粘贴失败</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Unsupported pasted content</source>
        <translation>不支持的粘贴内容</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>An exception occurred while parsing and pasting content</source>
        <translation>解析粘贴内容过程出现异常</translation>
    </message>
</context>
<context>
    <name>DA::DAPyWorkFlowNodeItemSettingWidget</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowNodeItemSettingWidget.cpp" line="+39"/>
        <source>Parameters</source>
        <translation>参数</translation>
    </message>
</context>
<context>
    <name>DA::DAPyWorkFlowNodeListWidget</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowNodeListWidget.cpp" line="+123"/>
        <source>Favorite</source>
        <translation>收藏</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove Favorite</source>
        <translation>移除收藏</translation>
    </message>
</context>
<context>
    <name>DA::DAPyWorkFlowOperateWidget</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowOperateWidget.cpp" line="+243"/>
        <source>Title of new workflow</source>
        <translation>新工作流标题</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Title:</source>
        <translation>标题:</translation>
    </message>
    <message>
        <location line="+267"/>
        <location line="+392"/>
        <source>Question</source>
        <translation>疑问</translation>
    </message>
    <message>
        <location line="-391"/>
        <source>Confirm to delete workflow:%1</source>
        <translation>是否确认删除工作流:%1</translation>
    </message>
    <message>
        <location line="+142"/>
        <location line="+13"/>
        <location line="+13"/>
        <source>Missing view</source>
        <translation>缺少视图</translation>
    </message>
    <message>
        <location line="+13"/>
        <location line="+13"/>
        <location line="+15"/>
        <location line="+14"/>
        <location line="+13"/>
        <location line="+13"/>
        <location line="+13"/>
        <location line="+13"/>
        <location line="+482"/>
        <source>No active workflow detected</source>
        <translation>未检测到激活的工作流</translation>
    </message>
    <message>
        <location line="-559"/>
        <source>Workflow execution failed</source>
        <translation>工作流执行失败</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Workflow termination has not been implemented yet</source>
        <translation>工作流终止功能尚未实现</translation>
    </message>
    <message>
        <location line="+179"/>
        <source>Confirm to close workflow</source>
        <translation>是否确认关闭工作流</translation>
    </message>
    <message>
        <location line="+243"/>
        <location line="+1"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Cut</source>
        <translation>剪切</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Paste</source>
        <translation>粘贴</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Select All</source>
        <translation>全选</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Select all items</source>
        <translation>全选所有图元</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom In</source>
        <translation>放大</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom in graphics view</source>
        <translation>放大画布</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom Out</source>
        <translation>缩小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom out graphics view</source>
        <translation>缩小画布</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom to Fit</source>
        <translation>适合屏幕</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zoom to fit screen size</source>
        <translation>缩放到适合屏幕大小</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Cross Line Marker</source>
        <translation>十字标记线</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Horizontal Line Marker</source>
        <translation>水平标记线</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>Vertical Line Marker</source>
        <translation>垂直标记线</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+1"/>
        <source>None Marker</source>
        <translation>无标记线</translation>
    </message>
</context>
<context>
    <name>DA::DAPyWorkFlowScene</name>
    <message>
        <location filename="../DAPyWorkFlow/DAPyWorkFlowScene.cpp" line="+122"/>
        <source>DAPyWorkFlowScene::syncPyNodeRegister: registerNode failed</source>
        <translation>同步Python节点注册失败：registerNode 返回空</translation>
    </message>
    <message>
        <location line="+65"/>
        <source>DAPyWorkFlowScene::addPyNodeLink: connectNode failed, no valid connectionId</source>
        <translation>添加节点连接线失败：connectNode 未返回有效连接 ID</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>DAPyWorkFlowScene::removePyNodeLink: disconnectNode failed for connectionId: %1</source>
        <translation>移除节点连接线失败：断开连接 ID %1 失败</translation>
    </message>
    <message>
        <location line="+151"/>
        <source>DAPyWorkFlowScene::createPyNode: Manager or workflow is not set</source>
        <translation>创建 Python 节点失败：管理器或工作流未设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>DAPyWorkFlowScene::createPyNode: invalid metadata (qualified_name: %1)</source>
        <translation>创建 Python 节点失败：元数据无效（qualified_name: %1）</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>DAPyWorkFlowScene::createPyNode: factory failed to create proxy for %1</source>
        <translation>创建 Python 节点失败：工厂无法为 %1 创建代理</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>DAPyWorkFlowScene::createPyNode: addNode failed for %1</source>
        <translation>创建 Python 节点失败：注册节点 %1 失败</translation>
    </message>
    <message>
        <location line="+135"/>
        <source>Remove Node</source>
        <translation>移除节点</translation>
    </message>
    <message>
        <location line="+501"/>
        <source>Remove Selected Items</source>
        <translation>移除选中项</translation>
    </message>
    <message>
        <location line="+271"/>
        <source>DAPyWorkFlowScene::saveToXml failed: %1</source>
        <translation>保存场景到 XML 失败：%1</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>DAPyWorkFlowScene::loadFromXml: DAPyWorkFlowScene element not found</source>
        <translation>从 XML 加载场景失败：未找到 DAPyWorkFlowScene 元素</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>DAPyWorkFlowScene::loadFromXml failed: %1</source>
        <translation>从 XML 加载场景失败：%1</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>DAPyWorkFlowScene::saveToFile failed: %1</source>
        <translation>保存场景到文件失败：%1</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>DAPyWorkFlowScene::loadFromFile failed: %1</source>
        <translation>从文件加载场景失败：%1</translation>
    </message>
</context>
<context>
    <name>DA::DARecentFilesManager</name>
    <message>
        <location filename="../DAGui/DARecentFilesManager.cpp" line="+153"/>
        <source>(empty)</source>
        <translation>空</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Clear menu</source>
        <translation>清空</translation>
    </message>
    <message>
        <location filename="../DAGui/DARecentFilesManager.h" line="+24"/>
        <source>Recent files</source>
        <translation>最近打开文件</translation>
    </message>
</context>
<context>
    <name>DA::DARenameColumnsNameDialog</name>
    <message>
        <location filename="../DAGui/Dialog/DARenameColumnsNameDialog.cpp" line="+59"/>
        <source>name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Duplicate column name &quot;%1&quot;, please reset the column name of column %2</source>
        <translation>列名“%1”存在重复，请重新设置第%2列的列名</translation>
    </message>
</context>
<context>
    <name>DA::DASettingPageAdvanced</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPageAdvanced.cpp" line="+20"/>
        <location line="+14"/>
        <source> s</source>
        <translation>秒</translation>
    </message>
    <message>
        <location line="-13"/>
        <source>Unlimited</source>
        <translation>无限</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> day</source>
        <translation>天</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> min</source>
        <translation>分钟</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+5"/>
        <source>Disabled</source>
        <translation>禁用</translation>
    </message>
    <message>
        <location line="-3"/>
        <source>Empty for system temporary directory</source>
        <translation>留空使用系统临时目录</translation>
    </message>
    <message>
        <location line="+86"/>
        <source>Advanced</source>
        <translation>高级</translation>
    </message>
    <message>
        <location line="+74"/>
        <source>Select plugin search path</source>
        <translation>选择插件搜索路径</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Select node script search path</source>
        <translation>选择节点脚本搜索路径</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Select script workspace directory</source>
        <translation>选择脚本工作区目录</translation>
    </message>
</context>
<context>
    <name>DA::DASettingPageGeneral</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPageGeneral.cpp" line="+78"/>
        <source>Windows 7</source>
        <translation>Windows 7</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2013</source>
        <translation>Office 2013</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2016 Blue</source>
        <translation>Office 2016 蓝色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2016 Green</source>
        <translation>Office 2016 绿色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2016 Dark</source>
        <translation>Office 2016 深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2021 Blue</source>
        <translation>Office 2021 蓝色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2021 Green</source>
        <translation>Office 2021 绿色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Office 2021 Dark</source>
        <translation>Office 2021 深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark</source>
        <translation>深色</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dark 2</source>
        <translation>深色2</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>System</source>
        <translation>跟随系统</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>General</source>
        <translation>通用</translation>
    </message>
    <message>
        <location line="+130"/>
        <source>Question</source>
        <translation>疑问</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>This operation will delete the file that records the window state information. After deleting the file, if the window state information recording is not enabled, the window will open in the default layout</source>
        <translation>此操作将删除记录窗口位置信息的文件，删除文件后，如果不开启窗口位置信息记录，窗口将以默认布局打开</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Successfully removed window state record file</source>
        <translation>成功删除窗口状态记录文件</translation>
    </message>
</context>
<context>
    <name>DA::DASettingPageLog</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPageLog.cpp" line="+21"/>
        <source>Rotating</source>
        <translation>按大小轮转</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Daily</source>
        <translation>按日期分割</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Console only</source>
        <translation>仅控制台</translation>
    </message>
    <message>
        <location line="+4"/>
        <source> MB</source>
        <translation> MB</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>Trace</source>
        <translation>跟踪</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Debug</source>
        <translation>调试</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Info</source>
        <translation>信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Critical</source>
        <translation>严重</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Off</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Log</source>
        <translation>消息</translation>
    </message>
</context>
<context>
    <name>DA::DASettingPagePython</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPagePython.cpp" line="+60"/>
        <source>Cannot write python config file: %1</source>
        <translation>无法写入Python配置文件：%1</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Python</source>
        <translation>Python</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Select Python Interpreter</source>
        <translation>选择Python解释器</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Information</source>
        <translation>信息</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No Python interpreter found in system PATH</source>
        <translation>系统PATH中未找到Python解释器</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Please specify a Python interpreter path</source>
        <translation>请指定Python解释器路径</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>File does not exist: %1</source>
        <translation>文件不存在：%1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Failed to run: %1</source>
        <translation>运行失败：%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>OK</source>
        <translation>正常</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Select module search path</source>
        <translation>选择模块搜索路径</translation>
    </message>
</context>
<context>
    <name>DA::DASettingWidget</name>
    <message>
        <location filename="../DAGui/DASettingWidget.cpp" line="+97"/>
        <source>page changed, but cannot identify the sender widget</source>
        <translation>页面已更改，但无法识别发送者控件</translation>
    </message>
</context>
<context>
    <name>DA::DAStatusBarWidget</name>
    <message>
        <location filename="../DAGui/DAStatusBarWidget.cpp" line="+47"/>
        <source>Workflow</source>
        <translation>工作流</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Switch to Workflow Mode</source>
        <translation>切换为工作流模式</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Switch to Data Mode</source>
        <translation>切换为数据模式</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Chart</source>
        <translation>图表</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Switch to Chart Mode</source>
        <translation>切换为图表模式</translation>
    </message>
</context>
<context>
    <name>DA::DATableDisplayFormatComboBox</name>
    <message>
        <location filename="../DAGui/DATableDisplayFormatComboBox.cpp" line="+38"/>
        <location line="+31"/>
        <source>General</source>
        <translation>通用</translation>
    </message>
    <message>
        <location line="-30"/>
        <location line="+32"/>
        <source>Number</source>
        <translation>数值</translation>
    </message>
    <message>
        <location line="-31"/>
        <location line="+32"/>
        <source>Scientific</source>
        <translation>科学计数法</translation>
    </message>
    <message>
        <location line="-31"/>
        <location line="+32"/>
        <source>Percentage</source>
        <translation>百分比</translation>
    </message>
    <message>
        <location line="-31"/>
        <location line="+34"/>
        <source>Date/Time</source>
        <translation>日期时间</translation>
    </message>
    <message>
        <location line="-33"/>
        <location line="+34"/>
        <source>Datetime as Number</source>
        <translation>时间显示为数字</translation>
    </message>
    <message>
        <location line="-33"/>
        <location line="+36"/>
        <source>Text</source>
        <translation>文本</translation>
    </message>
</context>
<context>
    <name>DA::DAToolBox</name>
    <message>
        <location filename="../DAGui/DAToolBox.cpp" line="+88"/>
        <source>Favorite</source>
        <translation>收藏</translation>
    </message>
</context>
<context>
    <name>DA::DATreeModel</name>
    <message>
        <location filename="../DAGui/Models/DATreeModel.cpp" line="+114"/>
        <source>DATreeModel encountered invalid item</source>
        <translation>DATreeModel遇到无效的item</translation>
    </message>
</context>
<context>
    <name>DA::DATxtFileImportDialog</name>
    <message>
        <location filename="../DAGui/Dialog/DATxtFileImportDialog.cpp" line="+49"/>
        <source>,(comma)</source>
        <translation>,逗号</translation>
    </message>
    <message>
        <location line="+1"/>
        <source> (space)</source>
        <translation>空格</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>\t(tab stop)</source>
        <translation>tab制表位</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>;(semicolon)</source>
        <translation>;分号</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>_(underscore)</source>
        <translation>_下横杠</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>-(dash)</source>
        <translation>-横杠</translation>
    </message>
    <message>
        <location line="+137"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Failed to read text file(%1), reason: %2</source>
        <translation>读取文本文件(%1)失败,原因:%2</translation>
    </message>
</context>
<context>
    <name>DA::DAWorkbenchAboutDialog</name>
    <message>
        <location filename="../APP/Dialog/DAWorkbenchAboutDialog.cpp" line="+138"/>
        <source>Version %1.%2.%3</source>
        <translation>版本 %1.%2.%3</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>AI Agent driven data analysis workbench</source>
        <translation>AI Agent 驱动的数据分析工作台</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The third-party libraries listed above retain their original licenses, please refer to the corresponding projects for details.</source>
        <translation>上述第三方库保留其原始许可证，详见对应项目。</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>DAWorkbench is an AI Agent driven data analysis workbench built on C++17/Qt, featuring a directed-graph workflow engine, embedded Python (pandas/numpy) data processing, interactive publication-grade charting, and a plugin architecture supporting both C++ and Python extensions.</source>
        <translation>DAWorkbench 是一个基于 C++17/Qt 的 AI Agent 驱动数据分析工作台，具备有向图工作流引擎、内嵌 Python（pandas/numpy）数据处理、交互式出版级图表绘制，以及同时支持 C++ 与 Python 扩展的插件架构。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>This software is open source under the LGPL v3.0 license.</source>
        <translation>本软件基于 LGPL v3.0 许可证开源。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Project homepage: %1</source>
        <translation>项目主页：%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Documentation: %1</source>
        <translation>文档：%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Contact email: %1</source>
        <translation>联系邮箱：%1</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>C++ Third-Party Libraries</source>
        <translation>C++ 第三方库</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Application and UI framework</source>
        <translation>应用程序与 UI 框架</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Ribbon style main window framework</source>
        <translation>Ribbon 风格主窗口框架</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Advanced docking system</source>
        <translation>高级停靠系统</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Plotting engine (maintained fork with QwtFigure extensions)</source>
        <translation>绘图引擎（维护分支，含 QwtFigure 扩展）</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Logging library</source>
        <translation>日志库</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Seamless C++/Python interoperability</source>
        <translation>C++/Python 无缝互操作</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>ZIP archive reading/writing</source>
        <translation>ZIP 压缩包读写</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Data compression</source>
        <translation>数据压缩</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Ordered hash map</source>
        <translation>有序哈希表</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Lightweight CTK widget set</source>
        <translation>轻量 CTK 控件集</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>General purpose widgets</source>
        <translation>通用控件</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+35"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="-34"/>
        <location line="+35"/>
        <source>Usage</source>
        <translation>用途</translation>
    </message>
    <message>
        <location line="-34"/>
        <location line="+35"/>
        <source>License</source>
        <translation>许可证</translation>
    </message>
    <message>
        <location line="-34"/>
        <source>Version</source>
        <translation>版本</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Python Dependencies</source>
        <translation>Python 依赖</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Embedded Python interpreter: %1</source>
        <translation>内嵌 Python 解释器：%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Core data analysis</source>
        <translation>核心数据分析</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Numerical computing</source>
        <translation>数值计算</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Scientific computing</source>
        <translation>科学计算</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Excel file reading/writing</source>
        <translation>Excel 文件读写</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Text encoding detection</source>
        <translation>文本编码检测</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Arrow/Parquet data format</source>
        <translation>Arrow/Parquet 数据格式</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Plotting foundation</source>
        <translation>绘图基础</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Statistical plotting</source>
        <translation>统计绘图</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Wavelet analysis</source>
        <translation>小波分析</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>AI Agent orchestration framework</source>
        <translation>AI Agent 编排框架</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>LLM service integration</source>
        <translation>LLM 服务集成</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>LangGraph command line tools</source>
        <translation>LangGraph 命令行工具</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data model validation</source>
        <translation>数据模型校验</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Token counting</source>
        <translation>Token 计数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Python logging</source>
        <translation>Python 日志</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Type hint backports</source>
        <translation>类型注解向后移植</translation>
    </message>
</context>
<context>
    <name>DAAppController</name>
    <message>
        <location filename="../APP/DAAppController.cpp" line="-1487"/>
        <source>DA</source>
        <translation>DA</translation>
    </message>
</context>
<context>
    <name>DAAppRibbonArea</name>
    <message>
        <location line="+11"/>
        <source>warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>The current function is not implemented, only the UI is reserved, please pay attention: https://gitee.com/czyt1988/data-work-flow</source>
        <translation>当前功能未实现，仅保留UI，请留意：https://gitee.com/czyt1988/data-work-flow</translation>
    </message>
</context>
<context>
    <name>DAAxObjectExcelWrapper</name>
    <message>
        <location filename="../DAAxOfficeWrapper/DAAxObjectExcelWrapper.cpp" line="-711"/>
        <source>File &quot;%1&quot; does not exist</source>
        <translation>文件 &quot;%1&quot; 不存在</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Failed to open Excel file &quot;%1&quot;</source>
        <translation>无法打开 Excel 文件 &quot;%1&quot;</translation>
    </message>
</context>
<context>
    <name>DAChart3DCommonItemsSettingWidget</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DCommonItemsSettingWidget.ui" line="+14"/>
        <source>3D Common Item Setting</source>
        <translation>3D 通用元素设置</translation>
    </message>
</context>
<context>
    <name>DAChart3DSettingWidget</name>
    <message>
        <location filename="../DAGui/Chart3DSetting/DAChart3DSettingWidget.ui" line="+14"/>
        <source>3D Chart Setting</source>
        <translation>3D 图表设置</translation>
    </message>
</context>
<context>
    <name>DAChartAdd3DBarWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAdd3DBarWidget.ui" line="+14"/>
        <source>Add 3D Bar</source>
        <translation>添加 3D 柱状图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data Mode</source>
        <translation>数据模式</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>1D Series (one column)</source>
        <translation>一维序列（单列）</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>2D Grid (DataFrame as Z matrix)</source>
        <translation>二维网格（DataFrame 作为 Z 矩阵）</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>DataFrame</source>
        <translation>DataFrame</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Value Column</source>
        <translation>数值列</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Rows and columns of the DataFrame are used as Y and X coordinates, cell values as bar height.</source>
        <translation>DataFrame 的行和列分别作为 Y 和 X 坐标，单元格数值作为柱高</translation>
    </message>
</context>
<context>
    <name>DAChartAdd3DLineWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAdd3DLineWidget.ui" line="+14"/>
        <source>Add 3D Line</source>
        <translation>添加 3D 折线图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>DataFrame</source>
        <translation>DataFrame</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>X Column</source>
        <translation>X 列</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Y Column</source>
        <translation>Y 列</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Z Column</source>
        <translation>Z 列</translation>
    </message>
</context>
<context>
    <name>DAChartAdd3DSurfaceWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAdd3DSurfaceWidget.ui" line="+14"/>
        <source>Add 3D Surface</source>
        <translation>添加 3D 曲面图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data Mode</source>
        <translation>数据模式</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Grid (DataFrame as Z matrix)</source>
        <translation>网格（DataFrame 作为 Z 矩阵）</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Scatter (X/Y/Z columns)</source>
        <translation>散点（X/Y/Z 列）</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>DataFrame</source>
        <translation>DataFrame</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Rows and columns of the DataFrame are used as Y and X coordinates, cell values as Z height.</source>
        <translation>DataFrame 的行和列分别作为 Y 和 X 坐标，单元格数值作为 Z 高度</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>X Column</source>
        <translation>X 列</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Y Column</source>
        <translation>Y 列</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Z Column</source>
        <translation>Z 列</translation>
    </message>
</context>
<context>
    <name>DAChartAddBoxChartWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddBoxChartWidget.ui" line="+14"/>
        <source>Add Box Chart</source>
        <translation>添加箱线图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>DataFrame</source>
        <translation>DataFrame</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Columns</source>
        <translation>柱状</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Select columns to plot as box charts</source>
        <translation>选择要绘制箱线图的列</translation>
    </message>
</context>
<context>
    <name>DAChartAddContourWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddContourWidget.ui" line="+14"/>
        <source>Add Contour Map</source>
        <translation>添加等高线图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Value</source>
        <translation>值</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Select three columns (x, y, value) to render a contour map</source>
        <translation>选择三列（x、y、数值）以绘制等高线图</translation>
    </message>
</context>
<context>
    <name>DAChartAddCurveWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddCurveWidget.ui" line="+14"/>
        <source>Add XY Series</source>
        <translation>添加 XY 序列</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+12"/>
        <location line="+61"/>
        <source>Autoincrement series</source>
        <translation>自增序列</translation>
    </message>
    <message>
        <location line="-46"/>
        <location line="+61"/>
        <source>Initial value</source>
        <translation>初始值</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+14"/>
        <location line="+47"/>
        <location line="+14"/>
        <source>1</source>
        <translation>1</translation>
    </message>
    <message>
        <location line="-68"/>
        <location line="+61"/>
        <source>Self increasing step size</source>
        <translation>自增步长</translation>
    </message>
    <message>
        <location line="-41"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
</context>
<context>
    <name>DAChartAddHistogramWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddHistogramWidget.ui" line="+14"/>
        <source>Add Histogram</source>
        <translation>添加直方图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data Series</source>
        <translation>数据序列</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Bins</source>
        <translation>分箱</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Number of bins</source>
        <translation>分箱数量</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Y Axis</source>
        <translation>Y轴</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Count</source>
        <translation>计数</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Density</source>
        <translation>密度</translation>
    </message>
</context>
<context>
    <name>DAChartAddMultiBarWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddMultiBarWidget.ui" line="+14"/>
        <source>Add Multi Bar Chart</source>
        <translation>添加多系列柱状图</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Autoincrement series</source>
        <translation>自增序列</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Initial value</source>
        <translation>初始值</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+14"/>
        <source>1</source>
        <translation>1</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>Self increasing step size</source>
        <translation>自增步长</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Y (Multiple)</source>
        <translation>Y（多选）</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Drag the data into the corresponding list. Multiple Y series will be grouped as bar sets.</source>
        <translation>将数据拖入对应列表，多个 Y 序列将分组为柱组</translation>
    </message>
</context>
<context>
    <name>DAChartAddOHLCSeriesWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddOHLCSeriesWidget.ui" line="+14"/>
        <source>Add XY Series</source>
        <translation>添加 XY 序列</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Time</source>
        <translation>时间</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Autoincrement series</source>
        <translation>自增序列</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Initial value</source>
        <translation>初始值</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+14"/>
        <source>1</source>
        <translation>1</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>Self increasing step size</source>
        <translation>自增步长</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Open</source>
        <translation>打开</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Hight</source>
        <translation>最高</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Low</source>
        <translation>最低</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Close</source>
        <translation>收盘</translation>
    </message>
</context>
<context>
    <name>DAChartAddSpectrogramWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddSpectrogramWidget.ui" line="+14"/>
        <source>Add Curve</source>
        <translation>添加曲线</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>1.Data</source>
        <translation>1.数据</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>&gt;&gt;</source>
        <translation>&gt;&gt;</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>2.Plot</source>
        <translation>2.绘图</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsBarplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsBarplotWidget.ui" line="+14"/>
        <source>Barplot Settings</source>
        <translation>柱状图设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X axis (categorical)</source>
        <translation>X 轴（分类）</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Y axis (numeric, optional — empty for countplot)</source>
        <translation>Y 轴（数值列，可选——留空则绘制计数图）</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Hue (optional grouping column)</source>
        <translation>分组列（可选）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Statistics</source>
        <translation>数据统计</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Estimator</source>
        <translation>估计函数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>mean</source>
        <translation>均值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>median</source>
        <translation>中位数</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>sum</source>
        <translation>求和</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>count</source>
        <translation>计数</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>std</source>
        <translation>标准差</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>var</source>
        <translation>方差</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Confidence interval</source>
        <translation>置信区间</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>68</source>
        <translation>68</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>95</source>
        <translation>95</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>99</source>
        <translation>99</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Bootstrap iterations</source>
        <translation>Bootstrap 迭代次数</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Orientation</source>
        <translation>方向</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>vertical</source>
        <translation>垂直</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>horizontal</source>
        <translation>水平</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Bar width</source>
        <translation>柱宽</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Show legend (hue)</source>
        <translation>显示图例（分组）</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsBoxplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsBoxplotWidget.ui" line="+14"/>
        <source>Stats Boxplot Settings</source>
        <translation>统计箱线图设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data (select one or more columns)</source>
        <translation>数据（选择一列或多列）</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Hue (optional grouping column)</source>
        <translation>分组列（可选）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Options</source>
        <translation>选项</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Whisker multiplier (IQR)</source>
        <translation>须线倍数（IQR）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Box width (0-1)</source>
        <translation>箱体宽度（0-1）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Show outliers</source>
        <translation>显示离群点</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Show means</source>
        <translation>显示均值</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsEcdfplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsEcdfplotWidget.ui" line="+14"/>
        <source>ECDF Plot Settings</source>
        <translation>ECDF 图设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Hue (optional grouping column)</source>
        <translation>分组列（可选）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Weights (optional — overrides stat to proportion)</source>
        <translation>权重（可选——统计量将强制为占比）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Options</source>
        <translation>选项</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Statistic</source>
        <translation>统计量</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>proportion</source>
        <translation>占比</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>count</source>
        <translation>计数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Complementary CDF (1-CDF)</source>
        <translation>互补 CDF（1-CDF）</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Line width</source>
        <translation>线宽</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Show legend (hue)</source>
        <translation>显示图例（分组）</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsHeatmapWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsHeatmapWidget.ui" line="+14"/>
        <source>Stats Heatmap Settings</source>
        <translation>统计热力图设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X axis (column direction)</source>
        <translation>X 轴（列方向）</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Y axis (row direction)</source>
        <translation>Y 轴（行方向）</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Value column (optional — unchecked = count)</source>
        <translation>数值列（可选——不勾选则计数）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Aggregation &amp;&amp; Colour</source>
        <translation>聚合与颜色</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Aggregation function</source>
        <translation>聚合函数</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>mean</source>
        <translation>均值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>sum</source>
        <translation>求和</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>count</source>
        <translation>计数</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>median</source>
        <translation>中位数</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>max</source>
        <translation>最大值</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>min</source>
        <translation>最小值</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Colour map</source>
        <translation>色图</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>viridis</source>
        <translation>viridis</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>plasma</source>
        <translation>plasma</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>inferno</source>
        <translation>inferno</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>magma</source>
        <translation>magma</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>cividis</source>
        <translation>cividis</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>coolwarm</source>
        <translation>coolwarm</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>RdBu_r</source>
        <translation>RdBu_r</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Standardisation</source>
        <translation>标准化</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>row</source>
        <translation>按行</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>column</source>
        <translation>按列</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Value Range (0 = auto)</source>
        <translation>数值范围（0 = 自动）</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>vmin (0 = auto)</source>
        <translation>最小值 vmin（0 = 自动）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>vmax (0 = auto)</source>
        <translation>最大值 vmax（0 = 自动）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>center (0 = none)</source>
        <translation>中心值 center（0 = 无）</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Annotations</source>
        <translation>标注</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Annotate cells</source>
        <translation>标注单元格</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Format string</source>
        <translation>格式字符串</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>.2f</source>
        <translation>.2f</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsHistplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsHistplotWidget.ui" line="+14"/>
        <source>Stats Histplot Settings</source>
        <translation>统计直方图设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Hue (optional grouping column)</source>
        <translation>分组列（可选）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Bins</source>
        <translation>分箱</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Number of bins</source>
        <translation>分箱数量</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Bin width (0 = auto)</source>
        <translation>分箱宽度（0 = 自动）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Bin range min (blank = auto)</source>
        <translation>分箱范围下限（留空 = 自动）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Bin range max (blank = auto)</source>
        <translation>分箱范围上限（留空 = 自动）</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Statistics</source>
        <translation>数据统计</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Y-axis statistic</source>
        <translation>Y 轴统计量</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>count</source>
        <translation>计数</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>density</source>
        <translation>密度</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>frequency</source>
        <translation>频数</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>probability</source>
        <translation>概率</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>percent</source>
        <translation>百分比</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Overlay KDE curve</source>
        <translation>叠加 KDE 曲线</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>KDE bandwidth</source>
        <translation>KDE 带宽</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>scott</source>
        <translation>scott</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>silverman</source>
        <translation>silverman</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Cumulative</source>
        <translation>累积</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Fill bars</source>
        <translation>填充柱形</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsKdeplot1dWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsKdeplot1dWidget.ui" line="+14"/>
        <source>Stats KDE 1D Settings</source>
        <translation>统计 KDE 一维设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Hue (optional grouping column)</source>
        <translation>分组列（可选）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Bandwidth</source>
        <translation>带宽</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Bandwidth method</source>
        <translation>带宽方法</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>scott</source>
        <translation>scott</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>silverman</source>
        <translation>silverman</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>custom</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Custom bandwidth value</source>
        <translation>自定义带宽值</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Grid size (smoothness)</source>
        <translation>网格大小（平滑度）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Options</source>
        <translation>选项</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Fill area under curve</source>
        <translation>填充曲线下区域</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Shade mode (low alpha)</source>
        <translation>阴影模式（低透明度）</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Cumulative distribution</source>
        <translation>累积分布</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Common normalization (hue)</source>
        <translation>统一归一化（按分组）</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Threshold fill (requires fill)</source>
        <translation>阈值填充（需启用填充）</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Fill x &lt;=</source>
        <translation>填充 x &lt;=</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Fill x &gt;=</source>
        <translation>填充 x &gt;=</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>(NaN = no lower limit)</source>
        <translation>（NaN = 无下限）</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>(NaN = no upper limit)</source>
        <translation>（NaN = 无上限）</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsKdeplot2dWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsKdeplot2dWidget.ui" line="+14"/>
        <source>Stats KDE 2D Settings</source>
        <translation>统计 KDE 二维设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Bandwidth</source>
        <translation>带宽</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Bandwidth method</source>
        <translation>带宽方法</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>scott</source>
        <translation>scott</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>silverman</source>
        <translation>silverman</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>custom</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Custom bandwidth value</source>
        <translation>自定义带宽值</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Grid size (resolution)</source>
        <translation>网格大小（分辨率）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Contours</source>
        <translation>等高线</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Number of contour levels</source>
        <translation>等高线层数</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Threshold (0-1)</source>
        <translation>阈值（0-1）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Fill contours</source>
        <translation>填充等高线区域</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Show heatmap</source>
        <translation>显示热力图</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Options</source>
        <translation>选项</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Common normalization (reserved for hue)</source>
        <translation>统一归一化（为分组预留）</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsRegplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsRegplotWidget.ui" line="+14"/>
        <source>Regplot Settings</source>
        <translation>回归图设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X axis</source>
        <translation>X 轴</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Y axis</source>
        <translation>Y 轴</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Regression</source>
        <translation>回归</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Polynomial order</source>
        <translation>多项式阶数</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Confidence interval</source>
        <translation>置信区间</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>68</source>
        <translation>68</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>95</source>
        <translation>95</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>99</source>
        <translation>99</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Bootstrap iterations</source>
        <translation>Bootstrap 迭代次数</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Draw scatter points</source>
        <translation>绘制散点</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Draw regression line</source>
        <translation>绘制回归线</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Scatter style</source>
        <translation>散点样式</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Marker size</source>
        <translation>标记大小</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Alpha (transparency)</source>
        <translation>透明度（Alpha）</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Regression line style</source>
        <translation>回归线样式</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Line width</source>
        <translation>线宽</translation>
    </message>
</context>
<context>
    <name>DAChartAddStatsScatterplotWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddStatsScatterplotWidget.ui" line="+14"/>
        <source>Stats Scatterplot Settings</source>
        <translation>统计散点图设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X axis</source>
        <translation>X 轴</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Y axis</source>
        <translation>Y 轴</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Hue (optional grouping column)</source>
        <translation>分组列（可选）</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Marker</source>
        <translation>标记</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Marker size</source>
        <translation>标记大小</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Marker style</source>
        <translation>标记样式</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>circle</source>
        <translation>圆形</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>square</source>
        <translation>方形</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>diamond</source>
        <translation>菱形</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>triangle</source>
        <translation>三角形</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>cross</source>
        <translation>十字形</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>plus</source>
        <translation>加号形</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>star</source>
        <translation>星形</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Alpha (transparency)</source>
        <translation>透明度（Alpha）</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Show legend (hue)</source>
        <translation>显示图例（分组）</translation>
    </message>
</context>
<context>
    <name>DAChartAddVectorFieldWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddVectorFieldWidget.ui" line="+14"/>
        <source>Add Vector Field</source>
        <translation>添加向量场</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>U (vector x)</source>
        <translation>U（向量 x 分量）</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>V (vector y)</source>
        <translation>V（向量 y 分量）</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Select four columns (x, y, u, v) to render a vector field</source>
        <translation>选择四列（x、y、u、v）以绘制向量场</translation>
    </message>
</context>
<context>
    <name>DAChartAddXYESeriesWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddXYESeriesWidget.ui" line="+14"/>
        <source>Add XY Series</source>
        <translation>添加 XY 序列</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+12"/>
        <location line="+61"/>
        <source>Autoincrement series</source>
        <translation>自增序列</translation>
    </message>
    <message>
        <location line="-46"/>
        <location line="+75"/>
        <source>Initial value</source>
        <translation>初始值</translation>
    </message>
    <message>
        <location line="-68"/>
        <location line="+14"/>
        <location line="+40"/>
        <location line="+7"/>
        <source>1</source>
        <translation>1</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+68"/>
        <source>Self increasing step size</source>
        <translation>自增步长</translation>
    </message>
    <message>
        <location line="-48"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+67"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
</context>
<context>
    <name>DAChartAddXYSeriesWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddXYSeriesWidget.ui" line="+14"/>
        <source>Add XY Series</source>
        <translation>添加 XY 序列</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Add series to X</source>
        <translation>添加序列到 X</translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+86"/>
        <source>Autoincrement series</source>
        <translation>自增序列</translation>
    </message>
    <message>
        <location line="-71"/>
        <location line="+86"/>
        <source>Initial value</source>
        <translation>初始值</translation>
    </message>
    <message>
        <location line="-79"/>
        <location line="+14"/>
        <location line="+72"/>
        <location line="+14"/>
        <source>1</source>
        <translation>1</translation>
    </message>
    <message>
        <location line="-93"/>
        <location line="+86"/>
        <source>Self increasing step size</source>
        <translation>自增步长</translation>
    </message>
    <message>
        <location line="-66"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Add series to Y</source>
        <translation>添加序列到 Y</translation>
    </message>
    <message>
        <location line="+87"/>
        <source>Drag the data into the corresponding list</source>
        <translation>把数据拖入对应的列表</translation>
    </message>
</context>
<context>
    <name>DAChartAddtGridRasterDataWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartAddtGridRasterDataWidget.ui" line="+14"/>
        <source>Add XY Series</source>
        <translation>添加 XY 序列</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Matrics</source>
        <translation>矩阵</translation>
    </message>
</context>
<context>
    <name>DAChartCommonItemsSettingWidget</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartCommonItemsSettingWidget.ui" line="+14"/>
        <source>Common Item Setting</source>
        <translation>通用图元设置</translation>
    </message>
</context>
<context>
    <name>DAChartManageWidget</name>
    <message>
        <location filename="../DAGui/Chart/DAChartManageWidget.ui" line="+14"/>
        <source>Chart Manage</source>
        <translation>图表管理</translation>
    </message>
</context>
<context>
    <name>DAChartSeriesPickerWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartSeriesPickerWidget.ui" line="+14"/>
        <source>Select Series</source>
        <translation>选择序列</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>X:</source>
        <translation>X:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>data[&apos;column&apos;]</source>
        <translation>data[&apos;column&apos;]</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Return to add chart</source>
        <translation>回到添加绘图</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>...</source>
        <translation>...</translation>
    </message>
</context>
<context>
    <name>DAChartSeriesSelectWidget</name>
    <message>
        <location filename="../DAGui/ChartAddItem/DAChartSeriesSelectWidget.ui" line="+14"/>
        <source>Series Select</source>
        <translation>序列选择</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Add series</source>
        <translation>添加序列</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Remove selected series</source>
        <translation>移除所选序列</translation>
    </message>
</context>
<context>
    <name>DAChartSettingWidget</name>
    <message>
        <location filename="../DAGui/Chart/DAChartSettingWidget.ui" line="+14"/>
        <source>Chart Setting</source>
        <translation>图表设置</translation>
    </message>
</context>
<context>
    <name>DAChartSymbolEditWidget</name>
    <message>
        <location filename="../DAGui/ChartSetting/DAChartSymbolEditWidget.ui" line="+26"/>
        <source>Chart Symbol Edit</source>
        <translation>图表符号编辑</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Fill Color</source>
        <translation>填充颜色</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Style</source>
        <translation>样式</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Outline</source>
        <translation>轮廓</translation>
    </message>
</context>
<context>
    <name>DADataManageWidget</name>
    <message>
        <location filename="../DAGui/DADataManageWidget.ui" line="+14"/>
        <source>Data Manage</source>
        <translation>数据管理</translation>
    </message>
</context>
<context>
    <name>DADataManagerTreeWidget</name>
    <message>
        <location filename="../DAGui/DADataManagerTreeWidget.ui" line="+14"/>
        <source>Form</source>
        <translation>窗体</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>-</source>
        <translation>-</translation>
    </message>
</context>
<context>
    <name>DADataOperateOfDataFrameWidget</name>
    <message>
        <location filename="../DAGui/DADataOperateOfDataFrameWidget.ui" line="+14"/>
        <source>DataFrame Operate</source>
        <translation>DataFrame 操作</translation>
    </message>
</context>
<context>
    <name>DADataframeToVectorPointWidget</name>
    <message>
        <location filename="../DAGui/DADataframeToVectorPointWidget.ui" line="+14"/>
        <source>Dataframe To Vector Point</source>
        <translation>DataFrame 转矢量点</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>x:</source>
        <translation>x:</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>y:</source>
        <translation>y:</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>data view</source>
        <translation>数据视图</translation>
    </message>
</context>
<context>
    <name>DADialogChartGuide</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogChartGuide.ui" line="+14"/>
        <source>Chart Guide</source>
        <translation>图表向导</translation>
    </message>
</context>
<context>
    <name>DADialogDataFrameSeriesSelector</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogDataFrameSeriesSelector.ui" line="+14"/>
        <source>Dialog</source>
        <translation>对话框</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Data preview</source>
        <translation>数据预览</translation>
    </message>
</context>
<context>
    <name>DADialogDataframeColumnCastToDatetime</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogDataframeColumnCastToDatetime.ui" line="+14"/>
        <source>Cast To Datetime</source>
        <translation>转换为日期时间</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If True and no format is given, attempt to infer the format of the datetime strings, and if it can be inferred, switch to a faster method of parsing them. In some cases this can increase the parsing speed by ~5-10x.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;若为 True 且未指定 format，将尝试推断日期时间字符串的格式，若可推断则切换到更快的解析方式。某些情况下可将解析速度提升约 5-10 倍。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>infer datetime format </source>
        <translation>推断日期时间格式</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+8"/>
        <source>format</source>
        <translation>格式</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>%d/%m/%Y</source>
        <translation>%d/%m/%Y</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If True, require an exact format match.&lt;/p&gt;&lt;p&gt;If False, allow the format to match anywhere in the target string.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;若为 True，要求格式精确匹配。&lt;/p&gt;&lt;p&gt;若为 False，允许格式匹配目标字符串的任意位置。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>exact </source>
        <translation>精确匹配</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>errors:</source>
        <translation>错误处理:</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>invalid parsing will raise an exception</source>
        <translation>无效解析将抛出异常</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>invalid parsing will be set as NaN</source>
        <translation>无效解析将设为 NaN</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>invalid parsing will return the input</source>
        <translation>无效解析将返回原输入</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Define the reference date. The numeric values would be parsed as number of units (defined by unit) since this reference date.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;定义参考日期。数值将被解析为自该参考日期起以指定单位（由 unit 定义）计数的值。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>origin</source>
        <translation>参考日期</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;origin is set to 1970-01-01&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;参考日期设为 1970-01-01&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>unix</source>
        <translation>unix</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;unit must be ‘D’, and origin is set to beginning of Julian Calendar. Julian day number 0 is assigned to the day starting at noon on January 1, 4713 BC&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;单位必须为 ‘D’，参考日期设为儒略历的起始。儒略日 0 对应公元前 4713 年 1 月 1 日正午开始的那一天。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>julian</source>
        <translation>julian</translation>
    </message>
    <message>
        <location line="+10"/>
        <location line="+9"/>
        <location line="+10"/>
        <location line="+10"/>
        <location line="+13"/>
        <location line="+10"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;unit of the arg (D,s,ms,us,ns) denote the unit, which is an integer or float number. This will be based off the origin. Example, with unit=’ms’ and origin=’unix’ (the default), this would calculate the number of milliseconds to the unix epoch start.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;参数的单位（D,s,ms,us,ns）表示数值的单位，可为整数或浮点数。该值基于参考日期计算。例如，当 unit=’ms’ 且 origin=’unix’（默认）时，将计算距离 Unix 纪元起点的毫秒数。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="-49"/>
        <source>unit</source>
        <translation>单位</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>D</source>
        <translation>D</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>s</source>
        <translation>s</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>ms</source>
        <translation>ms</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>us</source>
        <translation>us</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>ns</source>
        <translation>ns</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>parse set:</source>
        <translation>解析设置:</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Specify a date parse order&lt;/p&gt;&lt;p&gt;If True, parses dates with the day first, eg 10/11/12 is parsed as 2012-11-10. &lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600; color:#afaf00;&quot;&gt;Warning&lt;/span&gt;:&lt;span style=&quot; font-style:italic;&quot;&gt; dayfirst=True is not strict, but will prefer to parse with day first (this is a known bug, based on dateutil behavior)&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;指定日期解析顺序&lt;/p&gt;&lt;p&gt;若为 True，优先按日解析，例如 10/11/12 将解析为 2012-11-10。&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600; color:#afaf00;&quot;&gt;警告&lt;/span&gt;:&lt;span style=&quot; font-style:italic;&quot;&gt; dayfirst=True 并非严格规则，只是优先按日解析（这是基于 dateutil 行为的已知问题）。&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>day first</source>
        <translation>日优先</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Specify a date parse order&lt;/p&gt;&lt;p&gt;- If True parses dates with the year first, eg 10/11/12 is parsed as 2010-11-12.&lt;/p&gt;&lt;p&gt;- If both dayfirst and yearfirst are True, yearfirst is preceded (same as dateutil).&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600; color:#b6a80b;&quot;&gt;Warning&lt;/span&gt;:&lt;span style=&quot; font-style:italic;&quot;&gt; yearfirst=True is not strict, but will prefer to parse with year first (this is a known bug, based on dateutil behavior).&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;指定日期解析顺序&lt;/p&gt;&lt;p&gt;- 若为 True，优先按年解析，例如 10/11/12 将解析为 2010-11-12。&lt;/p&gt;&lt;p&gt;- 若 dayfirst 和 yearfirst 同时为 True，yearfirst 优先（与 dateutil 一致）。&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600; color:#b6a80b;&quot;&gt;警告&lt;/span&gt;:&lt;span style=&quot; font-style:italic;&quot;&gt; yearfirst=True 并非严格规则，只是优先按年解析（这是基于 dateutil 行为的已知问题）。&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>year first</source>
        <translation>年优先</translation>
    </message>
    <message>
        <location line="+10"/>
        <location line="+15"/>
        <source>utc</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location line="-3"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Return UTC DatetimeIndex if True (converting any tz-aware datetime.datetime objects as well).&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;若为 True，返回 UTC DatetimeIndex（同时转换带时区的 datetime.datetime 对象）。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If True, use a cache of unique, converted dates to apply the datetime conversion. May produce significant speed-up when parsing duplicate date strings, especially ones with timezone offsets.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;若为 True，使用唯一已转换日期的缓存来执行日期时间转换。在解析重复日期字符串（尤其是带时区偏移的）时可显著提升速度。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>cache</source>
        <translation>缓存</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>DADialogDataframeColumnCastToNumeric</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogDataframeColumnCastToNumeric.ui" line="+14"/>
        <source>Cast To Numeric</source>
        <translation>转换为数值</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>errors:</source>
        <translation>错误处理:</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>invalid parsing will raise an exception</source>
        <translation>无效解析将抛出异常</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>invalid parsing will be set as NaN</source>
        <translation>无效解析将设为 NaN</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>invalid parsing will return the input</source>
        <translation>无效解析将返回原输入</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>downcast:</source>
        <translation>向下转换:</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+13"/>
        <location line="+10"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;smallest signed int dtype (min.: np.int8)&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;最小的有符号整数类型（最小为 np.int8）&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="-7"/>
        <source>integer</source>
        <translation>整数</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>signed</source>
        <translation>有符号</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;smallest unsigned int dtype (min.: np.uint8)&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;最小的无符号整数类型（最小为 np.uint8）&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>unsigned</source>
        <translation>无符号</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;smallest float dtype (min.: np.float32)&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;最小的浮点数类型（最小为 np.float32）&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>float</source>
        <translation>浮点数</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>DADialogDataframeColumnDescribe</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogDataframeColumnDescribe.ui" line="+14"/>
        <source>Column Statistics</source>
        <translation>列统计</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Column:</source>
        <translation>列：</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Type:</source>
        <translation>类型：</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Statistic</source>
        <translation>统计量</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Value</source>
        <translation>值</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Close</source>
        <translation>收盘</translation>
    </message>
</context>
<context>
    <name>DADialogInsertNewColumn</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogInsertNewColumn.ui" line="+14"/>
        <source>Insert New Column</source>
        <translation>插入新列</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>dtype</source>
        <translation>数据类型</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Fill Setting</source>
        <translation>填充设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+20"/>
        <source>Fill in the same value</source>
        <translation>填充相同值</translation>
    </message>
    <message>
        <location line="-10"/>
        <location line="+29"/>
        <source>Generate growth value</source>
        <translation>生成递增值</translation>
    </message>
    <message>
        <location line="-13"/>
        <source>default value</source>
        <translation>默认值</translation>
    </message>
    <message>
        <location line="+26"/>
        <location line="+24"/>
        <source>start</source>
        <translation>起始</translation>
    </message>
    <message>
        <location line="-14"/>
        <location line="+34"/>
        <source>stop</source>
        <translation>结束</translation>
    </message>
    <message>
        <location line="-7"/>
        <location line="+20"/>
        <source>yyyy-MM-dd HH:mm:ss</source>
        <translation>yyyy-MM-dd HH:mm:ss</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>DADialogStatsChartGuide</name>
    <message>
        <location filename="../DAGui/Dialog/DADialogStatsChartGuide.ui" line="+14"/>
        <source>Statistics Chart Guide</source>
        <translation>统计图表向导</translation>
    </message>
</context>
<context>
    <name>DAExportToPngSettingDialog</name>
    <message>
        <location filename="../APP/Dialog/DAExportToPngSettingDialog.ui" line="+14"/>
        <source>Export PNG</source>
        <translation>导出PNG</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>custom</source>
        <translation>自定义</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>screen</source>
        <translation>屏幕</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>print</source>
        <translation>打印</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DPI</source>
        <translation>DPI</translation>
    </message>
    <message>
        <location line="+56"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Export</source>
        <translation>导出</translation>
    </message>
</context>
<context>
    <name>DAFigureWidget</name>
    <message>
        <location filename="../DAFigure/DAFigureWidget.cpp" line="-1248"/>
        <source>Figure</source>
        <translation>绘图</translation>
    </message>
</context>
<context>
    <name>DAGraphicsPixmapItemSettingWidget</name>
    <message>
        <location filename="../DAGui/DAGraphicsPixmapItemSettingWidget.ui" line="+14"/>
        <source>Pixmap Item Setting</source>
        <translation>图像图元设置</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Alpha</source>
        <translation>透明度</translation>
    </message>
</context>
<context>
    <name>DAMessageLogViewWidget</name>
    <message>
        <location filename="../DAGui/DAMessageLogViewWidget.ui" line="+17"/>
        <source>Message View</source>
        <translation>消息视图</translation>
    </message>
    <message>
        <location line="+46"/>
        <location line="+20"/>
        <source>...</source>
        <translation>...</translation>
    </message>
</context>
<context>
    <name>DANodeItemSettingWidget</name>
    <message>
        <location filename="../DAGui/DANodeItemSettingWidget.ui" line="+14"/>
        <source>Node Item Setting</source>
        <translation>节点图元设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Width</source>
        <translation>宽度</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Height</source>
        <translation>高度</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Lock Aspect Ratio</source>
        <translation>锁定宽高比</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>rotation</source>
        <translation>旋转</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Position</source>
        <translation>位置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>x</source>
        <translation>x</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>y</source>
        <translation>y</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Link Point Location</source>
        <translation>连接点位置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Input Location</source>
        <translation>输入位置</translation>
    </message>
    <message>
        <location line="+199"/>
        <source>Output Location</source>
        <translation>输出位置</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Property</source>
        <translation>属性</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>movable</source>
        <translation>可移动</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>resizable</source>
        <translation>可缩放</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>tooltip</source>
        <translation>工具提示</translation>
    </message>
</context>
<context>
    <name>DANodeLinkItemSettingWidget</name>
    <message>
        <location filename="../DAGui/DANodeLinkItemSettingWidget.ui" line="+14"/>
        <source>Node Link Item Setting</source>
        <translation>节点连线图元设置</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>front style</source>
        <translation>前端点样式</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>pen:</source>
        <translation>画笔:</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>link style:</source>
        <translation>连线样式:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>end style</source>
        <translation>后端点样式</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>end point size</source>
        <translation>端点大小</translation>
    </message>
</context>
<context>
    <name>DANodeSettingWidget</name>
    <message>
        <location filename="../DAGui/DANodeSettingWidget.ui" line="+14"/>
        <source>Node Setting</source>
        <translation>节点设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Meta Data</source>
        <translation>元数据</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Prototype</source>
        <translation>原型</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Group</source>
        <translation>分组</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Name:</source>
        <translation>名称:</translation>
    </message>
</context>
<context>
    <name>DAPluginManagerDialog</name>
    <message>
        <location filename="../APP/DAPluginManagerDialog.ui" line="+14"/>
        <source>Plugin Manager</source>
        <translation>插件管理</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>ok</source>
        <translation>确认</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>cannel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>DAPyDType</name>
    <message>
        <location filename="../DAPyBindQt/numpy/DAPyDType.cpp" line="+652"/>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>string (nullable)</source>
        <translation>string（可空）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>boolean (nullable)</source>
        <translation>boolean（可空）</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+3"/>
        <source>(nullable)</source>
        <translation>（可空）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>category</source>
        <translation>category</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>datetime (with timezone)</source>
        <translation>datetime（带时区）</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>period</source>
        <translation>period</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>interval</source>
        <translation>interval</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Arrow</source>
        <translation>Arrow</translation>
    </message>
</context>
<context>
    <name>DAPyWorkFlowEditWidget</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowEditWidget.ui" line="+20"/>
        <source>Workflow Edit</source>
        <translation>工作流编辑</translation>
    </message>
</context>
<context>
    <name>DAPyWorkFlowNodeItemSettingWidget</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowNodeItemSettingWidget.ui" line="+14"/>
        <source>Node Setting</source>
        <translation>节点设置</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>Node</source>
        <translation>节点</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Item</source>
        <translation>图元</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Link</source>
        <translation>连线</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Picture</source>
        <translation>图片</translation>
    </message>
</context>
<context>
    <name>DAPyWorkFlowNodeListWidget</name>
    <message>
        <location filename="../DAGui/DAPyWorkFlowNodeListWidget.ui" line="+23"/>
        <source>Node List</source>
        <translation>节点列表</translation>
    </message>
</context>
<context>
    <name>DAPyWorkFlowSceneSerializer</name>
    <message>
        <location filename="../DAPyWorkFlow/DAPyWorkFlowSceneSerializer.cpp" line="+105"/>
        <source>scene or doc pointer is null</source>
        <translation>scene或doc指针为空</translation>
    </message>
    <message>
        <location line="+93"/>
        <source>sceneElement or scene pointer is null</source>
        <translation>sceneElement或scene指针为空</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Manager or workflow is invalid</source>
        <translation>Manager或workflow无效</translation>
    </message>
    <message>
        <location line="+93"/>
        <source>Cannot open file for writing: %1</source>
        <translation>无法打开文件写入: %1</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Cannot open file for reading: %1</source>
        <translation>无法打开文件读取: %1</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>XML parse error: %1 (line:%2 col:%3)</source>
        <translation>XML解析错误: %1 (行:%2 列:%3)</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>XML document has no root element</source>
        <translation>XML文档无根元素</translation>
    </message>
</context>
<context>
    <name>DARenameColumnsNameDialog</name>
    <message>
        <location filename="../DAGui/Dialog/DARenameColumnsNameDialog.ui" line="+14"/>
        <source>Rename Table</source>
        <translation>重命名表</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Table Name:</source>
        <translation>表名:</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>DASettingContainerWidget</name>
    <message>
        <location filename="../DAGui/DASettingContainerWidget.ui" line="+20"/>
        <source>Setting</source>
        <translation>设置</translation>
    </message>
</context>
<context>
    <name>DASettingDialog</name>
    <message>
        <location filename="../DAGui/DASettingDialog.ui" line="+14"/>
        <source>Setting</source>
        <translation>设置</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Apply</source>
        <translation>应用</translation>
    </message>
</context>
<context>
    <name>DASettingPageAdvanced</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPageAdvanced.ui" line="+14"/>
        <source>Advanced Setting</source>
        <translation>高级设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Behavior</source>
        <translation>行为</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Workflow execution timeout</source>
        <translation>工作流执行超时</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Max recent files</source>
        <translation>最大最近文件数</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Crash dump retention days</source>
        <translation>崩溃转储保留天数</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Auto-save interval</source>
        <translation>自动保存间隔</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Show splash screen on startup</source>
        <translation>启动时显示启动画面</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Plugin search path</source>
        <translation>插件搜索路径</translation>
    </message>
    <message>
        <location line="+9"/>
        <location line="+69"/>
        <source>Browse</source>
        <translation>浏览</translation>
    </message>
    <message>
        <location line="-59"/>
        <source>Node script search paths</source>
        <translation>节点脚本搜索路径</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Add</source>
        <translation>添加</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Remove</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Script workspace</source>
        <translation>脚本工作区</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Workspace directory</source>
        <translation>工作区目录</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Script execution timeout</source>
        <translation>脚本执行超时</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Script result max characters</source>
        <translation>脚本结果最大字符数</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Note: plugin path, node script paths and splash changes take effect after restarting the program; script workspace directory takes effect when the project is next opened</source>
        <translation>注意：插件路径、节点脚本路径和启动画面的修改在重启程序后生效；脚本工作区目录在下次打开工程时生效</translation>
    </message>
</context>
<context>
    <name>DASettingPageGeneral</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPageGeneral.ui" line="+14"/>
        <source>General Setting</source>
        <translation>通用设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Appearance</source>
        <translation>外观</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Standard Mode</source>
        <translation>标准模式</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Standard Mode With 2 Row</source>
        <translation>2行标准模式</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Compact Mode</source>
        <translation>紧凑模式</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Minimalist mode</source>
        <translation>最小模式</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Ribbon theme</source>
        <translation>Ribbon 主题</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Application font</source>
        <translation>应用程序字体</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Is the geometry and status of the program interface recorded</source>
        <translation>是否在程序退出的时候记录程序的界面布局和位置</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Clear the saved window state</source>
        <translation>删除窗口状态记录文件</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Language</source>
        <translation>语言</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Interface language</source>
        <translation>界面语言</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Note: language and font changes take effect after restarting the program</source>
        <translation>注意：语言和字体更改在重启程序后生效</translation>
    </message>
</context>
<context>
    <name>DASettingPageLog</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPageLog.ui" line="+14"/>
        <source>Log Setting</source>
        <translation>日志设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Log Level</source>
        <translation>日志级别</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>File log level</source>
        <translation>文件日志级别</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>UI queue level</source>
        <translation>UI 队列级别</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Output log to stdout</source>
        <translation>输出日志到标准输出</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Log File</source>
        <translation>日志文件</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Rotation mode</source>
        <translation>轮转模式</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Max size per file</source>
        <translation>单文件最大大小</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Max files to keep</source>
        <translation>最大保留文件数</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>UI Display</source>
        <translation>界面显示</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Display number of logs</source>
        <translation>显示的日志条数</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Note: file level, stdout and rotation changes take effect after restarting the program</source>
        <translation>注意：文件级别、标准输出和轮转更改在重启程序后生效</translation>
    </message>
</context>
<context>
    <name>DASettingPagePython</name>
    <message>
        <location filename="../APP/SettingPages/DASettingPagePython.ui" line="+14"/>
        <source>Python Setting</source>
        <translation>Python 设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Python Interpreter</source>
        <translation>Python 解释器</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Interpreter path</source>
        <translation>解释器路径</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Browse</source>
        <translation>浏览</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Auto Detect</source>
        <translation>自动检测</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Test</source>
        <translation>测试</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Extra module search paths (sys.path)</source>
        <translation>额外模块搜索路径 (sys.path)</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Add</source>
        <translation>添加</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Remove</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Note: interpreter and module path changes take effect after restarting the program</source>
        <translation>注意：解释器和模块路径更改在重启程序后生效</translation>
    </message>
</context>
<context>
    <name>DASettingWidget</name>
    <message>
        <location filename="../DAGui/DASettingWidget.ui" line="+14"/>
        <source>Setting</source>
        <translation>设置</translation>
    </message>
</context>
<context>
    <name>DATxtFileImportDialog</name>
    <message>
        <location filename="../DAGui/Dialog/DATxtFileImportDialog.ui" line="+14"/>
        <source>Txt Import</source>
        <translation>Txt 导入</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Text File Path</source>
        <translation>文本文件路径</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>The maximum number of rows to read</source>
        <translation>读取的最大行数</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>The number of lines to skip at the end of the file</source>
        <translation>文件末尾跳过的行数</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Auto</source>
        <translation>自动</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>max rows</source>
        <translation>最大行数</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>The number of lines to skip at the beginning of the file</source>
        <translation>文件开头跳过的行数</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>delimiter</source>
        <translation>分隔符</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>skip rows</source>
        <translation>跳过行数</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>encoding</source>
        <translation>编码</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Character or regex pattern to treat as the delimiter&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;作为分隔符的字符或正则表达式模式&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>skip over blank lines rather than interpreting as NaN values</source>
        <translation>跳过空行而不是将其解析为 NaN 值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>skip blank lines</source>
        <translation>跳过空行</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>skip footer</source>
        <translation>跳过末尾</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+13"/>
        <source>Row number(s) containing column labels and marking the start of the data</source>
        <translation>包含列标签并标记数据起点的行号</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>header row</source>
        <translation>表头行</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>No Error</source>
        <translation>无错误</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Preview</source>
        <translation>预览</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Roboto&apos;,&apos;Ubuntu&apos;,&apos;Helvetica Neue&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;PingFang SC&apos;,&apos;Hiragino Sans GB&apos;,&apos;Microsoft YaHei UI&apos;,&apos;Microsoft YaHei&apos;,&apos;Source Han Sans CN&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:15px; color:#05073b; background-color:#fdfdfe;&quot;&gt;In order to avoid the interface stalling due to loading of large texts, the maximum number of words in this preview is limited to 100,000 characters&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-family:&apos;-apple-system&apos;,&apos;BlinkMacSystemFont&apos;,&apos;Segoe UI&apos;,&apos;Roboto&apos;,&apos;Ubuntu&apos;,&apos;Helvetica Neue&apos;,&apos;Helvetica&apos;,&apos;Arial&apos;,&apos;PingFang SC&apos;,&apos;Hiragino Sans GB&apos;,&apos;Microsoft YaHei UI&apos;,&apos;Microsoft YaHei&apos;,&apos;Source Han Sans CN&apos;,&apos;sans-serif&apos;,&apos;Apple Color Emoji&apos;,&apos;Segoe UI Emoji&apos;; font-size:15px; color:#05073b; background-color:#fdfdfe;&quot;&gt;为避免加载大段文本导致界面卡顿，此预览的最大字数限制为 100,000 字符。&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>preview max row:</source>
        <translation>预览最大行数:</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>Text</source>
        <translation>文本</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
</context>
<context>
    <name>DAWorkbenchAboutDialog</name>
    <message>
        <location filename="../APP/Dialog/DAWorkbenchAboutDialog.ui" line="+20"/>
        <source>About DAWorkbench</source>
        <translation>关于 DAWorkbench</translation>
    </message>
    <message>
        <location line="+159"/>
        <source>OK</source>
        <translation>确认</translation>
    </message>
</context>
<context>
    <name>DataAnalysisUI</name>
    <message>
        <location filename="../../plugins/DataAnalysis/DataAnalysisUI.cpp" line="+87"/>
        <location line="+87"/>
        <source>Data Cleaning</source>
        <translation>数据清洗</translation>
    </message>
    <message>
        <location line="-54"/>
        <location line="+83"/>
        <source>Data Filtering</source>
        <translation>数据过滤</translation>
    </message>
    <message>
        <location line="-62"/>
        <location line="+77"/>
        <source>Statistics</source>
        <translation>数据统计</translation>
    </message>
    <message>
        <location line="-58"/>
        <source>Export 
Individual Data</source>
        <translation>导出
单个数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export 
Multiple Data</source>
        <translation>导出
多个数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Export all data from the data management area to a folder, with each dataset saved as an individual data file.</source>
        <translation>把数据管理区所有数据导出到一个文件夹中，每个数据形成一个数据文件</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Export 
To Excel</source>
        <translation>导出Excel</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Export all data from the data management area to an Excel file, with each dataset as a separate sheet.</source>
        <translation>把数据管理区所有数据导出到一个excel文件中，每个数据将作为excel的一个sheet</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Drop None</source>
        <translation>删除
缺失值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Drop rows which contain missing values</source>
        <translation>删除包含缺失值的行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Drop Duplicates</source>
        <translation>删除
重复值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Drop duplicate data</source>
        <translation>删除数据中的重复记录</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Fill rows which contain missing values by interpolation</source>
        <translation>插值法填充包含缺失值的行</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>The Z-Score outlier replacement method is a parametric approach based on the normal distribution assumption. It identifies outliers by quantifying how many standard deviations a data point deviates from the mean, and replaces outliers with reasonable strategies to preserve data integrity.</source>
        <translation>Z-Score（标准化分数）异常值替换方法是一种基于正态分布假设的参数化方法，通过量化数据点偏离均值的标准差倍数识别异常值，并采用合理策略替换异常值以保留数据完整性。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Transform Skewed</source>
        <translation>转换偏态数据</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Eval Data</source>
        <translation>列运算</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Query Data</source>
        <translation>条件筛选</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Data Description</source>
        <translation>数据描述</translation>
    </message>
    <message>
        <location line="-40"/>
        <source>Fill None</source>
        <translation>填充
缺失值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Fill rows which contain missing values</source>
        <translation>填充包含缺失值的行</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Fill Interpolate</source>
        <translation>插值填充</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>IQR Outlier Handling</source>
        <translation>IQR
异常值处理</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The IQR (Interquartile Range) outlier handling method is a non-parametric approach based on data distribution. It identifies extreme values deviating from the overall distribution using the interquartile range, unaffected by outliers themselves and featuring strong stability.</source>
        <translation>IQR（四分位距）异常值处理是一种基于数据分布的非参数方法，核心逻辑是通过数据的四分位数范围识别偏离整体分布的极端值，不受异常值本身影响，稳定性强。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Z-Score Outlier Handling</source>
        <translation>Z-Score
异常值处理</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Transform skewed numerical data to improve distribution</source>
        <translation>转换偏态数值数据以改善分布</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Evaluate a string describing operations on DataFrame columns</source>
        <translation>输入关于列操作的表达式字符串并执行计算</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Filter data outside of the given criteria</source>
        <translation>过滤给定条件外的数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data Retrieval</source>
        <translation>数据检索</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Retrieve data for a certain condition</source>
        <translation>检索某条件的数据</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Filter by Column</source>
        <translation>列数据过滤</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sort</source>
        <translation>数据排序</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sort Data</source>
        <translation>对数据进行排序</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Pivot Table</source>
        <translation>数据
透视表</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Create Pivot Table</source>
        <translation>创建数据透视表</translation>
    </message>
</context>
<context>
    <name>DataFrameCreatePivotTableDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameCreatePivotTableDialog.ui" line="+14"/>
        <source>Pivot Table Guide</source>
        <translation>数据透视表向导</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Pivot Table</source>
        <translation>数据
透视表</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Aggregate function</source>
        <translation>聚合函数</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Special All columns and rows will be added with partial group aggregates across the categories on the rows and columns</source>
        <translation>特殊：所有列和行都会添加按行/列类别的部分分组聚合值</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Margins</source>
        <translation>边缘汇总</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Specifies if the result should be sorted.</source>
        <translation>指定结果是否需要排序。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Sort</source>
        <translation>数据排序</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Name of the row / column that will contain the totals</source>
        <translation>包含合计的行/列名称</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>All</source>
        <translation>全部</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameCreatePivotTableDialog.cpp" line="+77"/>
        <source>Value</source>
        <translation>值</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Index</source>
        <translation>索引</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Columns</source>
        <translation>值、索引、列</translation>
    </message>
    <message>
        <location line="+134"/>
        <source>mean</source>
        <translation>均值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>sum</source>
        <translation>求和</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>count</source>
        <translation>计数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>size</source>
        <translation>大小</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>min</source>
        <translation>最小值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>max</source>
        <translation>最大值</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>median</source>
        <translation>中位数</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>std</source>
        <translation>标准差</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>var</source>
        <translation>方差</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>first</source>
        <translation>第一个</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>last</source>
        <translation>最后一个</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>prod</source>
        <translation>乘积</translation>
    </message>
</context>
<context>
    <name>DataFrameDataSearchDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameDataSearchDialog.ui" line="+14"/>
        <source>Dataframe Search</source>
        <translation>DataFrame 搜索</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Seacrch</source>
        <translation>搜索</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>From Begin</source>
        <translation>从头开始</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Find item:</source>
        <translation>查找内容:</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Next</source>
        <translation>下一个</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Close</source>
        <translation>收盘</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameDataSearchDialog.cpp" line="+59"/>
        <source>Cannot find item</source>
        <translation>无法找到条目</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Found at column %1, line %2</source>
        <translation>在第%2行、第%1列找到</translation>
    </message>
</context>
<context>
    <name>DataFrameDataSelectDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameDataSelectDialog.ui" line="+14"/>
        <location line="+6"/>
        <source>Data Filter</source>
        <translation>数据筛选</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Data</source>
        <translation>数据</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Range</source>
        <translation>范围</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameDataSelectDialog.cpp" line="+75"/>
        <location line="+24"/>
        <source>The current input cannot be converted to a floating-point number.</source>
        <translation>当前输入内容无法转换为浮点数</translation>
    </message>
</context>
<context>
    <name>DataFrameEvalDatasDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameEvalDatasDialog.ui" line="+14"/>
        <source>Eval Data</source>
        <translation>列运算</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Enter an expression, for example: new_col = age * 2. Column names containing spaces or punctuations (besides underscores) or starting with digits must be surrounded by backticks. (For example, a column named “Area (cm^2)” would be referenced as `Area (cm^2)`). Column names which are Python keywords (like “list”, “for”, “import”, etc) cannot be used. For example, if one of your columns is called a a and you want to sum it with b, your eval should be `a a` + b.</source>
        <translation>输入一个表达式，例如：new_col = age * 2。包含空格或下划线以外标点、或以数字开头的列名必须用反引号包围。（例如名为 “Area (cm^2)” 的列应写为 `Area (cm^2)`）。Python 关键字（如 “list”、“for”、“import” 等）不能作为列名使用。例如，如果某列名为 a a，想与 b 相加，eval 表达式应写为 `a a` + b。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>The string to evaluate</source>
        <translation>要求值的字符串</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Explanation：</source>
        <translation>说明：</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameEvalDatasDialog.cpp" line="+15"/>
        <source># I. Basic Syntax

You can write expressions using the following elements:

- **Column names**: Use column names directly in calculations (e.g., `age`, `salary`)
- **Constants**: Numbers, strings, and boolean values (e.g., `10`, `&quot;male&quot;`, `True`)
- **Operators**:
  - Mathematical operations: `+`, `-`, `*`, `/`, `**` (power), `%` (modulus)
  - Comparison operations: `==`, `!=`, `&gt;`, `&lt;`, `&gt;=`, `&lt;=`
  - Logical operations: `and`, `or`, `not`
- **Function calls** (partially supported):
  - Common math functions: `abs()`, `sin()`, `cos()`, `log()`, `exp()`, etc.
  - Conditional logic: `where(condition, x, y)`
  - String operations: `str.contains()`, `str.startswith()`, etc. (to be used with columns)

| Goal | Example Expression |
|------|--------------------|
| Add a new column | `new_col = col1 + col2` |
| Modify an existing column | `col = col * 2` |
| Conditional assignment | `col = where(col &gt; 10, 1, 0)` |
| Filter rows (returns boolean) | `col1 &gt; 5 and col2 &lt; 10` |

---

## Example 1: Add or Modify a Column

```python
age + 10
```

This adds 10 to each value in the `age` column and either updates the original column or writes to a new column.

---

## Example 2: Create a New Column and Assign Values

```python
new_column = salary * 1.1
```

This creates a new column named `new_column`, whose values are 1.1 times those of the `salary` column.

---

## Example 3: Conditional Filtering and Assignment

```python
bonus = where(age &gt; 30, salary * 0.2, salary * 0.1)
```

This means: if age is greater than 30, the bonus is 20% of the salary; otherwise, it&apos;s 10%.

---

## Example 4: String Matching (for filtering)

```python
name.str.contains(&quot;John&quot;)
```

This can be used to filter rows where the name contains &quot;John&quot;.</source>
        <translation># 一、基本语法

可以使用以下元素编写表达式：

- **列名**：直接在计算中使用列名（如 `age`、`salary`）
- **常量**：数字、字符串和布尔值（如 `10`、`&quot;male&quot;`、`True`）
- **运算符**：
  - 数学运算：`+`、`-`、`*`、`/`、`**`（幂）、`%`（取模）
  - 比较运算：`==`、`!=`、`&gt;`、`&lt;`、`&gt;=`、`&lt;=`
  - 逻辑运算：`and`、`or`、`not`
- **函数调用**（部分支持）：
  - 常用数学函数：`abs()`、`sin()`、`cos()`、`log()`、`exp()` 等
  - 条件逻辑：`where(condition, x, y)`
  - 字符串操作：`str.contains()`、`str.startswith()` 等（需配合列使用）

| 目标 | 表达式示例 |
|------|--------------------|
| 新增列 | `new_col = col1 + col2` |
| 修改现有列 | `col = col * 2` |
| 条件赋值 | `col = where(col &gt; 10, 1, 0)` |
| 筛选行（返回布尔值） | `col1 &gt; 5 and col2 &lt; 10` |

---

## 示例 1：新增或修改列

```python
age + 10
```

将 `age` 列的每个值加 10，结果可更新原列或写入新列。

---

## 示例 2：创建新列并赋值

```python
new_column = salary * 1.1
```

创建名为 `new_column` 的新列，其值为 `salary` 列的 1.1 倍。

---

## 示例 3：条件筛选与赋值

```python
bonus = where(age &gt; 30, salary * 0.2, salary * 0.1)
```

含义：若 age 大于 30，则 bonus 为 salary 的 20%；否则为 10%。

---

## 示例 4：字符串匹配（用于筛选）

```python
name.str.contains(&quot;John&quot;)
```

可用于筛选 name 中包含 &quot;John&quot; 的行。</translation>
    </message>
</context>
<context>
    <name>DataFrameExportRangeSelectDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameExportRangeSelectDialog.ui" line="+14"/>
        <source>Export Setting</source>
        <translation>导出设置</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Select Export Range</source>
        <translation>选择导出范围</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Export All</source>
        <translation>导出全部</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Export Selected</source>
        <translation>导出所选</translation>
    </message>
</context>
<context>
    <name>DataFrameQueryDatasDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameQueryDatasDialog.ui" line="+14"/>
        <source>Query Data</source>
        <translation>条件筛选</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>You can refer to column names that are not valid Python variable names by surrounding them in backticks. Column names containing spaces or punctuations (besides underscores) or starting with digits must be surrounded by backticks. (For example, a column named “Area (cm^2)” would be referenced as `Area (cm^2)`). Column names which are Python keywords (like “list”, “for”, “import”, etc) cannot be used. For example, if one of your columns is called a a and you want to compare it with b, your query should be `a a` &gt; b.</source>
        <translation>可以用反引号包围不合法的 Python 列名。包含空格或下划线以外标点、或以数字开头的列名必须用反引号包围。（例如名为 “Area (cm^2)” 的列应写为 `Area (cm^2)`）。Python 关键字（如 “list”、“for”、“import” 等）不能作为列名使用。例如，如果某列名为 a a，想与 b 比较，query 表达式应写为 `a a` &gt; b。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>The query string to evaluate</source>
        <translation>要求值的查询字符串</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Explanation：</source>
        <translation>说明：</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameQueryDatasDialog.cpp" line="+15"/>
        <source>Using the **Query Data** feature, you can filter data using expressions:

1  **Comparison Operators**: Supports `==`, `&gt;`, `&lt;`, `&gt;=`, `&lt;=`, `!=` for direct comparison of column names and values.  
   **Example**:  
   `A &gt; 2 &amp; B &lt; 8` filters rows where the value in column **A** is greater than **2** and the value in column **B** is less than **8**.

2  **Inter-Column Comparisons**: Directly compare values between columns.  
   **Example**:  
   `A &gt; B` filters rows where the value in column **A** is greater than the value in column **B**.

3  **Logical Operators**: Supports `and`, `or`, `not`, `in`, and `not in` for simplified multi-condition filtering.
   **Examples**:  
   - `A &gt; 2 and B &lt; 8` filters rows where **A** &gt; 2 and **B** &lt; 8.  
   - `A in (&quot;S&quot;, &quot;C&quot;)` filters rows where **A** is either &quot;S&quot; or &quot;C&quot;.

4  **Arithmetic and Complex Logic**: Allows arithmetic operations and complex logical expressions.  
   **Example**:  
   `(A * 3 &gt; 1) | ((B + 12.5) &lt; 5)`.

5  **Range Filtering with `between`**: Use `between` to filter numeric ranges.  
   **Example**:  
   `A.between(2, 8)` filters values in column **A** between **2** and **8**.

6  **String Operations with `str` Methods**: Supports string column processing (e.g., length, prefix matching).  
   **Example**:  
   `Ticket.str.startswith(&quot;A&quot;)` filters rows where the **Ticket** column starts with &quot;A&quot;.

**Note**:  
If a column name contains spaces or special characters, enclose it in backticks (`` ` ``), e.g., `` `Embarked On` ``.</source>
        <translation>使用 **数据查询** 功能，可以通过表达式筛选数据：

1  **比较运算符**：支持 `==`、`&gt;`、`&lt;`、`&gt;=`、`&lt;=`、`!=`，用于直接比较列名和值。  
   **示例**：  
   `A &gt; 2 &amp; B &lt; 8` 筛选 **A** 列值大于 **2** 且 **B** 列值小于 **8** 的行。

2  **列间比较**：直接比较不同列之间的值。  
   **示例**：  
   `A &gt; B` 筛选 **A** 列值大于 **B** 列值的行。

3  **逻辑运算符**：支持 `and`、`or`、`not`、`in`、`not in`，简化多条件筛选。  
   **示例**：  
   - `A &gt; 2 and B &lt; 8` 筛选 **A** &gt; 2 且 **B** &lt; 8 的行。  
   - `A in (&quot;S&quot;, &quot;C&quot;)` 筛选 **A** 为 &quot;S&quot; 或 &quot;C&quot; 的行。

4  **算术与复杂逻辑**：支持算术运算和复杂逻辑表达式。  
   **示例**：  
   `(A * 3 &gt; 1) | ((B + 12.5) &lt; 5)`。

5  **使用 `between` 进行范围筛选**：用 `between` 筛选数值范围。  
   **示例**：  
   `A.between(2, 8)` 筛选 **A** 列值介于 **2** 和 **8** 之间的行。

6  **使用 `str` 方法的字符串操作**：支持字符串列处理（如长度、前缀匹配）。  
   **示例**：  
   `Ticket.str.startswith(&quot;A&quot;)` 筛选 **Ticket** 列以 &quot;A&quot; 开头的行。

**注意**：  
若列名包含空格或特殊字符，需用反引号包围（`` ` ``），如 `` `Embarked On` ``。</translation>
    </message>
</context>
<context>
    <name>DataFrameSortDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameSortDialog.ui" line="+14"/>
        <source>Sort</source>
        <translation>数据排序</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataFrameSortDialog.cpp" line="+13"/>
        <source>Ascending</source>
        <translation>升序</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Descending</source>
        <translation>降序</translation>
    </message>
</context>
<context>
    <name>DataframeExportSettingsDialog</name>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataframeExportSettingsDialog.ui" line="+14"/>
        <source>Export Data Setting</source>
        <translation>导出数据设置</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Select Folder To Export</source>
        <translation>选择导出文件夹</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Export All</source>
        <translation>导出全部</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Export Selected</source>
        <translation>导出所选</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Feather</source>
        <translation>Feather</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Comma-separated values - Universal text format. Best for data exchange and basic analysis.</source>
        <translation>逗号分隔值 - 通用文本格式。最适合数据交换和基础分析。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Column-oriented binary format. High compression and fast querying. Perfect for big data and analytics.</source>
        <translation>列式二进制格式。高压缩比，查询速度快。适合大数据和分析场景。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>JSON</source>
        <translation>JSON</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Microsoft Excel format. Supports multiple sheets and formatting.</source>
        <translation>Microsoft Excel 格式。支持多表和格式设置。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Python-specific binary format. Preserves complete object structure. Best for temporary storage within Python applications.</source>
        <translation>Python 专用二进制格式。保留完整的对象结构。最适合在 Python 应用内临时存储。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Lightweight binary format. Extremely fast read/write speeds. Great for intermediate data storage and Python/R interoperability.</source>
        <translation>轻量级二进制格式。读写速度极快。适合中间数据存储以及 Python/R 互操作。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Pickle</source>
        <translation>Pickle</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>HTML</source>
        <translation>HTML</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>csv</source>
        <translation>csv</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>xlsx</source>
        <translation>xlsx</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Parquet</source>
        <translation>Parquet</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>JavaScript Object Notation. Human-readable, web-friendly format. Excellent for web APIs and configuration files.</source>
        <translation>JavaScript 对象表示法。人类可读、Web 友好的格式。非常适合 Web API 和配置文件。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Web page format. Preserves table styling. Ideal for embedding data in reports or emails.</source>
        <translation>网页格式。保留表格样式。适合在报告或邮件中嵌入数据。</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Dialogs/DataframeExportSettingsDialog.cpp" line="+74"/>
        <source>Select Folder</source>
        <translation>选择文件夹</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Please select the folder for exporting data</source>
        <translation>请选择需要导出数据的文件夹</translation>
    </message>
</context>
<context>
    <name>DataframeIOWorker</name>
    <message>
        <location filename="../../plugins/DataAnalysis/DataframeIOWorker.cpp" line="+71"/>
        <source>No data is selected. Please select the data to export first.</source>
        <translation>没有选中任何数据，请先选中要导出的数据</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Export Data</source>
        <translation>导出数据</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Successfully exported %1 to %2</source>
        <translation>成功把%1导出到%2</translation>
    </message>
    <message>
        <location line="+47"/>
        <source>Excel</source>
        <translation>Excel</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Save as Excel File</source>
        <translation>保存为 excel 文件</translation>
    </message>
    <message>
        <location line="+80"/>
        <source>%1. Elapsed: %2:%3</source>
        <translation>%1,已用时%2:%3</translation>
    </message>
</context>
<context>
    <name>DataframeOperateWorker</name>
    <message>
        <location filename="../../plugins/DataAnalysis/DataframeOperateWorker.cpp" line="+68"/>
        <source>Please first open the data table to operate on.</source>
        <translation>请先打开要操作的数据表</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>This function only supports data in the pandas DataFrame format.</source>
        <translation>只支持dataframe格式数据</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>%1_Describe</source>
        <translation>%1_描述</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Generate descriptive statistics that summarize the central tendency, dispersion and shape of the [%1]’s distribution, excluding NaN values</source>
        <translation>生成描述性统计数据，总结[%1]分布的集中趋势、离散度和形状，排除NaN值</translation>
    </message>
    <message>
        <location line="+58"/>
        <source>%1_PivotTable</source>
        <translation>%1_数据透视表</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Generate a pivot table of %1</source>
        <translation>生成%1的数据透视表</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../APP/main.cpp" line="+109"/>
        <source>Failed to set console output codepage to UTF-8</source>
        <translation>设置控制台输出代码页为 UTF-8 失败</translation>
    </message>
    <message>
        <location line="+70"/>
        <source>Initializing...</source>
        <translation>正在初始化...</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Initializing core components...</source>
        <translation>正在初始化核心组件...</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Kernel initialization failed</source>
        <translation>内核初始化失败</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Loading user interface...</source>
        <translation>正在加载用户界面...</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Opening project...</source>
        <translation>正在打开工程...</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Importing data...</source>
        <translation>正在导入数据...</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Ready</source>
        <translation>启动完成</translation>
    </message>
    <message>
        <location line="+142"/>
        <source>Python interpreter path is %1</source>
        <translation>Python解释器路径为%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Python home path is %1</source>
        <translation>Python主目录路径为%1</translation>
    </message>
    <message>
        <location filename="../DAGui/Commands/DACommandsDataFrame.cpp" line="+20"/>
        <source>set dataframe data</source>
        <translation>改变单元格数据</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>insert row</source>
        <translation>插入一行</translation>
    </message>
    <message>
        <location line="+44"/>
        <location line="+27"/>
        <source>insert column &quot;%1&quot;</source>
        <translation>插入列“%1”</translation>
    </message>
    <message>
        <location line="+47"/>
        <source>drop dataframe rows</source>
        <translation>移除dataframe行</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>drop dataframe columns</source>
        <translation>移除dataframe列</translation>
    </message>
    <message>
        <location line="+66"/>
        <source>change column type</source>
        <translation>改变列数据类型</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>paste data</source>
        <translation>粘贴数据</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>eval data</source>
        <translation>列运算</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>cast column to number</source>
        <translation>转换列数据为数值</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>cast column to datetime</source>
        <translation>改变列数据为日期</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>set column to index</source>
        <translation>转换列为索引</translation>
    </message>
    <message>
        <location filename="../DAGui/Commands/DACommandsTableStyle.cpp" line="+16"/>
        <source>change table style</source>
        <translation>改变表格样式</translation>
    </message>
    <message>
        <location filename="../DAGui/DAXmlHelper.cpp" line="+199"/>
        <location line="+277"/>
        <source>error occurred while loading nodes</source>
        <translation>加载节点时发生错误</translation>
    </message>
    <message>
        <location line="-272"/>
        <location line="+275"/>
        <source>error occurred while loading node links</source>
        <translation>加载节点连线时发生错误</translation>
    </message>
    <message>
        <location line="-270"/>
        <source>error occurred while loading special items</source>
        <translation>加载特殊图元时发生错误</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>error occurred while loading scene info</source>
        <translation>加载场景信息时发生错误</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>loadNodesView: error occurred</source>
        <translation>加载节点视图时发生错误</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>loadNodeLinksView: error occurred</source>
        <translation>加载连线视图时发生错误</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>loadCommonItems: error occurred</source>
        <translation>加载通用图元时发生错误</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>loadSceneInfo: error occurred</source>
        <translation>加载场景信息时发生错误</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>loadNodesView: manager or workflow is not valid</source>
        <translation>加载节点视图：管理器或工作流无效</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>loadNodesView: node element missing id attribute</source>
        <translation>加载节点视图：节点元素缺少id属性</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>loadNodesView: node_id=%1 not found in Python workflow</source>
        <translation>加载节点视图：在Python工作流中未找到node_id=%1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>loadNodesView: wrapPyNode failed for node_id=%1</source>
        <translation>加载节点视图：为node_id=%1包装节点失败</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>loadNodeLinksView: cannot find nodes for link (from=%1, to=%2)</source>
        <translation>加载连线视图：无法找到连线对应的节点(from=%1, to=%2)</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>loadNodeLinksView: wrapPyNodeLink failed</source>
        <translation>加载连线视图：包装连线失败</translation>
    </message>
    <message>
        <location line="+8"/>
        <location line="+6"/>
        <location line="+654"/>
        <location line="+6"/>
        <location line="+85"/>
        <location line="+6"/>
        <source>link item failed to load from xml</source>
        <translation>连线图元从xml加载失败</translation>
    </message>
    <message>
        <location line="-676"/>
        <source>Load Nodes</source>
        <translation>加载节点</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>error occurred while loading items</source>
        <translation>加载图元时发生错误</translation>
    </message>
    <message>
        <location line="+179"/>
        <location line="+75"/>
        <source>node missing id attribute, will skip this node</source>
        <translation>节点缺少id属性，将跳过此节点</translation>
    </message>
    <message>
        <location line="-55"/>
        <location line="+75"/>
        <source>Unable to create node by prototype=%1,name=%2</source>
        <translation>无法通过原型=%1创建节点，名称=%2</translation>
    </message>
    <message>
        <location line="-69"/>
        <location line="+75"/>
        <source>Node item has no proxy, prototype=%1,name=%2</source>
        <translation>节点图元没有代理，原型=%1，名称=%2</translation>
    </message>
    <message>
        <location line="+106"/>
        <location line="+20"/>
        <source>node(prototype=%1,name=%2) %3 tag is missing child tag &lt;name&gt;</source>
        <translation>节点(原型=%1,名称=%2)的%3标签缺少子标签&lt;name&gt;</translation>
    </message>
    <message>
        <location line="+108"/>
        <source>link item failed to save to xml</source>
        <translation>链接线保存到xml失败</translation>
    </message>
    <message>
        <location line="+31"/>
        <location line="+9"/>
        <location line="+69"/>
        <location line="+21"/>
        <source>link info: cannot find node in scene, id = %1</source>
        <translation>连线信息：无法在场景中找到节点，id = %1</translation>
    </message>
    <message>
        <location line="-83"/>
        <location line="+91"/>
        <source>Unable to link to node %3&apos;s link point %4 through link point %2 of node %1</source>
        <translation>节点%1无法通过连接点%2链接到节点%3的连接点%4</translation>
    </message>
    <message>
        <location line="-39"/>
        <location line="+21"/>
        <source>During the pasting process, the mapping corresponding to ID(%1) cannot be found</source>
        <translation>粘贴过程中，找不到ID(%1)对应的映射</translation>
    </message>
    <message>
        <location line="+157"/>
        <source>Unable to load item information from &lt;%1&gt;</source>
        <translation>无法通过&lt;%1&gt;加载元件信息</translation>
    </message>
    <message>
        <location line="+275"/>
        <source>current workflow file version:</source>
        <translation>当前工作流文件版本:</translation>
    </message>
    <message>
        <location line="+69"/>
        <source>An exception occurred during the process of parsing and pasting content, missing workflow tag</source>
        <translation>解析粘贴内容过程出现异常,缺失workflow标签</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>Cannot create item by class name:%1, maybe unregistered to DAGraphicsItemFactory</source>
        <translation>无法通过类名:%1创建元件,类名没有注册到DAGraphicsItemFactory</translation>
    </message>
    <message>
        <location line="+185"/>
        <source>unknown exception: get null figure widget at %1</source>
        <translation>未知异常：在第%1个位置获取到空的figure窗口</translation>
    </message>
    <message>
        <location line="+998"/>
        <source>The attribute %1=%2 under the tag %3 cannot be converted to double</source>
        <translation>标签%3下的属性%1=%2无法转换为double</translation>
    </message>
    <message>
        <location filename="../DAGui/DAZipArchive.cpp" line="+121"/>
        <source>No error</source>
        <translation>无错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>End of list of file</source>
        <translation>文件列表结束</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>File I/O error: %1</source>
        <translation>文件I/O错误: %1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Invalid parameter</source>
        <translation>无效参数</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bad zip file</source>
        <translation>损坏的zip文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Internal error</source>
        <translation>内部错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>CRC error</source>
        <translation>CRC校验错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Open error</source>
        <translation>打开错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unknown error (%1)</source>
        <translation>未知错误(%1)</translation>
    </message>
    <message>
        <location line="+380"/>
        <source>Failed to replace archive file</source>
        <translation>替换归档文件失败</translation>
    </message>
    <message>
        <location line="+202"/>
        <source>Failed to open archive</source>
        <translation>打开归档失败</translation>
    </message>
    <message>
        <location filename="../DAGui/DAZipArchiveTask_Xml.cpp" line="+57"/>
        <source>cannot create archive at &quot;%1&quot;, because %2</source>
        <translation>无法在&quot;%1&quot;创建归档，因为%2</translation>
    </message>
    <message>
        <location filename="../DAGui/Models/DADataManagerTreeModel.cpp" line="-440"/>
        <source>%1.%2,size:%3</source>
        <translation>%1.%2,长度:%3</translation>
    </message>
    <message>
        <location filename="../DAPyWorkFlow/DAPyNodeFactory.cpp" line="+236"/>
        <source>DA Python Node Factory</source>
        <translation>DA Python 节点工厂</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Python workflow node factory, discovers and creates Python-defined nodes via DANodeRegistry</source>
        <translation>Python工作流节点工厂，通过DANodeRegistry发现和创建Python定义的节点</translation>
    </message>
    <message>
        <location filename="../DAPyWorkFlow/DAPyWorkFlowUndoCommands.cpp" line="+29"/>
        <source>Add Node</source>
        <translation>添加节点</translation>
    </message>
    <message>
        <location line="+71"/>
        <source>Remove Node</source>
        <translation>移除节点</translation>
    </message>
    <message>
        <location line="+59"/>
        <source>Add Link</source>
        <translation>添加连接</translation>
    </message>
    <message>
        <location line="+57"/>
        <source>Remove Link</source>
        <translation>移除连接</translation>
    </message>
    <message>
        <location filename="../DAUtils/DAFormSchemaIO.cpp" line="+30"/>
        <source>field missing required &apos;name&apos;</source>
        <translation>字段缺少必需的 &apos;name&apos;</translation>
    </message>
    <message>
        <location line="+140"/>
        <source>&apos;items&apos; must be an array</source>
        <translation>&apos;items&apos; 必须是数组</translation>
    </message>
    <message>
        <location filename="../DAUtils/DATextReadWriter.cpp" line="+404"/>
        <source>No error occurred</source>
        <translation>没有发生错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>An error occurred when reading from the file</source>
        <translation>读取文件时发生错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>An error occurred when writing to the file</source>
        <translation>写入文件时发生错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>A fatal error occurred</source>
        <translation>发生致命错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Out of resources (eg, too many open files, out of memory, etc)</source>
        <translation>资源不足（例如：打开文件过多、内存不足等）</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The file could not be opened</source>
        <translation>无法打开文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The operation was aborted</source>
        <translation>操作被中止</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>A timeout occurred</source>
        <translation>发生超时</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>An unspecified error occurred</source>
        <translation>发生未指定的错误</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The file could not be removed</source>
        <translation>无法删除文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The file could not be renamed</source>
        <translation>无法重命名文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The position in the file could not be changed</source>
        <translation>无法更改文件位置</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The file could not be resized</source>
        <translation>无法调整文件大小</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The file could not be accessed</source>
        <translation>无法访问文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>The file could not be copied</source>
        <translation>无法复制文件</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>User Terminated</source>
        <translation>用户终止</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Unknown Error</source>
        <translation>未知错误</translation>
    </message>
    <message>
        <location filename="../DAUtils/DAXMLProtocol.cpp" line="+91"/>
        <source>DA xml protocol&apos;s root element error</source>
        <translation>DA xml协议的根节点异常</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>root element name error, require &quot;da&quot; but get %1</source>
        <translation>DA xml协议根节点要求为&quot;da&quot;标签，但解析到的为%1</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DA xml protocol missing &lt;props&gt; tag</source>
        <translation>DA xml协议缺失&lt;props&gt;标签</translation>
    </message>
    <message>
        <location filename="../DAPyScripts/DAPyScripts.cpp" line="+71"/>
        <source>Failed to initialize import scripts: %1</source>
        <translation>初始化导入脚本失败：%1</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DAPyScripts is not initialized, getIO() called before initScripts()</source>
        <translation>DAPyScripts 未初始化，在 initScripts() 之前调用了 getIO()</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>DAPyScripts is not initialized, getDataFrame() called before initScripts()</source>
        <translation>DAPyScripts 未初始化，在 initScripts() 之前调用了 getDataFrame()</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>DAPyScripts is not initialized, getDataProcess() called before initScripts()</source>
        <translation>DAPyScripts 未初始化，在 initScripts() 之前调用了 getDataProcess()</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DAPyScripts is not initialized, getStatistics() called before initScripts()</source>
        <translation>DAPyScripts 未初始化，在 initScripts() 之前调用了 getStatistics()</translation>
    </message>
    <message>
        <location filename="../DAPyScripts/DAPyScriptsDataFrame.cpp" line="+17"/>
        <source>cannot import da_dataframe module</source>
        <translation>无法导入 da_dataframe 模块</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>cannot import DAWorkbench.dataframe</source>
        <translation>无法导入 DAWorkbench.dataframe 模块</translation>
    </message>
    <message>
        <location line="+266"/>
        <source>setnan: rowsIndex size(%1) != colsIndex size(%2)</source>
        <translation>setnan: 行索引长度(%1)与列索引长度(%2)不一致</translation>
    </message>
    <message>
        <location filename="../DAPyScripts/DAPyScriptsDataProcess.cpp" line="+41"/>
        <source>cannot import da_data_processing module</source>
        <translation>无法导入 da_data_processing 模块</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>cannot import DAWorkbench.data_processing</source>
        <translation>无法导入 DAWorkbench.data_processing 模块</translation>
    </message>
    <message>
        <location filename="../DAPyScripts/DAPyScriptsIO.cpp" line="+68"/>
        <source>cannot import da_io module</source>
        <translation>无法导入 da_io 模块</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>cannot import DAWorkbench.io</source>
        <translation>无法导入 DAWorkbench.io 模块</translation>
    </message>
    <message>
        <location filename="../DAPyScripts/DAPyWorkBench.cpp" line="+41"/>
        <source>cannot import DAWorkbench module</source>
        <translation>无法导入 DAWorkbench 模块</translation>
    </message>
    <message>
        <location filename="../DAPyBindQt/pandas/DAPyIndex.cpp" line="+321"/>
        <source>DAPyIndex: the Python object type is not pandas.Index</source>
        <translation>DAPyIndex：Python 对象类型不是 pandas.Index</translation>
    </message>
    <message>
        <location filename="../DAPyBindQt/pandas/DAPyModulePandas.cpp" line="+193"/>
        <source>failed to open file %1 with UTF-8, trying ANSI encoding</source>
        <translation>使用 UTF-8 打开文件 %1 失败，尝试使用 ANSI 编码</translation>
    </message>
    <message>
        <location filename="../DAPyBindQt/pandas/DAPySeries.cpp" line="+683"/>
        <source>DAPySeries: the Python object type is not pandas.Series</source>
        <translation>DAPySeries：Python 对象类型不是 pandas.Series</translation>
    </message>
    <message>
        <location filename="../DAPluginSupport/DAPluginOption.cpp" line="+94"/>
        <source>Failed to load %1 (Reason: %2)</source>
        <translation>加载 %1 失败（原因：%2）</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Failed to create plugin instance from %1. Error: %2</source>
        <translation>无法从 %1 创建插件实例。错误：%2</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Failed to cast to DA plugin interface: %1</source>
        <translation>无法转换到 DA 插件接口：%1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Loaded plugin: %1</source>
        <translation>已加载插件：%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>successfully loaded plugin %1, but failed to initialize</source>
        <translation>成功加载插件 %1，但插件初始化失败</translation>
    </message>
    <message>
        <location line="+107"/>
        <source>plugin file name:%1,iid:%2,name:%3,description:%4,version:%5,error string:%6</source>
        <translation>插件文件名：%1，iid：%2，名称：%3，描述：%4，版本：%5，错误信息：%6</translation>
    </message>
    <message>
        <location filename="../DAGraphicsView/DACommandsForGraphics.cpp" line="+31"/>
        <source>Item Add</source>
        <translation>添加图元</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Items Add</source>
        <translation>添加多个图元</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Item Remove</source>
        <translation>移除图元</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>Items Remove</source>
        <translation>移除多个图元</translation>
    </message>
    <message>
        <location line="+63"/>
        <source>Items Move</source>
        <translation>移动多个图元</translation>
    </message>
    <message>
        <location line="+114"/>
        <source>Item Move</source>
        <translation>移动图元</translation>
    </message>
    <message>
        <location line="+88"/>
        <location line="+9"/>
        <source>Item Resize</source>
        <translation>调整图元尺寸</translation>
    </message>
    <message>
        <location line="+88"/>
        <source>Item Resize Width</source>
        <translation>调整图元宽度</translation>
    </message>
    <message>
        <location line="+63"/>
        <source>Item Resize Height</source>
        <translation>调整图元高度</translation>
    </message>
    <message>
        <location line="+70"/>
        <source>Item Rotation</source>
        <translation>旋转图元</translation>
    </message>
    <message>
        <location filename="../DAGraphicsView/DAGraphicsItemFactory.cpp" line="+88"/>
        <source>Class name %1 not registered to item factory</source>
        <translation>类名 %1 未注册到 item 工厂</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Type %1 not registered to item factory</source>
        <translation>类型 %1 未注册到 item 工厂</translation>
    </message>
    <message>
        <location filename="../DAFigure/DAChartItemCreatInteractor.cpp" line="+173"/>
        <source>Horizontal Line Marker</source>
        <translation>水平线标记</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Vertical Line Marker</source>
        <translation>垂直直线标记</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Cross Line Marker</source>
        <translation>十字线标记</translation>
    </message>
    <message>
        <location filename="../DAFigure/DAChartUtil.cpp" line="+48"/>
        <source>unknown chart</source>
        <translation>未知绘图</translation>
    </message>
    <message>
        <location line="+8"/>
        <location line="+7"/>
        <source>untitle-chart</source>
        <translation>绘图-未命名</translation>
    </message>
    <message>
        <location line="-2"/>
        <source>chart-%1</source>
        <translation>绘图-%1</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>untitle</source>
        <translation>未命名</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>item[%1]</source>
        <translation>图元[%1]</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>grid</source>
        <translation>网格</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>scale-%1</source>
        <translation>比例图元-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>legend-%1</source>
        <translation>图例-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>marker-%1</source>
        <translation>标记-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>curve-%1</source>
        <translation>曲线-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>spectro-%1</source>
        <translation>色谱图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>interval curve-%1</source>
        <translation>区间图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>histogram-%1</source>
        <translation>直方图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>spectrogram-%1</source>
        <translation>谱图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>graphic-%1</source>
        <translation>图像-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>OHLC-%1</source>
        <translation>OHLC图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>bar-%1</source>
        <translation>柱状图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>multibar-%1</source>
        <translation>柱状图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>shape-%1</source>
        <translation>形状-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+18"/>
        <source>text-%1</source>
        <translation>文本-%1</translation>
    </message>
    <message>
        <location line="-15"/>
        <source>zone-%1</source>
        <translation>区间-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>quiver-%1</source>
        <translation>流场图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>arrow-%1</source>
        <translation>箭头-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>box-%1</source>
        <translation>箱线图-%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>probe-%1</source>
        <translation>探针-%1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>unknown-%1</source>
        <translation>未知-%1</translation>
    </message>
    <message>
        <location filename="../DAFigure/DAFigureWidgetCommands.cpp" line="+59"/>
        <location line="+14"/>
        <source>create chart</source>
        <translation>创建绘图</translation>
    </message>
    <message>
        <location line="+74"/>
        <location line="+17"/>
        <source>create 3D chart</source>
        <translation>创建3D绘图</translation>
    </message>
    <message>
        <location line="+61"/>
        <source>remove chart</source>
        <translation>移除绘图</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>remove 3D chart</source>
        <translation>移除3D绘图</translation>
    </message>
    <message>
        <location line="+58"/>
        <source>set figure widget size</source>
        <translation>设置绘图中窗体的尺寸</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>add item in chart</source>
        <translation>添加图元到绘图</translation>
    </message>
    <message>
        <location line="+66"/>
        <source>add 3D item in chart</source>
        <translation>添加3D图元到绘图</translation>
    </message>
    <message>
        <location line="+67"/>
        <source>move plot item to another chart</source>
        <translation>移动图元到另一个绘图</translation>
    </message>
    <message>
        <location line="+63"/>
        <source>move 3D plot item to another chart</source>
        <translation>移动3D图元到另一个绘图</translation>
    </message>
    <message>
        <location line="+58"/>
        <source>remove item from chart</source>
        <translation>删除绘图中的图元</translation>
    </message>
    <message>
        <location line="+68"/>
        <source>move plot item position</source>
        <translation>移动图元位置</translation>
    </message>
    <message>
        <location filename="../DAFigure/Models/DAStandardItemPlot.cpp" line="+72"/>
        <source>layout-%1</source>
        <translation>布局-%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>layout</source>
        <translation>布局</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>Parasite Plot</source>
        <translation>寄生绘图</translation>
    </message>
    <message>
        <location filename="../DAFigure/Models/DAStandardItemPlotScale.cpp" line="+167"/>
        <source>DateTime Scale</source>
        <translation>时间轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Log Scale</source>
        <translation>对数轴</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Y Left</source>
        <translation>Y左轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Y Right</source>
        <translation>Y右轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Bottom</source>
        <translation>X底轴</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>X Top</source>
        <translation>X顶轴</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Unknown</source>
        <translation>未知</translation>
    </message>
    <message>
        <location filename="../DAData/DAAbstractData.cpp" line="+154"/>
        <source>none</source>
        <translation>无</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>package</source>
        <translation>数据包</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>object</source>
        <translation>对象</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>dataframe</source>
        <translation>数据框</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>series</source>
        <translation>序列</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>raw</source>
        <translation>原始数据</translation>
    </message>
    <message>
        <location filename="../DAData/DACommandsDataManager.cpp" line="+22"/>
        <source>add data</source>
        <translation>添加数据</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>remove data</source>
        <translation>移除数据</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>rename data</source>
        <translation>重命名数据</translation>
    </message>
    <message>
        <location filename="../../plugins/DataAnalysis/Commands.cpp" line="+25"/>
        <source>Data Select</source>
        <translation>数据过滤</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Data Sort</source>
        <translation>数据排序</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>Data Query</source>
        <translation>数据查询</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>Eval Data</source>
        <translation>列运算</translation>
    </message>
    <message>
        <location filename="../DAGui/DAFormEditorRegistry.cpp" line="+376"/>
        <source>Add</source>
        <translation>添加</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Remove</source>
        <translation>删除</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>New Item</source>
        <translation>新项目</translation>
    </message>
    <message>
        <location filename="../APP/SettingPages/DAAppConfig.cpp" line="+84"/>
        <location line="+37"/>
        <source>Cannot open config file &quot;%1&quot;: %2</source>
        <translation>无法打开配置文件\&quot;%1\&quot;，原因是%2</translation>
    </message>
    <message>
        <location line="-28"/>
        <source>Cannot load config file &quot;%1&quot;: %2</source>
        <translation>无法加载配置文件\&quot;%1\&quot;，原因是%2</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Config file (%1) is missing the &lt;configs&gt; tag</source>
        <translation>配置文件(%1)缺失&lt;configs&gt;标签</translation>
    </message>
    <message>
        <location filename="../DAPyScripts/DAPyScriptsStatistics.cpp" line="+17"/>
        <source>cannot import DAWorkbench.DAStatistics module</source>
        <translation>无法导入 DAWorkbench.DAStatistics 模块</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>cannot import DAWorkbench.DAStatistics</source>
        <translation>无法导入 DAWorkbench.DAStatistics 模块</translation>
    </message>
    <message>
        <location filename="../APP/DAStatsPlotCoordinator.cpp" line="+84"/>
        <source>The selected data source is empty</source>
        <translation>选中的数据源为空</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Unknown plot type: %1</source>
        <translation>未知的绘图类型: %1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Python error in statistical plot: %1</source>
        <translation>统计绘图 Python 错误: %1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Error in statistical plot: %1</source>
        <translation>统计绘图错误: %1</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Column &apos;%1&apos; not found in data</source>
        <translation>数据中找不到列 &apos;%1&apos;</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Failed to compute histogram for column &apos;%1&apos;: %2</source>
        <translation>计算列 &apos;%1&apos; 的直方图失败: %2</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Histogram result is empty for column &apos;%1&apos;</source>
        <translation>列 &apos;%1&apos; 的直方图结果为空</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>KDE overlay failed for column &apos;%1&apos;: %2</source>
        <translation>列 &apos;%1&apos; 的 KDE 叠加失败: %2</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Failed to compute grouped histogram (column &apos;%1&apos;, hue &apos;%2&apos;): %3</source>
        <translation>计算分组直方图失败 (列 &apos;%1&apos;, 分组 &apos;%2&apos;): %3</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>KDE overlay failed for hue groups: %1</source>
        <translation>分组 KDE 叠加失败: %1</translation>
    </message>
    <message>
        <location filename="../DAFigure/Models/DAStandardItemPlot3D.cpp" line="+69"/>
        <source>3D Chart</source>
        <translation>3D绘图</translation>
    </message>
    <message>
        <location filename="../DAGui/Commands/DACommandsTableColumnFormat.cpp" line="+16"/>
        <source>change table display format</source>
        <translation>改变表格显示格式</translation>
    </message>
    <message>
        <location filename="../DAPyScripts/DAPyScriptRunner.cpp" line="+268"/>
        <source>Python interpreter is not initialized, script runner cannot start</source>
        <translation>Python 解释器未初始化，脚本执行引擎无法启动</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Script runner failed to import baseline module %1: %2</source>
        <translation>脚本执行引擎导入基线模块 %1 失败：%2</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Failed to initialize script runner: %1</source>
        <translation>初始化脚本执行引擎失败：%1</translation>
    </message>
    <message>
        <location filename="../DAFigure/DAChartTextEditorPopup.cpp" line="+80"/>
        <source>Text Background Color</source>
        <translation>文字背景色</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Strikethrough</source>
        <translation>删除线</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Superscript</source>
        <translation>上标</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Subscript</source>
        <translation>下标</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Clear</source>
        <translation>清除</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Clear character format of selected text</source>
        <translation>清除选中文本的字符格式</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>OK</source>
        <translation>确定</translation>
    </message>
    <message>
        <location filename="../DAFigure/DAChartTextMarkerEditor.cpp" line="+128"/>
        <source>Text Marker</source>
        <translation>文本标注</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Text</source>
        <translation>文本</translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../APP/main.cpp" line="-127"/>
        <source>version:%1,compile datetime:%2</source>
        <translation>版本：%1，编译时间：%2</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>The project file to open</source>
        <translation>要打开的工程文件</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Import data into the application, supporting formats such as CSV, XLSX, TXT, PKL, etc.If you want to import multiple datasets, you can use the command multiple times; the program will execute them one by one</source>
        <translation>导入数据到应用程序中，支持csv/xlsx/txt/pkl等格式，如果要导入多个数据，你可以使用多次命令，程序会逐一执行</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Disable the splash screen during startup, useful for debugging to avoid the splash window blocking the IDE</source>
        <translation>禁用启动画面，适用于调试时避免启动窗口遮挡IDE</translation>
    </message>
</context>
</TS>
