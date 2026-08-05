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
}

function createMessageBubble(className) {
    let div = document.createElement('div');
    div.className = 'message-bubble ' + className;
    div.dataset.rawText = '';
    return div;
}

function scrollToBottom() {
    window.scrollTo(0, document.body.scrollHeight);
}

function init() {
    initMarkdown();
    new QWebChannel(qt.webChannelTransport, function(channel) {
        chatBridge = channel.objects.chatBridge;
    });
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
    document.getElementById('messages').appendChild(group);
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

// —— 被 C++ 调用的函数 ——
// 通过 DAAgentWebChannel::callJS(evaluateJavaScript) 调用

function appendUserMessage(text) {
    flushAgentMessage();
    closeToolGroup();
    let bubble = createMessageBubble('user');
    bubble.textContent = text;
    document.getElementById('messages').appendChild(bubble);
    scrollToBottom();
}

function appendToken(text) {
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

function appendQuestion(text, options) {
    flushAgentMessage();
    closeToolGroup();
    let qBubble = createMessageBubble('question');
    qBubble.innerHTML = '<p>' + md.renderInline(text) + '</p>';
    let btnContainer = document.createElement('div');
    btnContainer.className = 'question-options';
    options.forEach(function(opt) {
        let btn = document.createElement('button');
        btn.textContent = opt;
        btn.onclick = function() {
            chatBridge.onUserSelect(opt);
            qBubble.classList.add('answered');
        };
        btnContainer.appendChild(btn);
    });
    qBubble.appendChild(btnContainer);
    document.getElementById('messages').appendChild(qBubble);
    scrollToBottom();
}

function clearChat() {
    if (renderTimer) {
        clearTimeout(renderTimer);
        renderTimer = null;
    }
    document.getElementById('messages').innerHTML = '';
    currentAgentMsg = null;
    currentToolGroup = null;
    pendingToolCards = [];
}

function escapeHtml(text) {
    let div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// 初始化
init();
