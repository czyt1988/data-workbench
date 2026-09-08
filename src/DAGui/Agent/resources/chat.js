let chatBridge = null;
let md = null;
let currentAgentMsg = null;  // 当前正在流式输出的消息 DOM 节点
let renderTimer = null;      // appendToken 的防抖计时器
const RENDER_DEBOUNCE_MS = 50;  // 最多每 50ms 重新渲染一次

// —— 工具调用分组渲染状态 ——
// 协议保证 tool_call/tool_result 严格交替（Python _rpc_call 顺序 await），
// 因此同一时刻至多一张卡片待结果，按工具名 FIFO 匹配即可。
let currentToolGroup = null;   // 当前工具分组容器（null 表示无活跃分组）
let pendingToolCards = [];     // 等待结果的卡片列表 [{card, toolName}]

// —— 历史重放性能（分段渲染 + 滚动抑制）——
// 诊断结论（harness 实测）：重放 340 事件时逐事件 scrollToBottom() 强制同步
// reflow 合计占渲染耗时 ~97%（O(n²)：每次 reflow 随 DOM 增大线性变贵）。
// 两个对策：
//   1) suppressAutoScroll：批量重放/分段加载期间跳过逐事件滚动，收尾统一滚一次；
//   2) HISTORY_CHUNK_SIZE 分段渲染：超长会话只渲染尾部一段，滚动到顶部再
//      prepend 更早一段（保持视口不跳动），首屏耗时与会话总长度解耦。
const HISTORY_CHUNK_SIZE = 150;   // 每段渲染的 UI 事件数
let suppressAutoScroll = false;   // true 时 scrollToBottom 早退（批量渲染窗口）
let pendingEarlierEvents = [];    // 尚未渲染的更早事件（时间升序，尾部为更近）
let renderTargetOverride = null;  // 非 null 时渲染函数追加到此容器（prepend 分段用）
let loadEarlierSentinel = null;   // 顶部"加载更早"哨兵元素（null 表示无）
let loadingEarlier = false;       // 分段加载进行中（防 scroll 事件重入）

// —— 输入区/状态栏 web 化状态 ——
// i18n 静态标签由 C++ 在握手(onReady)时经 setI18nLabels 注入（C++ 仍是唯一 i18n 拥有者，
// 沿用 appendQuestion 推 tr("Submit") 的既有约定；JS 为哑显示）。
let i18n = {
    send: 'Send', stop: 'Stop',
    ready: 'Ready', thinking: 'Agent thinking...', stopping: 'Stopping...',
    starting: 'Agent starting...',
    inputPlaceholder: '', tokenEmpty: 'tokens: -',
    popoverInput: 'input: %1', popoverOutput: 'output: %1',
    popoverTotal: 'total: %1', popoverWindow: 'window: %1',
    popoverSource: 'source: %1', popoverSourceUnknown: 'unknown',
    modelEmpty: 'No model', modelSelectTip: 'Select LLM model',
    modelProvidersTitle: 'Providers', modelBack: 'Back',
    errorDetails: 'Details', errorCopy: 'Copy', errorCopied: 'Copied', errorTruncated: '[truncated]',
    // —— 历史分段懒加载 ——
    loadEarlier: 'Load earlier messages',
    // —— 权限模式选择器（permission-layer P1）——
    modeSelectTip: 'Permission mode',
    modeYolo: 'Full Auto', modeAuto: 'Auto', modeManual: 'Ask Every Time',
    modeYoloTip: 'Run everything without asking (system directories still blocked)',
    modeAutoTip: 'Reads and chart edits pass; file writes and code execution judged by rules',
    modeManualTip: 'File writes and code execution need approval every time',
    modeYoloConfirm: 'Switch to Full Auto mode? Code execution and file writes will no longer ask for confirmation.',
    modeYoloConfirmOk: 'Switch', modeYoloConfirmCancel: 'Cancel',
    // —— 工具审批卡（permission-layer P1）——
    approvalNeeds: 'needs your approval',
    approvalApprove: 'Approve', approvalDeny: 'Deny',
    approvalApproveRemember: 'Approve && remember for this session',
    approvalApproved: 'Approved', approvalDenied: 'Denied',
    approvalApprovedRemembered: 'Approved (remembered for this session)',
    approvalCodeMoreLines: '%1 more lines',
    approvalFromSubagent: 'From subagent: %1',
    // —— 工具排队状态（决策点 2 ③，审计问题 12）——
    toolQueued: 'queued', toolRunning: 'running',
    // —— 子 agent 进度卡片（subagent-phase1 C）——
    subagentTaskCount: '%1 subagent task(s)',
    subagentProgress: '%1/%2 done',
    subagentCompleted: 'completed',
    subagentQueued: 'queued', subagentRunning: 'running',
    subagentDone: 'done', subagentFailed: 'failed',
    subagentTimeout: 'timeout', subagentStopped: 'stopped'
};
let agentBusy = false;          // 当前是否思考中（驱动 send-btn 的 Send/Stop 切换）
let tokenStatsCache = null;     // 缓存最近一次 setTokenStats 的 5 值，供 popover 渲染

// —— 两级模型选择器状态 ——
// C++ 推送 flat 可用模型列表（{provider,model,context_window,max_output_tokens}），
// JS 按 provider 分组渲染两级选择器：第一层供应商列表，第二层该供应商的模型列表。
let modelSelectorData = [];      // 分组后 [{provider, models:[{model,...}]}]
let activeProvider = '';          // 当前激活供应商
let activeModel = '';             // 当前激活模型 id
let modelDropdownView = 'providers';  // 'providers'（供应商层）| 'models'（模型层）
let modelDropdownProvider = '';       // 模型层当前展示的供应商

// —— 权限模式选择器状态（permission-layer P1）——
// 三档：yolo（全自动，默认）/ auto（规则判定）/ manual（每次询问）。
// C++ 经 setPermissionMode 推送当前态；用户点选切 yolo 时先弹二次确认卡。
let activePermissionMode = 'yolo';   // 当前激活模式（未知态回退 yolo，与 C++ 默认一致）

function initMarkdown() {
    md = window.markdownit({
        html: false,
        breaks: true,
        linkify: true,
        highlight: function(str, lang) {
            if (lang && hljs.getLanguage(lang)) {
                try {
                    return '<pre><code class="hljs">' +
                        hljs.highlight(str, {language: lang}).value +
                        '</code></pre>';
                } catch (__) {}
            }
            return '<pre><code class="hljs">' + md.utils.escapeHtml(str) + '</code></pre>';
        }
    });
    setupMathRules(md);
}

// —— KaTeX 数学公式渲染（与 MarkdownView/resources/markdown.js 保持一致）——
// 识别 $...$（行内）与 $$...$$（独立成行）两类 LaTeX 公式，经 katex.renderToString
// 输出 HTML。$ 内侧紧邻非空白字符，避免 "$10 与 $20" 之类的货币误判。
function setupMathRules(markdown) {
    // 行内公式 $...$：要求开 $ 后与闭 $ 前均非空白，且闭 $ 后不是数字（排除 "$5 1990s$"）
    markdown.inline.ruler.after('escape', 'math_inline', function(state, silent) {
        var start = state.pos;
        if (state.src[start] !== '$') return false;
        if (start > 0 && !/[\s(\[]/.test(state.src[start - 1])) return false;  // 前需空白/括号
        var pos = start + 1;
        if (pos >= state.posMax || /\s/.test(state.src[pos])) return false;   // 开 $ 后非空白
        var found = -1;
        while (pos < state.posMax) {
            if (state.src[pos] === '$' && !/\s/.test(state.src[pos - 1])) { found = pos; break; }
            if (state.src[pos] === '\n') break;  // 行内公式不跨行
            pos++;
        }
        if (found < 0) return false;
        if (found + 1 < state.posMax && /[0-9]/.test(state.src[found + 1])) return false;  // 闭 $ 后非数字
        if (!silent) {
            var token = state.push('math_inline', 'math', 0);
            token.markup = '$';
            token.content = state.src.slice(start + 1, found);
        }
        state.pos = found + 1;
        return true;
    });
    markdown.renderer.rules.math_inline = function(tokens, idx) {
        return renderKatex(tokens[idx].content, false);
    };
    // 独立公式 $$...$$：整块匹配（含前后空行由 block 环境处理）
    markdown.block.ruler.after('blockquote', 'math_block', function(state, startLine, endLine, silent) {
        var startPos = state.bMarks[startLine] + state.tShift[startLine];
        var maxPos = state.eMarks[startLine];
        var line = state.src.slice(startPos, maxPos);
        var m = /^\s*\$\$([\s\S]+?)\$\$\s*$/.exec(state.src.slice(startPos, state.eMarks[endLine - 1]));
        if (!m) {
            // 多行 $$：首行以 $$ 开头，向后查找 $$ 结束行
            if (!/^\s*\$\$/.test(line)) return false;
            var endL = startLine + 1;
            var foundEnd = false;
            while (endL < endLine) {
                var l = state.src.slice(state.bMarks[endL] + state.tShift[endL], state.eMarks[endL]);
                if (/\$\$\s*$/.test(l)) { foundEnd = true; break; }
                endL++;
            }
            if (!foundEnd) return false;
            var tex = [];
            for (var i = startLine; i <= endL; i++) {
                var cl = state.src.slice(state.bMarks[i] + state.tShift[i], state.eMarks[i]);
                cl = cl.replace(/^\s*\$\$/, '').replace(/\$\$\s*$/, '');
                tex.push(cl);
            }
            if (!silent) {
                var token = state.push('math_block', 'math', 0);
                token.block = true;
                token.content = tex.join('\n').trim();
                token.map = [startLine, endL + 1];
            }
            state.line = endL + 1;
            return true;
        }
        if (!silent) {
            var token2 = state.push('math_block', 'math', 0);
            token2.block = true;
            token2.content = m[1].trim();
            token2.map = [startLine, startLine + 1];
        }
        state.line = startLine + 1;
        return true;
    });
    markdown.renderer.rules.math_block = function(tokens, idx) {
        return '<p class="math-block">' + renderKatex(tokens[idx].content, true) + '</p>';
    };
}

function renderKatex(tex, displayMode) {
    if (typeof katex === 'undefined' || !katex) {
        // katex 未加载（资源缺失）时降级为代码文本，保持内容不丢
        return '<code>' + (displayMode ? '$$' + tex + '$$' : '$' + tex + '$') + '</code>';
    }
    try {
        return katex.renderToString(tex, {throwOnError: false, displayMode: !!displayMode});
    } catch (e) {
        return '<code>' + (displayMode ? '$$' + tex + '$$' : '$' + tex + '$') + '</code>';
    }
}

function createMessageBubble(className) {
    let div = document.createElement('div');
    div.className = 'message-bubble ' + className;
    div.dataset.rawText = '';
    return div;
}

function scrollToBottom() {
    // 批量重放/分段加载窗口内跳过：每事件强制 reflow 是 O(n²) 性能杀手
    if (suppressAutoScroll) return;
    // #messages 是唯一滚动容器（body 已 overflow:hidden），滚它而非 window
    var m = document.getElementById('messages');
    if (m) { m.scrollTop = m.scrollHeight; }
}

// 历史渲染的追加目标：默认 #messages；分段 prepend 时由 loadEarlierChunk
// 切到 detached 容器（渲染完整体 insertBefore 哨兵，保持时序）。
function getRenderTarget() {
    return renderTargetOverride || document.getElementById('messages');
}

function init() {
    initMarkdown();
    new QWebChannel(qt.webChannelTransport, function(channel) {
        chatBridge = channel.objects.chatBridge;
        // 握手：通知 C++ web 侧已就绪，C++ 回推 setI18nLabels/setBusy/setAvailableModels/setActiveModel/setTokenStats。
        // 缓解 JS-ready 竞态——若 agent 信号在 chat.html 加载完成前触发，此处 flush 当前态。
        if (chatBridge && typeof chatBridge.onReady === 'function') {
            chatBridge.onReady();
        }
    });
    // 拦截 da-figure: 超链接点击，交给 C++ 端打开对应绘图。
    // 事件委托挂在稳定的 #messages 上：流式防抖会重建气泡 innerHTML，
    // 绑在气泡节点上的监听器会丢失，挂在 #messages 始终有效（clearChat 只清 innerHTML）。
    var msgs = document.getElementById('messages');
    if (msgs) {
        msgs.addEventListener('click', function(e) {
            var link = e.target.closest('a');
            if (!link) return;
            var href = link.getAttribute('href') || '';
            if (href.toLowerCase().indexOf('da-figure:') !== 0) return;  // 仅匹配 da-figure: 前缀，放行普通链接
            e.preventDefault();  // 阻止 WebEngine 内部导航（自定义 scheme 无目标页）
            // 流式未完成时 href 可能只是 "da-figure:"，跳过避免无效调用
            if (href === 'da-figure:' || href === 'da-figure:/') return;
            if (chatBridge && typeof chatBridge.onFigureLink === 'function') {
                chatBridge.onFigureLink(href);
            }
        });
        // 分段懒加载：滚动到顶部附近（<=60px）自动 prepend 更早一段。
        // requestLoadEarlier 内有 loadingEarlier 防重入；scrollTop 只读不写，
        // scroll 事件本身在布局之后派发，无强制 reflow 开销。
        msgs.addEventListener('scroll', function() {
            if (pendingEarlierEvents.length === 0) return;
            if (msgs.scrollTop <= 60) {
                requestLoadEarlier();
            }
        });
    }

    // —— 输入区：发送/终止按钮 ——
    var sendBtn = document.getElementById('send-btn');
    if (sendBtn) {
        sendBtn.addEventListener('click', onSendClicked);
    }

    // —— token 明细 popover：点 token-label 切换显隐，点外部关闭 ——
    var tokenLabel = document.getElementById('token-label');
    var tokenPopover = document.getElementById('token-popover');
    if (tokenLabel && tokenPopover) {
        tokenLabel.addEventListener('click', function(e) {
            e.stopPropagation();
            if (tokenPopover.hasAttribute('hidden')) {
                rebuildTokenPopover();
                tokenPopover.removeAttribute('hidden');
            } else {
                tokenPopover.setAttribute('hidden', '');
            }
        });
        document.addEventListener('click', function(e) {
            if (tokenPopover.hasAttribute('hidden')) return;
            if (e.target === tokenLabel || tokenPopover.contains(e.target)) return;
            tokenPopover.setAttribute('hidden', '');
        });
    }

    // —— 两级模型选择器：触发按钮切换 + 点外部关闭 ——
    var modelTrigger = document.getElementById('model-trigger');
    if (modelTrigger) {
        modelTrigger.addEventListener('click', function(e) {
            e.stopPropagation();
            var dd = document.getElementById('model-dropdown');
            if (!dd) return;
            if (dd.hasAttribute('hidden')) {
                openModelDropdown();
            } else {
                closeModelDropdown();
            }
        });
    }
    var modelSelector = document.getElementById('model-selector');
    document.addEventListener('click', function(e) {
        var dd = document.getElementById('model-dropdown');
        if (!dd || dd.hasAttribute('hidden')) return;
        // 下拉内点击（back/供应商行）会先同步重渲染并 detach e.target，
        // contains(e.target) 会误判为外部点击而关闭下拉；
        // composedPath() 在事件派发时捕获完整路径，不受重渲染影响。
        if (modelSelector && e.composedPath && e.composedPath().indexOf(modelSelector) !== -1) return;
        if (modelSelector && modelSelector.contains(e.target)) return;
        closeModelDropdown();
    });

    // —— 权限模式选择器：触发按钮切换 + 点外部关闭（镜像模型选择器交互）——
    var modeTrigger = document.getElementById('mode-trigger');
    if (modeTrigger) {
        modeTrigger.addEventListener('click', function(e) {
            e.stopPropagation();
            var dd = document.getElementById('mode-dropdown');
            if (!dd) return;
            if (dd.hasAttribute('hidden')) {
                openModeDropdown();
            } else {
                closeModeDropdown();
            }
        });
    }
    var modeSelector = document.getElementById('mode-selector');
    document.addEventListener('click', function(e) {
        var dd = document.getElementById('mode-dropdown');
        if (!dd || dd.hasAttribute('hidden')) return;
        if (modeSelector && e.composedPath && e.composedPath().indexOf(modeSelector) !== -1) return;
        if (modeSelector && modeSelector.contains(e.target)) return;
        closeModeDropdown();
    });

    // 初始化触发按钮文案
    updateModelTrigger();
    updateModeTrigger();
}

// 用户点击发送/终止按钮。C++ 仍是编排者：JS 只负责取文本+清框+通知，
// 用户气泡由 C++ 收到 onUserMessage 后调 appendUserMessage 渲染（与旧 onSendClicked 同构）。
function onSendClicked() {
    if (agentBusy) {
        // 忙碌时按钮=Stop：通知 C++ 终止
        if (chatBridge && typeof chatBridge.onStopRequested === 'function') {
            chatBridge.onStopRequested();
        }
        return;
    }
    var ta = document.getElementById('input-edit');
    if (!ta) return;
    var text = ta.value.trim();
    if (!text) return;
    ta.value = '';  // JS 清框（C++ 侧不再触碰输入控件）
    if (chatBridge && typeof chatBridge.onUserMessage === 'function') {
        chatBridge.onUserMessage(text);
    }
}

// —— Agent 文本气泡辅助 ——

// 把当前累积的 token 立即渲染定稿（取消防抖）。用于工具调用开始前、
// 用户消息前等场景，使叙述文本与工具调用按时序排列，避免被后续
// finalizeAgentMessage(fullText) 覆盖丢失中间叙述。
function flushAgentMessage() {
    if (renderTimer) {
        clearTimeout(renderTimer);
        renderTimer = null;
    }
    if (!currentAgentMsg) return;
    if (currentAgentMsg.dataset.rawText) {
        currentAgentMsg.innerHTML = md.render(currentAgentMsg.dataset.rawText);
    } else {
        // 空气泡（agent 在工具调用前未输出任何文本）——移除避免留白
        currentAgentMsg.remove();
    }
    currentAgentMsg = null;
}

// —— 工具分组与卡片渲染 ——

// 右指箭头 SVG（展开时旋转 90° 朝下）
const CHEVRON_SVG = '<svg class="chevron" viewBox="0 0 24 24" width="14" height="14" aria-hidden="true">' +
    '<path fill="currentColor" d="m13.172 12l-4.95-4.95l1.414-1.413L16 12l-6.364 6.364l-1.414-1.415z"/></svg>';

// 关闭当前工具分组：标记完成状态并解除引用，使后续工具调用开启新分组。
function closeToolGroup() {
    if (!currentToolGroup) return;
    let allDone = pendingToolCards.length === 0;
    currentToolGroup.classList.remove('active');
    let dot = currentToolGroup.querySelector('.status-dot');
    let meta = currentToolGroup.querySelector('.group-meta');
    if (allDone) {
        if (dot) dot.className = 'status-dot ok';
        if (meta) meta.textContent = '\u00b7 completed';  // cn:· 已完成
    } else {
        // 有卡片未收到结果（异常路径）——标记为未完成
        if (dot) dot.className = 'status-dot err';
        if (meta) meta.textContent = '\u00b7 incomplete';  // cn:· 未完成
        pendingToolCards = [];
    }
    currentToolGroup = null;
}

// 若无活跃分组则创建一个。连续的工具调用（中间无 agent 文本）归入同一分组。
function ensureToolGroup() {
    if (currentToolGroup) return;
    let group = document.createElement('div');
    group.className = 'tool-group open active';
    group.innerHTML =
        '<button class="tool-group-head" type="button">' +
            '<span class="status-dot running"></span>' +
            '<span class="group-title">1 tool call</span>' +  // cn:1 个工具调用
            '<span class="group-meta">\u00b7 running</span>' +  // cn:· 运行中
            CHEVRON_SVG +
        '</button>' +
        '<div class="tool-group-body"></div>';
    group.querySelector('.tool-group-head').addEventListener('click', function() {
        group.classList.toggle('open');
    });
    getRenderTarget().appendChild(group);
    currentToolGroup = group;
}

// 创建一张工具调用卡片（折叠态）。args 已知，result 待 appendToolResult 填充。
function createToolCard(toolName, args) {
    let card = document.createElement('div');
    card.className = 'tool-card running';
    card.dataset.toolName = toolName;
    let argsJson = JSON.stringify(args, null, 2) || '{}';
    card.innerHTML =
        '<button class="tool-card-head" type="button">' +
            '<span class="status-dot running"></span>' +
            '<span class="card-title">' + escapeHtml(toolName) + '</span>' +
            '<span class="card-summary">running</span>' +  // cn:运行中
            CHEVRON_SVG +
        '</button>' +
        '<div class="tool-card-body">' +
            '<div class="tool-section">' +
                '<div class="tool-section-label">Input</div>' +  // cn:参数
                '<pre class="tool-json">' + escapeHtml(argsJson) + '</pre>' +
            '</div>' +
            '<div class="tool-section result-section" style="display:none">' +
                '<div class="tool-section-label">Output</div>' +  // cn:结果
                '<pre class="tool-json"></pre>' +
            '</div>' +
        '</div>';
    card.querySelector('.tool-card-head').addEventListener('click', function() {
        card.classList.toggle('open');
    });
    return card;
}

// 工具结果到达：更新卡片状态、摘要、结果区。
function updateToolCardResult(card, result) {
    let success = result.success !== false;
    let summary = result.message || result.error || (success ? 'done' : 'failed');  // cn:完成/失败
    card.classList.remove('running');
    card.classList.add(success ? 'ok' : 'err');
    let dot = card.querySelector('.status-dot');
    if (dot) dot.className = 'status-dot ' + (success ? 'ok' : 'err');
    let summaryEl = card.querySelector('.card-summary');
    if (summaryEl) summaryEl.textContent = summary;
    let resultSection = card.querySelector('.result-section');
    if (resultSection) {
        resultSection.style.display = '';
        let pre = resultSection.querySelector('.tool-json');
        if (pre) pre.textContent = JSON.stringify(result, null, 2);
    }
}

// 刷新分组标题：调用计数 + 运行/完成状态。
function updateToolGroupHeader() {
    if (!currentToolGroup) return;
    let cards = currentToolGroup.querySelectorAll('.tool-card');
    let n = cards.length;
    let title = currentToolGroup.querySelector('.group-title');
    if (title) title.textContent = n + ' tool call' + (n > 1 ? 's' : '');  // cn:N 个工具调用
    let pending = pendingToolCards.length;
    let meta = currentToolGroup.querySelector('.group-meta');
    let dot = currentToolGroup.querySelector('.status-dot');
    if (pending === 0) {
        if (meta) meta.textContent = '\u00b7 completed';  // cn:· 已完成
        if (dot) dot.className = 'status-dot ok';
    } else {
        if (meta) meta.textContent = '\u00b7 running';  // cn:· 运行中
        if (dot) dot.className = 'status-dot running';
    }
}

// —— 子 agent 进度卡片（subagent-phase1 C）——
// C++ 经 DAAgentWebChannel::updateSubagentProgress 推送 subagent_progress 协议消息：
//   {call_id, task_id?, subagent?, state: spawned|running|done|error|timeout|stopped,
//    message?, results?}；心跳为无 task_id 的 running 态（忽略）；聚合态为无
//   task_id 的 done（results.tasks 携带各任务终态）。卡片以 call_id 为键，
//   任务行以 task_id 为键幂等更新（乱序/迟到消息防御）。
var subagentCards = {};  // call_id → {group, tasks: {task_id: {el, terminal}}}

// 派发卡片创建（spawned 首条触发；折叠态，复用工具分组样式）。
function createSubagentGroup(callId) {
    flushAgentMessage();
    // 审计问题 28a：dispatch_subagents 的工具卡已建卡入 pendingToolCards，
    // 其结果要等派发结束才到达——直接 closeToolGroup 会因 pending 非空走
    // "异常路径"（红点 incomplete + 清空 pending），后到的 tool_result 被
    // appendToolResult findIndex 落空静默丢弃，dispatch 卡永远误标未完成。
    // 修复：暂存待决卡片跨过关组（卡片 DOM 留在上一组、继续等结果），
    // 关组按 completed 收尾，结果到达时依 FIFO 自然补全
    let carriedCards = pendingToolCards;
    pendingToolCards = [];
    closeToolGroup();  // 前序工具分组收尾，进度卡片独立成卡
    pendingToolCards = carriedCards;
    let group = document.createElement('div');
    group.className = 'tool-group subagent-group active';
    group.dataset.callId = callId;
    group.innerHTML =
        '<button class="tool-group-head" type="button">' +
            '<span class="status-dot running"></span>' +
            '<span class="group-title"></span>' +
            '<span class="group-meta"></span>' +
            CHEVRON_SVG +
        '</button>' +
        '<div class="tool-group-body"></div>';
    group.querySelector('.tool-group-head').addEventListener('click', function() {
        group.classList.toggle('open');
    });
    document.getElementById('messages').appendChild(group);
    return group;
}

// 任务行创建（task_id 形如 "explore #1"，message 为 spawned 携带的提示词摘要）。
function createSubagentTaskRow(taskId, message) {
    let row = document.createElement('div');
    row.className = 'subagent-task';
    row.dataset.taskId = taskId;
    row.innerHTML =
        '<span class="status-dot idle"></span>' +
        '<span class="subagent-task-name"></span>' +
        '<span class="subagent-task-meta"></span>';
    row.querySelector('.subagent-task-name').textContent = taskId;
    let meta = row.querySelector('.subagent-task-meta');
    meta.textContent = message || (i18n.subagentQueued || 'queued');
    if (message) {
        row.title = message;  // 完整提示词摘要挂 tooltip（折叠行内省略）
    }
    return row;
}

// 任务行状态更新（幂等：终态只允许一次，后续同任务消息忽略）。
function updateSubagentTaskRow(entry, taskId, state, message) {
    let t = entry.tasks[taskId];
    if (!t) {
        // 审计问题 28b：非 spawned 态也惰性建行（乱序/迟到/重放丢头部事件时
        // 任务进度不再整体不可见）；spawned 之外的态没有提示词摘要，行 meta
        // 由下方状态分支填充
        let el = createSubagentTaskRow(taskId, message);
        entry.group.querySelector('.tool-group-body').appendChild(el);
        t = entry.tasks[taskId] = { el: el, terminal: false };
    }
    if (t.terminal) return;  // 已终态——幂等防御
    let dot = t.el.querySelector('.status-dot');
    let meta = t.el.querySelector('.subagent-task-meta');
    if (state === 'spawned') {
        if (dot) dot.className = 'status-dot idle';
    } else if (state === 'running') {
        if (dot) dot.className = 'status-dot running';
        if (meta) meta.textContent = i18n.subagentRunning || 'running';
    } else if (state === 'done') {
        t.terminal = true;
        if (dot) dot.className = 'status-dot ok';
        if (meta) meta.textContent = i18n.subagentDone || 'done';
    } else if (state === 'error' || state === 'timeout') {
        t.terminal = true;
        if (dot) dot.className = 'status-dot err';
        if (meta) meta.textContent = message || (state === 'timeout'
            ? (i18n.subagentTimeout || 'timeout') : (i18n.subagentFailed || 'failed'));
        if (message) t.el.title = message;
    } else if (state === 'stopped') {
        t.terminal = true;
        if (dot) dot.className = 'status-dot idle';
        if (meta) meta.textContent = i18n.subagentStopped || 'stopped';
    }
}

// 刷新派发卡片头：任务计数 + 进度（终态数/总数），全部终态转 completed。
function updateSubagentGroupHeader(entry) {
    let total = 0, done = 0;
    for (var id in entry.tasks) {
        if (!Object.prototype.hasOwnProperty.call(entry.tasks, id)) continue;
        total++;
        if (entry.tasks[id].terminal) done++;
    }
    let title = entry.group.querySelector('.group-title');
    if (title) {
        title.textContent = fmtTmpl(i18n.subagentTaskCount || '%1 subagent task(s)', total);
    }
    let meta = entry.group.querySelector('.group-meta');
    let dot = entry.group.querySelector('.status-dot');
    if (total > 0 && done >= total) {
        if (meta) meta.textContent = '\u00b7 ' + (i18n.subagentCompleted || 'completed');
        if (dot) dot.className = 'status-dot ok';
        entry.group.classList.remove('active');
    } else {
        if (meta) {
            meta.textContent = '\u00b7 ' + String(i18n.subagentProgress || '%1/%2 done')
                .replace('%1', done).replace('%2', total);
        }
        if (dot) dot.className = 'status-dot running';
    }
}

// C++ 推送子 agent 进度（subagent_progress 协议消息原文）。
function updateSubagentProgress(payload) {
    payload = payload || {};
    let callId = payload.call_id || '';
    if (!callId) return;
    let taskId = payload.task_id || '';
    let state = payload.state || '';

    // 心跳：无 task_id 的 running 态（仅保活看门狗），UI 忽略不产生噪音
    if (!taskId && state === 'running') return;

    let entry = subagentCards[callId];
    if (!entry) {
        // 审计问题 28b（双保险）：非 spawned 态也惰性重建进度卡——切回运行中
        // 会话时 clearChat 已复位 subagentCards，Module 侧缓存重发若丢失头部
        // spawned 事件（或重放窗口竞态），后续进度不再被整体忽略；行级幂等
        // 由 updateSubagentTaskRow 的 terminal 守卫保障
        entry = subagentCards[callId] = { group: createSubagentGroup(callId), tasks: {} };
    }

    if (!taskId) {
        // 聚合终态（派发结束）：results.tasks 携带各任务 status，未终态的行按此收尾
        let results = (payload.results && typeof payload.results === 'object')
            ? (payload.results.tasks || []) : [];
        for (let i = 0; i < results.length; i++) {
            let r = results[i];
            if (!r || !r.task_id) continue;
            if (r.status === 'ok') {
                updateSubagentTaskRow(entry, r.task_id, 'done', '');
            } else if (r.status === 'timeout') {
                updateSubagentTaskRow(entry, r.task_id, 'timeout', r.error || '');
            } else if (r.status === 'stopped') {
                updateSubagentTaskRow(entry, r.task_id, 'stopped', '');
            } else if (r.status === 'error') {
                updateSubagentTaskRow(entry, r.task_id, 'error', r.error || '');
            }
        }
        // 聚合到达即派发结束：残余未终态行（进度消息丢失）统一按 done 收尾防御
        for (var id in entry.tasks) {
            if (Object.prototype.hasOwnProperty.call(entry.tasks, id) && !entry.tasks[id].terminal) {
                updateSubagentTaskRow(entry, id, 'done', '');
            }
        }
    } else {
        updateSubagentTaskRow(entry, taskId, state, payload.message || '');
    }
    updateSubagentGroupHeader(entry);
    scrollToBottom();
}

// —— 被 C++ 调用的函数 ——
// 通过 DAAgentWebChannel::callJS(evaluateJavaScript) 调用

function appendUserMessage(text) {
    flushAgentMessage();
    closeToolGroup();
    let bubble = createMessageBubble('user');
    bubble.dataset.rawText = text;
    bubble.innerHTML = md.render(text);
    getRenderTarget().appendChild(bubble);
    scrollToBottom();
}

function appendToken(text) {
    // 重试成功——隐藏重试状态条
    if (retryStatusBar) {
        hideRetryStatus();
    }
    // Agent 文本恢复——关闭当前工具分组，使后续工具调用开启新分组。
    // 这样被 agent 叙述分隔的两组工具调用各自成组，而非混在一起。
    if (currentToolGroup) closeToolGroup();
    if (!currentAgentMsg) {
        currentAgentMsg = createMessageBubble('agent');
        document.getElementById('messages').appendChild(currentAgentMsg);
    }
    currentAgentMsg.dataset.rawText += text;

    // 防抖：不要每个 token 都重新渲染整条气泡（避免 O(n²) 全量重解析）
    if (renderTimer) clearTimeout(renderTimer);
    renderTimer = setTimeout(function() {
        if (currentAgentMsg) {
            currentAgentMsg.innerHTML = md.render(currentAgentMsg.dataset.rawText);
            scrollToBottom();
        }
        renderTimer = null;
    }, RENDER_DEBOUNCE_MS);
}

function finalizeAgentMessage(fullText) {
    // 重试/流结束——隐藏重试状态条
    if (retryStatusBar) {
        hideRetryStatus();
    }
    // 消息结束，立即做最终渲染（取消未触发的防抖）
    if (renderTimer) {
        clearTimeout(renderTimer);
        renderTimer = null;
    }
    closeToolGroup();
    let trimmed = fullText ? fullText.trim() : '';
    if (currentAgentMsg) {
        if (trimmed) {
            currentAgentMsg.dataset.rawText = fullText;
            currentAgentMsg.innerHTML = md.render(fullText);
        } else {
            // 空回复——移除遗留的空气泡，避免留白
            currentAgentMsg.remove();
        }
        currentAgentMsg = null;
    } else if (trimmed) {
        // 防御：无打开的气泡但有内容时创建一个（正常路径不会走到，token 已先行创建）
        let bubble = createMessageBubble('agent');
        bubble.innerHTML = md.render(fullText);
        document.getElementById('messages').appendChild(bubble);
    }
    scrollToBottom();
}

function appendToolCall(toolName, args) {
    // 工具调用开始前定稿 agent 文本气泡，使叙述文本排在工具分组之前（时序正确）。
    flushAgentMessage();
    ensureToolGroup();
    let card = createToolCard(toolName, args);
    currentToolGroup.querySelector('.tool-group-body').appendChild(card);
    pendingToolCards.push({card: card, toolName: toolName});
    updateToolGroupHeader();
    scrollToBottom();
}

function appendToolResult(toolName, result) {
    // FIFO 按工具名匹配待处理卡片（协议保证顺序执行，至多一张待结果）
    let idx = pendingToolCards.findIndex(function(p) { return p.toolName === toolName; });
    if (idx < 0) return;  // 无匹配卡片——忽略（迟到/重复结果）
    let entry = pendingToolCards.splice(idx, 1)[0];
    updateToolCardResult(entry.card, result);
    updateToolGroupHeader();
    scrollToBottom();
}

// 工具排队状态（决策点 2 ③，审计问题 12）：全局执行队列的等待可解释——
// position>0 卡片摘要显示"排队中 #N"（跨会话队头等待不再被误判为卡死），
// position==0 表示出队开始执行，恢复"运行中"。从后向前找最近一张同名
// 待结果卡（FIFO 语义与 appendToolResult 对齐）
function markToolQueued(toolName, position) {
    for (var i = pendingToolCards.length - 1; i >= 0; i--) {
        if (pendingToolCards[i].toolName === toolName) {
            var summary = pendingToolCards[i].card.querySelector('.card-summary');
            if (summary) {
                summary.textContent = (position > 0)
                    ? (i18n.toolQueued || 'queued') + ' #' + position  // cn:排队中 #N
                    : (i18n.toolRunning || 'running');                 // cn:运行中
            }
            break;
        }
    }
}

function appendQuestion(text, options, submitLabel, customPlaceholder, multiSelect) {
    flushAgentMessage();
    closeToolGroup();
    let qBubble = createMessageBubble('question');
    if (multiSelect) { qBubble.classList.add('multi'); }
    qBubble.innerHTML = '<p>' + md.renderInline(text) + '</p>';

    // 选项按钮：单选点击切换选中（清除其他），多选 toggle 当前
    let btnContainer = document.createElement('div');
    btnContainer.className = 'question-options';
    options.forEach(function(opt) {
        let btn = document.createElement('button');
        btn.textContent = opt;
        btn.onclick = function() {
            if (multiSelect) {
                // 多选：toggle 当前选中
                btn.classList.toggle('selected');
            } else {
                // 单选：清除其他选中，标记当前
                let allBtns = btnContainer.querySelectorAll('button');
                allBtns.forEach(function(b) { b.classList.remove('selected'); });
                btn.classList.add('selected');
            }
        };
        btnContainer.appendChild(btn);
    });
    qBubble.appendChild(btnContainer);

    // 自定义回答输入框（与选项并存）
    let customContainer = document.createElement('div');
    customContainer.className = 'question-custom';
    let customInput = document.createElement('textarea');
    customInput.placeholder = customPlaceholder;
    customInput.rows = 2;
    customContainer.appendChild(customInput);
    qBubble.appendChild(customContainer);

    // 提交按钮：确认后才提交
    let submitBtn = document.createElement('button');
    submitBtn.className = 'question-submit';
    submitBtn.textContent = submitLabel;
    submitBtn.onclick = function() {
        let customText = customInput.value.trim();
        let answer = '';
        if (multiSelect) {
            // 多选：所有选中选项 + 自定义文字，换行拼接
            let selected = [];
            btnContainer.querySelectorAll('button.selected').forEach(function(b) {
                selected.push(b.textContent);
            });
            if (customText) { selected.push(customText); }
            answer = selected.join('\n');
        } else {
            // 单选：自定义文字优先，否则选中选项
            answer = customText || (function() {
                let s = btnContainer.querySelector('button.selected');
                return s ? s.textContent : '';
            })();
        }
        if (!answer) {
            // 未选择也未输入——聚焦输入框引导用户
            customInput.focus();
            return;
        }
        chatBridge.onUserSelect(answer);
        qBubble.classList.add('answered');
        // 提交后禁用所有交互元素
        let allBtns = btnContainer.querySelectorAll('button');
        allBtns.forEach(function(b) { b.disabled = true; });
        customInput.disabled = true;
        submitBtn.disabled = true;
    };
    qBubble.appendChild(submitBtn);

    getRenderTarget().appendChild(qBubble);
    scrollToBottom();
    // 返回气泡引用：历史重放的 question 分支直接持有节点做禁用/answered/答案
    // 追加（替代旧的容器内 querySelector:last-of-type 查询——该查询在分段
    // prepend 的 DocumentFragment 渲染目标上不可用，且存在兄弟节点误匹配风险）
    return qBubble;
}

function clearChat() {
    hideRetryStatus();
    if (renderTimer) {
        clearTimeout(renderTimer);
        renderTimer = null;
    }
    document.getElementById('messages').innerHTML = '';
    currentAgentMsg = null;
    currentToolGroup = null;
    pendingToolCards = [];
    subagentCards = {};
    // 分段渲染状态一并复位：未渲染的更早事件与哨兵随会话清空而失效，
    // 防止切换会话后"加载更早"把旧会话事件渲染进新聊天区
    pendingEarlierEvents = [];
    removeLoadEarlierSentinel();
}

// —— 输入区/状态栏 web 化（被 C++ 经 DAAgentWebChannel::callJS 调用）——
// 契约：C++ 仍是唯一 i18n 拥有者，所有显示文案由 C++ tr() 格式化后推送；
//       静态标签在握手时经 setI18nLabels 一次性注入，动态串走对应方法。

// 握手后注入静态 UI 标签。labels 为对象：{send,stop,ready,thinking,stopping,
// inputPlaceholder,tokenEmpty,popoverInput,popoverOutput,popoverTotal,
// popoverWindow,popoverSource,popoverSourceUnknown}。
function setI18nLabels(labels) {
    if (!labels || typeof labels !== 'object') return;
    for (var k in labels) {
        if (Object.prototype.hasOwnProperty.call(labels, k)) {
            i18n[k] = labels[k];
        }
    }
    // 应用 placeholder 到输入框
    var ta = document.getElementById('input-edit');
    if (ta && i18n.inputPlaceholder) { ta.placeholder = i18n.inputPlaceholder; }
    // 同步一次按钮初始文案（ready 态）
    applySendButtonState();
    // token 空态文案
    var tl = document.getElementById('token-label');
    if (tl && (tl.textContent === 'tokens: -' || !tl.textContent)) {
        tl.textContent = i18n.tokenEmpty;
    }
    // 模型选择器触发按钮文案（空态用 i18n.modelEmpty）
    updateModelTrigger();
}

// busy 打包：true→按钮 Stop(红)+输入禁用+状态 thinking；false→按钮 Send+输入启用+状态 ready。
// JS 内聚解释"忙态长什么样"，C++ 只发一个 bool。
function setBusy(busy) {
    agentBusy = busy;
    applySendButtonState();
    var btn = document.getElementById('send-btn');
    // 清除 starting/stopping 过渡态设的 disabled，恢复按钮可点击
    // （busy 时按钮为 Stop 需可点击停止，非 busy 时为 Send 需可点击发送）
    if (btn) { btn.disabled = false; }
    var ta = document.getElementById('input-edit');
    if (ta) { ta.disabled = busy; }
    setStatus(busy ? i18n.thinking : i18n.ready);
}

// 停止过渡态：按钮禁用（防重复点）+状态 stopping。由 onStopClicked 触发，
// 持续到 onAgentBusy(false)/onAgentReady 恢复。
function setStopping() {
    agentBusy = false;  // 非思考态，但按钮要禁用
    var btn = document.getElementById('send-btn');
    if (btn) { btn.disabled = true; btn.classList.remove('busy'); }
    var ta = document.getElementById('input-edit');
    if (ta) { ta.disabled = true; }
    setStatus(i18n.stopping);
}

// 启动过渡态：按钮+输入禁用 + 状态 starting。由 C++ agentStarting 信号触发，
// 持续到 onAgentReady（→setBusy(false)）/onAgentBusy(false) 恢复。
// 区别于 setStopping（终止中）——启动中是等待子进程冷启动完成。
function setStarting() {
    agentBusy = false;  // 非思考态
    var btn = document.getElementById('send-btn');
    if (btn) { btn.disabled = true; btn.classList.remove('busy'); btn.textContent = i18n.send; }
    var ta = document.getElementById('input-edit');
    if (ta) { ta.disabled = true; }
    setStatus(i18n.starting);
}

// 设置状态文案（左）。busy/stopping 内部调，也供 C++ 直接推过渡态。
function setStatus(text) {
    var el = document.getElementById('status-text');
    if (el) { el.textContent = text || ''; }
}

// —— 两级模型选择器（被 C++ 经 DAAgentWebChannel::callJS 调用）——
// C++ 推 flat 可用模型列表 + 激活供应商/模型；JS 按 provider 分组渲染两级选择器。

// 推送可用模型列表（flat 数组 {provider,model,context_window,max_output_tokens}）。
// JS 按 provider 分组（保持首次出现顺序），刷新下拉面板。
function setAvailableModels(models) {
    var order = [];
    var map = {};
    (models || []).forEach(function(m) {
        var p = (m && m.provider) ? m.provider : '';
        if (!(p in map)) { map[p] = []; order.push(p); }
        map[p].push(m);
    });
    modelSelectorData = order.map(function(p) {
        return { provider: p, models: map[p] };
    });
    updateModelTrigger();
    // 若下拉正打开，刷新内容以反映新列表
    var dd = document.getElementById('model-dropdown');
    if (dd && !dd.hasAttribute('hidden')) { renderModelDropdown(); }
}

// 推送激活供应商+模型：更新触发按钮文案 + 刷新选中高亮。
function setActiveModel(provider, model) {
    activeProvider = provider || '';
    activeModel = model || '';
    updateModelTrigger();
    var dd = document.getElementById('model-dropdown');
    if (dd && !dd.hasAttribute('hidden')) { renderModelDropdown(); }
}

// 更新触发按钮文案：空列表/空模型显示 i18n.modelEmpty，否则 "provider · model"。
function updateModelTrigger() {
    var el = document.getElementById('model-trigger-text');
    if (!el) return;
    var text = '';
    if (!modelSelectorData.length || !activeModel) {
        text = i18n.modelEmpty;
    } else if (activeProvider) {
        text = activeProvider + ' \u00b7 ' + activeModel;
    } else {
        text = activeModel;
    }
    el.textContent = text;
    var trig = document.getElementById('model-trigger');
    if (trig) { trig.title = i18n.modelSelectTip || ''; }
}

// 打开下拉：默认进供应商层（若激活供应商存在则直接进其模型层，优化常见切换）。
function openModelDropdown() {
    var dd = document.getElementById('model-dropdown');
    if (!dd) return;
    var hasActiveProvider = activeProvider && modelSelectorData.some(function(g) { return g.provider === activeProvider; });
    modelDropdownView = hasActiveProvider ? 'models' : 'providers';
    modelDropdownProvider = hasActiveProvider ? activeProvider : '';
    renderModelDropdown();
    dd.removeAttribute('hidden');
    var trig = document.getElementById('model-trigger');
    if (trig) { trig.classList.add('open'); }
}

function closeModelDropdown() {
    var dd = document.getElementById('model-dropdown');
    if (dd) { dd.setAttribute('hidden', ''); }
    var trig = document.getElementById('model-trigger');
    if (trig) { trig.classList.remove('open'); }
}

// ===========================================================================
// 权限模式选择器（permission-layer P1，镜像模型选择器交互）
// ===========================================================================

// 模式 → 触发按钮/下拉行文案
function modeLabel(mode) {
    if (mode === 'yolo') return i18n.modeYolo;
    if (mode === 'manual') return i18n.modeManual;
    return i18n.modeAuto;
}

// 模式 → 行为提示（下拉行副标题 + 触发按钮 title）
function modeTip(mode) {
    if (mode === 'yolo') return i18n.modeYoloTip;
    if (mode === 'manual') return i18n.modeManualTip;
    return i18n.modeAutoTip;
}

// C++ 经 setPermissionMode 推送当前模式（启动推送/热切换回显）
function setPermissionMode(mode) {
    activePermissionMode = (mode === 'auto' || mode === 'manual') ? mode : 'yolo';
    updateModeTrigger();
    var dd = document.getElementById('mode-dropdown');
    if (dd && !dd.hasAttribute('hidden')) { renderModeDropdown(); }
}

// 更新触发按钮文案（data-mode 供 CSS 按模式着色）
function updateModeTrigger() {
    var trig = document.getElementById('mode-trigger');
    if (!trig) return;
    var text = document.getElementById('mode-trigger-text');
    if (text) { text.textContent = modeLabel(activePermissionMode); }
    // \uFF1A = 全角冒号「：」，用转义避免 lupdate 按 GBK 解析 UTF-8 字节导致语法错误
    trig.title = (i18n.modeSelectTip || '') + '\uFF1A' + modeTip(activePermissionMode);
    trig.dataset.mode = activePermissionMode;
}

function openModeDropdown() {
    renderModeDropdown();
    var dd = document.getElementById('mode-dropdown');
    if (dd) { dd.removeAttribute('hidden'); }
    var trig = document.getElementById('mode-trigger');
    if (trig) { trig.classList.add('open'); }
    // 展开模式选择器时关闭模型下拉，避免两面板叠加
    closeModelDropdown();
}

function closeModeDropdown() {
    var dd = document.getElementById('mode-dropdown');
    if (dd && !dd.hasAttribute('hidden')) { dd.setAttribute('hidden', ''); }
    var trig = document.getElementById('mode-trigger');
    if (trig) { trig.classList.remove('open'); }
}

// 渲染三行模式列表（当前模式高亮）
function renderModeDropdown() {
    var dd = document.getElementById('mode-dropdown');
    if (!dd) return;
    dd.innerHTML = '';
    ['auto', 'manual', 'yolo'].forEach(function(m) {
        var row = document.createElement('button');
        row.type = 'button';
        row.className = 'mode-dd-row';
        if (m === activePermissionMode) { row.classList.add('active-mode'); }
        var lab = document.createElement('span');
        lab.className = 'mode-dd-label';
        lab.textContent = modeLabel(m);
        var tip = document.createElement('span');
        tip.className = 'mode-dd-tip';
        tip.textContent = modeTip(m);
        row.appendChild(lab);
        row.appendChild(tip);
        row.addEventListener('click', function(e) {
            e.stopPropagation();
            onModeRowClick(m);
        });
        dd.appendChild(row);
    });
}

// 模式行点击：切 yolo 先二次确认（含"代码执行将不再询问"警示）
function onModeRowClick(mode) {
    closeModeDropdown();
    if (mode === activePermissionMode) { return; }
    if (mode === 'yolo') {
        showYoloSwitchConfirm();
        return;
    }
    if (chatBridge && typeof chatBridge.onPermissionModeSelect === 'function') {
        chatBridge.onPermissionModeSelect(mode);
    }
}

// 会话内切入 yolo 的二次确认卡（JS 侧发起；A13 启动确认由 C++ 经
// appendStartupYoloConfirm 推送，二者共用 .mode-confirm 样式）
function showYoloSwitchConfirm() {
    var container = document.getElementById('messages');
    if (!container) return;
    var card = document.createElement('div');
    card.className = 'mode-confirm';
    var text = document.createElement('div');
    text.className = 'mode-confirm-text';
    text.textContent = i18n.modeYoloConfirm;
    card.appendChild(text);
    var actions = document.createElement('div');
    actions.className = 'mode-confirm-actions';
    var okBtn = document.createElement('button');
    okBtn.type = 'button';
    okBtn.className = 'mode-confirm-ok';
    okBtn.textContent = i18n.modeYoloConfirmOk;
    okBtn.addEventListener('click', function() {
        card.remove();
        if (chatBridge && typeof chatBridge.onPermissionModeSelect === 'function') {
            chatBridge.onPermissionModeSelect('yolo');
        }
    });
    var cancelBtn = document.createElement('button');
    cancelBtn.type = 'button';
    cancelBtn.className = 'mode-confirm-cancel';
    cancelBtn.textContent = i18n.modeYoloConfirmCancel;
    cancelBtn.addEventListener('click', function() { card.remove(); });
    actions.appendChild(okBtn);
    actions.appendChild(cancelBtn);
    card.appendChild(actions);
    container.appendChild(card);
    scrollToBottom();
}

// C++ 推送：启动读到显式设置的 yolo（A13）——弹一次确认卡，用户拒绝则降级 auto。
// 默认 yolo（未显式设置）不弹卡；与会话内切换确认不同，响应经
// onModeConfirmResponse(bool) 回 C++ 决策。
function appendStartupYoloConfirm(text, okLabel, cancelLabel) {
    var container = document.getElementById('messages');
    if (!container) return;
    var card = document.createElement('div');
    card.className = 'mode-confirm';
    var textEl = document.createElement('div');
    textEl.className = 'mode-confirm-text';
    textEl.textContent = text;
    card.appendChild(textEl);
    var actions = document.createElement('div');
    actions.className = 'mode-confirm-actions';
    var okBtn = document.createElement('button');
    okBtn.type = 'button';
    okBtn.className = 'mode-confirm-ok';
    okBtn.textContent = okLabel || i18n.modeYoloConfirmOk;
    okBtn.addEventListener('click', function() {
        card.remove();
        if (chatBridge && typeof chatBridge.onModeConfirmResponse === 'function') {
            chatBridge.onModeConfirmResponse(true);
        }
    });
    var cancelBtn = document.createElement('button');
    cancelBtn.type = 'button';
    cancelBtn.className = 'mode-confirm-cancel';
    cancelBtn.textContent = cancelLabel || i18n.modeYoloConfirmCancel;
    cancelBtn.addEventListener('click', function() {
        card.remove();
        if (chatBridge && typeof chatBridge.onModeConfirmResponse === 'function') {
            chatBridge.onModeConfirmResponse(false);
        }
    });
    actions.appendChild(okBtn);
    actions.appendChild(cancelBtn);
    card.appendChild(actions);
    container.appendChild(card);
    scrollToBottom();
}

// ===========================================================================
// 工具审批卡（permission-layer P1，ask 决策的 HITL 载体）
// ===========================================================================

// C++ 推送审批请求：渲染审批卡（操作摘要 + 批准/拒绝按钮）。
// payload: {tool, args, tier, rememberable}；rememberable 仅 file_write（A5），
// 控制是否渲染"批准并本会话记住"按钮（code_exec 无此按钮）。
function appendToolApproval(callId, payload) {
    payload = payload || {};
    var toolName = payload.tool || '';
    var args = (payload.args && typeof payload.args === 'object') ? payload.args : {};
    var tier = payload.tier || 'unknown';
    var rememberable = !!payload.rememberable;
    var subagent = payload.subagent || '';  // 子 agent 来源上下文（Q18；主 agent 调用为空）

    var card = document.createElement('div');
    card.className = 'approval-card pending';
    card.dataset.callId = callId;
    card.dataset.tier = tier;

    // 头部：图标 + 工具名 + "需要你的批准"
    var head = document.createElement('div');
    head.className = 'approval-head';
    var icon = document.createElement('span');
    icon.className = 'approval-icon';
    icon.textContent = '\uD83D\uDEE1\uFE0F';
    head.appendChild(icon);
    var title = document.createElement('span');
    title.className = 'approval-title';
    title.textContent = toolName + ' ' + (i18n.approvalNeeds || 'needs your approval');
    head.appendChild(title);
    card.appendChild(head);

    // 子 agent 来源上下文（Q18）：审批由子 agent 发起时渲染"来自子 Agent：explore #1"
    if (subagent) {
        var from = document.createElement('div');
        from.className = 'approval-from-subagent';
        from.textContent = fmtTmpl(i18n.approvalFromSubagent || 'From subagent: %1', subagent);
        card.appendChild(from);
    }

    // 摘要区：代码执行显示代码预览/脚本路径，文件写入显示路径+参数摘要
    var body = document.createElement('div');
    body.className = 'approval-body';
    if (typeof args.code === 'string' && args.code) {
        var pre = document.createElement('pre');
        pre.className = 'approval-code';
        var codeText = args.code;
        var lines = codeText.split('\n');
        if (lines.length > 15) {
            var more = (i18n.approvalCodeMoreLines || '%1 more lines')
                .replace('%1', String(lines.length - 15));
            codeText = lines.slice(0, 15).join('\n') + '\n\u2026 (' + more + ')';
        }
        pre.textContent = codeText;
        body.appendChild(pre);
    } else if (typeof args.path === 'string' && args.path) {
        var pathEl = document.createElement('div');
        pathEl.className = 'approval-path';
        pathEl.textContent = args.path;
        body.appendChild(pathEl);
    } else {
        var shown = {};
        for (var k in args) {
            if (!args.hasOwnProperty(k)) continue;
            var v = args[k];
            if (typeof v === 'string' && v.length > 200) { v = v.slice(0, 200) + '\u2026'; }
            shown[k] = v;
        }
        var pre2 = document.createElement('pre');
        pre2.className = 'approval-json';
        pre2.textContent = JSON.stringify(shown, null, 2);
        body.appendChild(pre2);
    }
    card.appendChild(body);

    // 操作区：批准 / （可选）批准并记住 / 拒绝
    var actions = document.createElement('div');
    actions.className = 'approval-actions';
    var approveBtn = document.createElement('button');
    approveBtn.type = 'button';
    approveBtn.className = 'approval-approve';
    approveBtn.textContent = i18n.approvalApprove;
    approveBtn.addEventListener('click', function() {
        respondToolApproval(card, callId, true, false);
    });
    actions.appendChild(approveBtn);
    if (rememberable) {
        var rememberBtn = document.createElement('button');
        rememberBtn.type = 'button';
        rememberBtn.className = 'approval-remember';
        rememberBtn.textContent = i18n.approvalApproveRemember;
        rememberBtn.addEventListener('click', function() {
            respondToolApproval(card, callId, true, true);
        });
        actions.appendChild(rememberBtn);
    }
    var denyBtn = document.createElement('button');
    denyBtn.type = 'button';
    denyBtn.className = 'approval-deny';
    denyBtn.textContent = i18n.approvalDeny;
    denyBtn.addEventListener('click', function() {
        respondToolApproval(card, callId, false, false);
    });
    actions.appendChild(denyBtn);
    card.appendChild(actions);

    var container = document.getElementById('messages');
    if (container) { container.appendChild(card); }
    scrollToBottom();
}

// 审批卡裁决：回传 C++（sendToolApproval），禁用按钮并标记结果
function respondToolApproval(card, callId, approved, remember) {
    if (card.dataset.resolved === '1') return;  // 防重复点击
    card.dataset.resolved = '1';
    if (chatBridge && typeof chatBridge.onToolApproval === 'function') {
        chatBridge.onToolApproval(callId, approved, remember);
    }
    card.classList.remove('pending');
    card.classList.add(approved ? 'approved' : 'denied');
    var buttons = card.querySelectorAll('.approval-actions button');
    for (var i = 0; i < buttons.length; i++) { buttons[i].disabled = true; }
    var note = document.createElement('div');
    note.className = 'approval-note';
    if (approved && remember) {
        note.textContent = i18n.approvalApprovedRemembered;
    } else if (approved) {
        note.textContent = i18n.approvalApproved;
    } else {
        note.textContent = i18n.approvalDenied;
    }
    card.appendChild(note);
}

// C++ 推送审批作废（子进程退出/崩溃/切换会话清理）：撤卡
function dismissToolApproval(callId) {
    var container = document.getElementById('messages');
    if (!container) return;
    var cards = container.querySelectorAll('.approval-card');
    for (var i = 0; i < cards.length; i++) {
        if (cards[i].dataset.callId === callId) {
            cards[i].remove();
            break;
        }
    }
}

// C++ 推送挂起问题卡作废（子进程退出/崩溃/用户 Stop/桥退役）：移除未回答的
// 问题卡。已回答的历史卡（.answered，含重放渲染的静态卡）不受影响。
function dismissQuestion() {
    var container = document.getElementById('messages');
    if (!container) return;
    var cards = container.querySelectorAll('.message-bubble.question');
    for (var i = 0; i < cards.length; i++) {
        if (!cards[i].classList.contains('answered')) {
            cards[i].remove();
        }
    }
}

// 渲染下拉面板（按 modelDropdownView 分发）。
function renderModelDropdown() {
    var dd = document.getElementById('model-dropdown');
    if (!dd) return;
    dd.innerHTML = '';
    if (!modelSelectorData.length) {
        var empty = document.createElement('div');
        empty.className = 'model-dd-empty';
        empty.textContent = i18n.modelEmpty;
        dd.appendChild(empty);
        return;
    }
    if (modelDropdownView === 'models') {
        renderModelList(dd, modelDropdownProvider);
    } else {
        renderProviderList(dd);
    }
}

// 第一层：供应商列表。每项显示供应商名 + 模型数 + 右箭头，点击进第二层。
function renderProviderList(dd) {
    var title = document.createElement('div');
    title.className = 'model-dd-title';
    title.textContent = i18n.modelProvidersTitle;
    dd.appendChild(title);
    modelSelectorData.forEach(function(g) {
        var row = document.createElement('button');
        row.className = 'model-dd-row provider-row';
        row.type = 'button';
        if (g.provider === activeProvider) { row.classList.add('active-provider'); }
        var label = document.createElement('span');
        label.className = 'model-dd-label';
        label.textContent = g.provider || '?';
        var count = document.createElement('span');
        count.className = 'model-dd-meta';
        count.textContent = g.models.length;
        var chev = document.createElement('span');
        chev.className = 'model-dd-chevron';
        chev.innerHTML = '<svg viewBox="0 0 24 24" width="14" height="14" aria-hidden="true">' +
            '<path fill="currentColor" d="m13.172 12l-4.95-4.95l1.414-1.413L16 12l-6.364 6.364l-1.414-1.415z"/></svg>';
        row.appendChild(label);
        row.appendChild(count);
        row.appendChild(chev);
        row.addEventListener('click', function() {
            modelDropdownView = 'models';
            modelDropdownProvider = g.provider;
            renderModelDropdown();
        });
        dd.appendChild(row);
    });
}

// 第二层：指定供应商的模型列表。顶部返回按钮回供应商层。
function renderModelList(dd, provider) {
    var back = document.createElement('button');
    back.className = 'model-dd-back';
    back.type = 'button';
    back.innerHTML = '<svg viewBox="0 0 24 24" width="14" height="14" aria-hidden="true">' +
        '<path fill="currentColor" d="M10 7l5 5l-5 5z"/></svg>' +
        '<span>' + escapeHtml(provider || '') + '</span>';
    back.addEventListener('click', function() {
        modelDropdownView = 'providers';
        modelDropdownProvider = '';
        renderModelDropdown();
    });
    dd.appendChild(back);
    var group = null;
    for (var i = 0; i < modelSelectorData.length; i++) {
        if (modelSelectorData[i].provider === provider) { group = modelSelectorData[i]; break; }
    }
    if (!group) { return; }
    group.models.forEach(function(m) {
        var row = document.createElement('button');
        row.className = 'model-dd-row model-row';
        row.type = 'button';
        if (m.model === activeModel && provider === activeProvider) {
            row.classList.add('active-model');
        }
        var label = document.createElement('span');
        label.className = 'model-dd-label';
        label.textContent = m.model || '';
        var meta = document.createElement('span');
        meta.className = 'model-dd-meta';
        var cw = (typeof m.context_window === 'number') ? m.context_window : 0;
        meta.textContent = cw > 0 ? (cw >= 1000 ? (Math.round(cw / 1000) + 'K') : String(cw)) : '';
        var check = document.createElement('span');
        check.className = 'model-dd-check';
        check.textContent = '\u2713';  // ✓
        row.appendChild(label);
        row.appendChild(meta);
        row.appendChild(check);
        row.addEventListener('click', function() {
            closeModelDropdown();
            if (chatBridge && typeof chatBridge.onModelSelect === 'function') {
                chatBridge.onModelSelect(provider, m.model);
            }
        });
        dd.appendChild(row);
    });
}

// 设置 token 计量（右）+ 缓存明细供 popover。label 已由 C++ 格式化。
function setTokenStats(label, inT, outT, total, window, source) {
    var el = document.getElementById('token-label');
    if (el) { el.textContent = label || ''; }
    tokenStatsCache = { inT: inT, outT: outT, total: total, window: window, source: source };
    // 若 popover 正在显示，同步刷新内容
    var pop = document.getElementById('token-popover');
    if (pop && !pop.hasAttribute('hidden')) { rebuildTokenPopover(); }
}

// 复位 token 计量到无活跃会话初始态（新会话/清空时）。
function resetTokenStats() {
    var el = document.getElementById('token-label');
    if (el) { el.textContent = i18n.tokenEmpty; }
    tokenStatsCache = null;
    var pop = document.getElementById('token-popover');
    if (pop) { pop.innerHTML = ''; pop.setAttribute('hidden', ''); }
}

// 聚焦输入框（新会话/切换后）。
function focusInput() {
    var ta = document.getElementById('input-edit');
    if (ta) { ta.focus(); }
}

// —— 内部辅助 ——
// 按 agentBusy 同步 send-btn 文案/样式（busy→Stop 红，否则 Send）。disabled 由调用方管。
function applySendButtonState() {
    var btn = document.getElementById('send-btn');
    if (!btn) return;
    if (agentBusy) {
        btn.textContent = i18n.stop;
        btn.classList.add('busy');
    } else {
        btn.textContent = i18n.send;
        btn.classList.remove('busy');
    }
}

// 构建 token 明细 popover 5 行（input/output/total/window + 分隔 + source）。
// popover 标签为 Qt 模板串 tr("input: %1")（复用既有翻译），JS 做 %1→值 替换。
function rebuildTokenPopover() {
    var pop = document.getElementById('token-popover');
    if (!pop) return;
    if (!tokenStatsCache) { pop.innerHTML = ''; return; }
    var c = tokenStatsCache;
    var winVal = c.window > 0 ? c.window : -1;
    var srcVal = c.source ? c.source : i18n.popoverSourceUnknown;
    pop.innerHTML =
        '<div class="popover-row">' + fmtTmpl(i18n.popoverInput, c.inT) + '</div>' +
        '<div class="popover-row">' + fmtTmpl(i18n.popoverOutput, c.outT) + '</div>' +
        '<div class="popover-row">' + fmtTmpl(i18n.popoverTotal, c.total) + '</div>' +
        '<div class="popover-row">' + fmtTmpl(i18n.popoverWindow, winVal) + '</div>' +
        '<div class="popover-sep"></div>' +
        '<div class="popover-row">' + fmtTmpl(i18n.popoverSource, srcVal) + '</div>';
}

// Qt 模板串 %1→值 替换（与 C++ .arg() 等价的 JS 侧哑替换）
function fmtTmpl(tmpl, val) {
    return String(tmpl).replace('%1', val);
}

// plan-04 step6: 批量重放历史会话记录到聊天界面（性能重构版）。
// events 为 C++ DAAgentWebChannel::loadHistory 合并后的 UI 事件数组：
//   {type:"user",message:{role,content}},
//   {type:"assistant",message:{role,content}},
//   {type:"tool",toolName,args,result,toolCallId},
//   {type:"question",toolName,args,result:{answer},toolCallId},
//   {type:"usage",...}/type:"summary" 跳过不渲染。
// 性能设计（诊断结论见文件头注释）：
//   1) 超过 HISTORY_CHUNK_SIZE 的事件只渲染尾部一段，更早的进 pendingEarlierEvents，
//      由顶部哨兵 + 滚动到顶触发 loadEarlierChunk 分批 prepend；
//   2) 重放窗口内 suppressAutoScroll 抑制逐事件 scrollToBottom（O(n²) reflow 根源），
//      收尾统一滚动一次。
// CRITICAL1: assistant 分支 createMessageBubble 不挂 DOM，必须 appendChild。
// MAJOR2: 配对由 C++ 完成，JS 直接读 ev.toolName/args/result（不再读 _toolName/_toolArgs）。
// MAJOR5 + 契约9: ask_user 历史用 appendQuestion 渲染问题气泡，然后内联 DOM 操作
//                 禁用按钮 + 加 answered class + 追加答案文本（chat.js 无 markQuestionAnswered）。
function loadHistory(events) {
    clearChat();
    if (!events || !events.length) {
        scrollToBottom();
        return;
    }
    let tail = events;
    if (events.length > HISTORY_CHUNK_SIZE) {
        pendingEarlierEvents = events.slice(0, events.length - HISTORY_CHUNK_SIZE);
        tail = events.slice(events.length - HISTORY_CHUNK_SIZE);
    } else {
        pendingEarlierEvents = [];
    }
    suppressAutoScroll = true;
    try {
        renderHistoryEvents(tail);
    } finally {
        suppressAutoScroll = false;
    }
    if (pendingEarlierEvents.length > 0) {
        insertLoadEarlierSentinel();
    }
    // concurrent-sessions 修复：收尾关闭最后的工具组（所有卡片已带 result，
    // 标记 completed），避免重放后末组永远显示 running 状态。
    // 例外（审计问题 7）：存在在途卡片（C++ 未配对 tool_call flush 的 running
    // 事件）时保持分组 active——实时 tool_result 稍后到达依 FIFO 补全，
    // 不得提前关组走 incomplete 误标路径（问题 28a 同款机制）
    if (pendingToolCards.length === 0) {
        closeToolGroup();
    }
    scrollToBottom();
}

// 渲染一批 UI 事件到 getRenderTarget()（#messages 或分段 prepend 的 detached 容器）。
// 事件自包含（C++ 已完成 tool_call/tool_result 配对），任意 chunk 边界安全。
function renderHistoryEvents(evs) {
    for (const ev of evs) {
        const t = ev.type;
        if (t === 'user') {
            const content = (ev.message && ev.message.content) ? ev.message.content : '';
            appendUserMessage(content);
        } else if (t === 'assistant') {
            // CRITICAL1: createMessageBubble 不挂 DOM，必须 appendChild，否则气泡游离、agent 文本不可见
            const content = (ev.message && ev.message.content) ? String(ev.message.content) : '';
            if (!content.trim()) {
                // 空 content（纯 tool_calls 的 assistant 记录）跳过，不留白气泡
                continue;
            }
            // concurrent-sessions 修复：重放文本前关闭当前工具组，对齐实时渲染的时序语义
            //（appendToken/finalizeAgentMessage 均会 closeToolGroup）。若不关闭，
            // ensureToolGroup 会把本会话所有工具卡片持续追加进第一个工具组
            //（其 DOM 位置在首条文本之前），导致"工具汇总在前、思考文本孤立在后"。
            closeToolGroup();
            const bubble = createMessageBubble('agent');
            bubble.dataset.rawText = content;
            bubble.innerHTML = md.render(content);
            getRenderTarget().appendChild(bubble);  // ← 必须挂到 DOM
        } else if (t === 'tool') {
            // MAJOR2: 读合并后字段 toolName/args/result（C++ 已配对）
            const toolName = ev.toolName || 'tool';
            const args = (ev.args && typeof ev.args === 'object') ? ev.args : {};
            if (ev.running === true) {
                // 在途工具调用（审计问题 7：切回运行中会话，C++ 把末尾未配对
                // tool_call 透传为 running 态事件）：只建卡不补结果——卡片入
                // pendingToolCards 等待，实时 tool_result 到达依 FIFO 自然补全
                appendToolCall(toolName, args);
            } else {
                let result = ev.result;
                // result 来自 C++ parseJsonStr（已 object）；防御性兼容历史 string 形态
                if (typeof result === 'string') {
                    try { result = JSON.parse(result || '{}'); } catch (e) { result = {}; }
                }
                if (!result || typeof result !== 'object') { result = {}; }
                appendToolCall(toolName, args);
                appendToolResult(toolName, result);
            }
        } else if (t === 'question') {
            // MAJOR5 + 契约9: ask_user 历史用 appendQuestion 渲染问题气泡（返回气泡
            // 引用），然后 DOM 操作禁用按钮 + 加 answered class + 追加答案文本
            const a = (ev.args && typeof ev.args === 'object') ? ev.args : {};
            const qBubble = appendQuestion(
                a.question || '',
                Array.isArray(a.options) ? a.options : [],
                a.submit_label || 'Submit',
                a.custom_placeholder || '',
                !!a.multi_select
            );
            if (qBubble) {
                // 禁用所有按钮（.question-options 内的选项按钮 + .question-submit 提交按钮）
                qBubble.querySelectorAll('button').forEach(function(b) { b.disabled = true; });
                // 禁用自定义输入框（.question-custom 内的 textarea）
                const ta = qBubble.querySelector('.question-custom textarea');
                if (ta) { ta.disabled = true; }
                // 加 answered class（chat.css:105-107 已有 .answered 禁用态样式）
                qBubble.classList.add('answered');
                // 追加答案文本（ev.result.answer 即用户当时的选择）
                const answer = (ev.result && typeof ev.result === 'object' && 'answer' in ev.result)
                             ? String(ev.result.answer)
                             : (typeof ev.result === 'string' ? ev.result : JSON.stringify(ev.result || ''));
                const ansDiv = document.createElement('div');
                ansDiv.className = 'question-answer';
                ansDiv.textContent = '\u2192 ' + answer;  // → answer
                qBubble.appendChild(ansDiv);
            }
        } else if (t === 'error') {
            // 决策点 4（审计问题 3）：落盘 error 记录重放——复用实时 appendError
            // 渲染错误卡（图标/配色按 error_type，detail 折叠面板）。message 为
            // 落盘原始文案，Dock 侧 onSessionSwitched 已经 mapErrorMessage 预映射
            // 为用户文案（与实时路径一致）
            const m = (ev.message && typeof ev.message === 'object') ? ev.message : {};
            appendError(m.message || '', m.error_type || '', m.detail || '');
        } else if (t === 'usage' || t === 'summary') {
            // 跳过（不渲染；token 由 C++ m_modelLabel/m_tokenLabel 显示，summary 一期不持久化渲染）
        }
    }
}

// —— 分段懒加载：顶部哨兵 + 滚动到顶触发 ——

// 在 #messages 顶部插入"加载更早"哨兵按钮（也可点击触发，滚动到顶自动触发）。
function insertLoadEarlierSentinel() {
    const s = document.createElement('button');
    s.className = 'load-earlier-sentinel';
    s.type = 'button';
    s.textContent = i18n.loadEarlier || 'Load earlier messages';
    s.addEventListener('click', requestLoadEarlier);
    const m = document.getElementById('messages');
    m.insertBefore(s, m.firstChild);
    loadEarlierSentinel = s;
}

function removeLoadEarlierSentinel() {
    if (loadEarlierSentinel && loadEarlierSentinel.parentNode) {
        loadEarlierSentinel.parentNode.removeChild(loadEarlierSentinel);
    }
    loadEarlierSentinel = null;
}

// 触发加载更早一段（scroll 到顶 / 哨兵点击共用）。setTimeout(0) 让点击/滚动
// 事件的交互反馈先渲染，长任务不阻塞在事件处理器内。
function requestLoadEarlier() {
    if (loadingEarlier || pendingEarlierEvents.length === 0) return;
    loadingEarlier = true;
    setTimeout(function() {
        try {
            loadEarlierChunk();
        } finally {
            loadingEarlier = false;
        }
    }, 0);
}

// prepend 更早一段事件到哨兵之前，并保持用户视口不跳动。
// 滚动位置保持：记录渲染前 scrollHeight，渲染后 scrollTop 加上高度增量
//（新内容插在上方，视觉锚点内容下移量 = 高度增量）。
// 渲染目标用 DocumentFragment：insertBefore 时子节点自动展开插入，
// 不引入包裹 div（避免破坏 #messages 的 flex 子项布局）。
function loadEarlierChunk() {
    if (pendingEarlierEvents.length === 0) return;
    const m = document.getElementById('messages');
    const prevHeight = m.scrollHeight;
    const count = Math.min(HISTORY_CHUNK_SIZE, pendingEarlierEvents.length);
    // 取 pending 尾部 count 条（时间上紧邻已渲染内容）
    const chunk = pendingEarlierEvents.splice(pendingEarlierEvents.length - count, count);
    // 渲染到 detached fragment 再整体插入：避免逐节点 insertBefore 引发多次 reflow
    const holder = document.createDocumentFragment();
    renderTargetOverride = holder;
    suppressAutoScroll = true;
    try {
        renderHistoryEvents(chunk);
        // chunk 末尾收尾工具组（对齐 loadHistory 收尾语义）
        closeToolGroup();
    } finally {
        suppressAutoScroll = false;
        renderTargetOverride = null;
    }
    // 插入位置：哨兵**之后**（哨兵与已渲染内容之间）。哨兵永远位于已渲染
    // 内容的最顶部（它标记"上方还有更早内容"），每个更早的段插在哨兵下方，
    // 时间序保持 [哨兵, 更早段, ..., 最新段]。若插到哨兵上方（insertBefore
    // holder, sentinel），哨兵会被推到段后面，下一次 prepend 的锚点错位，
    // 早段会插到晚段之后（顺序颠倒）。
    m.insertBefore(holder, loadEarlierSentinel ? loadEarlierSentinel.nextSibling : m.firstChild);
    // 保持视口锚定：内容整体下移了 (newHeight - prevHeight)
    m.scrollTop += (m.scrollHeight - prevHeight);
    if (pendingEarlierEvents.length === 0) {
        removeLoadEarlierSentinel();
    }
}

function escapeHtml(text) {
    let div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// —— 重试状态条 + 错误消息渲染（plan-06）——
// 被 C++ DAAgentWebChannel::callJS 调用：
//   showRetryStatus(attempt, maxAttempts, delayMs, errorType, errorMessage)
//   appendError(message, errorType)
// 重试成功（token 到达）或失败（error 到达）时由 appendToken/finalizeAgentMessage
// /appendError/clearChat 中的 hideRetryStatus 清除状态条。

let retryStatusBar = null;          // 当前重试状态条 DOM 元素
let retryCountdownInterval = null;  // 倒计时 setInterval 句柄

// 显示重试状态条 + 启动倒计时。多次重试连续到达时先清除旧状态条再创建新的。
function showRetryStatus(attempt, maxAttempts, delayMs, errorType, errorMessage) {
    // 如果已有状态条，先移除（更新为新的重试信息）
    hideRetryStatus();

    // 创建状态条 DOM
    retryStatusBar = document.createElement('div');
    retryStatusBar.className = 'retry-status-bar';

    const icon = document.createElement('span');
    icon.className = 'retry-icon';
    icon.textContent = '🔄';

    const text = document.createElement('span');
    text.className = 'retry-text';

    const remainingSpan = document.createElement('span');
    remainingSpan.className = 'retry-countdown';
    remainingSpan.textContent = formatCountdown(delayMs);

    text.innerHTML = '';
    text.appendChild(document.createTextNode('Retry ' + attempt + '/' + maxAttempts + ' · '));
    text.appendChild(remainingSpan);
    text.appendChild(document.createTextNode(' · ' + errorMessage));

    retryStatusBar.appendChild(icon);
    retryStatusBar.appendChild(text);

    // 插入到聊天容器末尾（当前 agent 消息之后）
    const container = document.getElementById('messages');
    container.appendChild(retryStatusBar);

    // 启动倒计时
    startRetryCountdown(delayMs);

    // 滚动到底部
    container.scrollTop = container.scrollHeight;
}

// 隐藏重试状态条：清理 interval 和 DOM 元素。
function hideRetryStatus() {
    if (retryCountdownInterval) {
        clearInterval(retryCountdownInterval);
        retryCountdownInterval = null;
    }
    if (retryStatusBar && retryStatusBar.parentNode) {
        retryStatusBar.parentNode.removeChild(retryStatusBar);
    }
    retryStatusBar = null;
}

// 启动倒计时：每秒更新 .retry-countdown 文本，到 0 后显示 "Retrying..."。
function startRetryCountdown(totalMs) {
    let remaining = Math.ceil(totalMs / 1000);
    const updateFn = function() {
        if (!retryStatusBar) {
            clearInterval(retryCountdownInterval);
            retryCountdownInterval = null;
            return;
        }
        const countdownEl = retryStatusBar.querySelector('.retry-countdown');
        if (countdownEl) {
            if (remaining > 0) {
                countdownEl.textContent = formatCountdown(remaining * 1000);
                remaining--;
            } else {
                countdownEl.textContent = 'Retrying...';
                clearInterval(retryCountdownInterval);
                retryCountdownInterval = null;
            }
        }
    };
    updateFn(); // 立即执行一次
    retryCountdownInterval = setInterval(updateFn, 1000);
}

// 格式化倒计时：>=60s 显示 "Xm Ys"，否则 "Ns"。
function formatCountdown(ms) {
    const seconds = Math.ceil(ms / 1000);
    if (seconds >= 60) {
        const mins = Math.floor(seconds / 60);
        const secs = seconds % 60;
        return mins + 'm ' + secs + 's';
    }
    return seconds + 's';
}

// 渲染错误消息：按 errorType 选择图标和配色。
// errorType 取值见 D9 协议枚举：quota_exhausted / auth_error / rate_limit_exhausted
// / network_exhausted / server_error_exhausted / bad_request / context_overflow
// / crash_recovery / timeout / unknown
function appendError(message, errorType, detail) {
    // 隐藏重试状态条（如果还在）
    hideRetryStatus();

    const container = document.getElementById('messages');
    const errorDiv = document.createElement('div');
    errorDiv.className = 'error-message';

    // 根据 errorType 选择图标和样式
    let icon = '⚠️';
    let errorClass = 'error-general';
    if (errorType === 'quota_exhausted') {
        icon = '💳'; errorClass = 'error-quota';
    } else if (errorType === 'auth_error') {
        icon = '🔑'; errorClass = 'error-auth';
    } else if (errorType === 'rate_limit_exhausted' || errorType === 'network_exhausted' || errorType === 'server_error_exhausted') {
        icon = '🔄'; errorClass = 'error-exhausted';
    } else if (errorType === 'crash_recovery') {
        icon = '🔧'; errorClass = 'error-crash';
    } else if (errorType === 'timeout') {
        icon = '⏱️'; errorClass = 'error-timeout';
    }

    errorDiv.classList.add(errorClass);

    // 主行：图标 + 友好消息文案（C++ mapErrorMessage 映射后的用户文案）
    const main = document.createElement('div');
    main.className = 'error-main';
    const iconEl = document.createElement('span');
    iconEl.className = 'error-icon';
    iconEl.textContent = icon;
    const textEl = document.createElement('span');
    textEl.className = 'error-text';
    textEl.textContent = message;
    main.appendChild(iconEl);
    main.appendChild(textEl);
    errorDiv.appendChild(main);

    // 可折叠详情面板（仅 detail 非空时）：默认折叠，避免长 traceback 占满对话界面。
    // <pre> 截断显示（前 1500 字符 + 截断标记），复制按钮复制完整原始 detail。
    // 标签（Details/Copy/Copied/[truncated]）由 C++ 经 setI18nLabels 注入 i18n。
    const fullDetail = (detail != null) ? String(detail) : '';
    if (fullDetail.trim()) {
        const MAX_DETAIL_CHARS = 1500;
        let displayDetail = fullDetail;
        let truncated = false;
        if (fullDetail.length > MAX_DETAIL_CHARS) {
            displayDetail = fullDetail.slice(0, MAX_DETAIL_CHARS)
                + '\n… ' + (fullDetail.length - MAX_DETAIL_CHARS) + ' chars '
                + (i18n.errorTruncated || '[truncated]');
            truncated = true;
        }
        const wrap = document.createElement('div');
        wrap.className = 'error-detail-wrap';

        const toggle = document.createElement('button');
        toggle.type = 'button';
        toggle.className = 'error-detail-toggle';
        toggle.innerHTML = CHEVRON_SVG
            + '<span class="error-detail-label">' + escapeHtml(i18n.errorDetails || 'Details') + '</span>'
            + (truncated ? '<span class="error-detail-meta">' + escapeHtml(i18n.errorTruncated || '[truncated]') + '</span>' : '');

        const body = document.createElement('div');
        body.className = 'error-detail-body';
        body.setAttribute('hidden', '');

        const pre = document.createElement('pre');
        pre.className = 'error-detail-pre';
        pre.textContent = displayDetail;  // textContent 防 HTML 注入

        const copyBtn = document.createElement('button');
        copyBtn.type = 'button';
        copyBtn.className = 'error-detail-copy';
        copyBtn.textContent = i18n.errorCopy || 'Copy';
        copyBtn.addEventListener('click', function() {
            copyTextToClipboard(fullDetail);
            const orig = copyBtn.textContent;
            copyBtn.textContent = i18n.errorCopied || 'Copied';
            copyBtn.classList.add('copied');
            setTimeout(function() {
                copyBtn.textContent = orig;
                copyBtn.classList.remove('copied');
            }, 1500);
        });

        body.appendChild(pre);
        body.appendChild(copyBtn);

        toggle.addEventListener('click', function() {
            if (body.hasAttribute('hidden')) {
                body.removeAttribute('hidden');
                toggle.classList.add('open');
            } else {
                body.setAttribute('hidden', '');
                toggle.classList.remove('open');
            }
        });

        wrap.appendChild(toggle);
        wrap.appendChild(body);
        errorDiv.appendChild(wrap);
    }

    container.appendChild(errorDiv);
    container.scrollTop = container.scrollHeight;
}

// 剪贴板复制：优先 navigator.clipboard（QWebEngine Chromium 支持），失败回退 execCommand。
// detail 可能含大量文本/特殊字符，用临时 textarea + execCommand 兜底确保复制成功。
function copyTextToClipboard(text) {
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(text).catch(function() {
            fallbackCopyTextToClipboard(text);
        });
    } else {
        fallbackCopyTextToClipboard(text);
    }
}
function fallbackCopyTextToClipboard(text) {
    var ta = document.createElement('textarea');
    ta.value = text;
    ta.style.position = 'fixed';
    ta.style.top = '0';
    ta.style.left = '0';
    ta.style.opacity = '0';
    document.body.appendChild(ta);
    ta.focus();
    ta.select();
    try { document.execCommand('copy'); } catch (e) {}
    document.body.removeChild(ta);
}

// 渲染系统消息：用户可见但不作为 LLM 对话内容的通知横幅（类似 MessageBox）。
// level: "info" | "warning" | "error"，控制图标和配色。
function appendSystemMessage(text, level) {
    var container = document.getElementById('messages');
    var div = document.createElement('div');
    div.className = 'system-message';

    var icon = 'ℹ️';
    var levelClass = 'system-info';
    if (level === 'warning') {
        icon = '⚠️';
        levelClass = 'system-warning';
    } else if (level === 'error') {
        // \u274C = ❌，用转义避免 lupdate 按 GBK 解析 UTF-8 字节导致语法错误
        icon = '\u274C';
        levelClass = 'system-error';
    }
    div.classList.add(levelClass);

    var iconEl = document.createElement('span');
    iconEl.className = 'system-message-icon';
    iconEl.textContent = icon;

    var textEl = document.createElement('span');
    textEl.className = 'system-message-text';
    textEl.textContent = text;

    div.appendChild(iconEl);
    div.appendChild(textEl);
    container.appendChild(div);

    container.scrollTop = container.scrollHeight;
}

// 初始化
init();
