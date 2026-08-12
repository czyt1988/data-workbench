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

// —— 输入区/状态栏 web 化状态 ——
// i18n 静态标签由 C++ 在握手(onReady)时经 setI18nLabels 注入（C++ 仍是唯一 i18n 拥有者，
// 沿用 appendQuestion 推 tr("Submit") 的既有约定；JS 为哑显示）。
let i18n = {
    send: 'Send', stop: 'Stop',
    ready: 'Ready', thinking: 'Agent thinking...', stopping: 'Stopping...',
    inputPlaceholder: '', tokenEmpty: 'tokens: -',
    popoverInput: 'input: %1', popoverOutput: 'output: %1',
    popoverTotal: 'total: %1', popoverWindow: 'window: %1',
    popoverSource: 'source: %1', popoverSourceUnknown: 'unknown'
};
let agentBusy = false;          // 当前是否思考中（驱动 send-btn 的 Send/Stop 切换）
let tokenStatsCache = null;     // 缓存最近一次 setTokenStats 的 5 值，供 popover 渲染

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
    // #messages 是唯一滚动容器（body 已 overflow:hidden），滚它而非 window
    var m = document.getElementById('messages');
    if (m) { m.scrollTop = m.scrollHeight; }
}

function init() {
    initMarkdown();
    new QWebChannel(qt.webChannelTransport, function(channel) {
        chatBridge = channel.objects.chatBridge;
        // 握手：通知 C++ web 侧已就绪，C++ 回推 setI18nLabels/setBusy/setModel/setTokenStats。
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

    document.getElementById('messages').appendChild(qBubble);
    scrollToBottom();
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
}

// busy 打包：true→按钮 Stop(红)+输入禁用+状态 thinking；false→按钮 Send+输入启用+状态 ready。
// JS 内聚解释"忙态长什么样"，C++ 只发一个 bool。
function setBusy(busy) {
    agentBusy = busy;
    applySendButtonState();
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

// 设置状态文案（左）。busy/stopping 内部调，也供 C++ 直接推过渡态。
function setStatus(text) {
    var el = document.getElementById('status-text');
    if (el) { el.textContent = text || ''; }
}

// 设置模型名（中）。label 已由 C++ 格式化为 "Model: <name>"，CSS ellipsis 截断。
function setModel(label) {
    var el = document.getElementById('model-label');
    if (!el) return;
    el.textContent = label || '';
    // tooltip 显示完整名（与旧 QFontMetrics tooltip 同效果）
    el.title = label || '';
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

// plan-04 step6: 批量重放历史会话记录到聊天界面。
// events 为 C++ DAAgentWebChannel::loadHistory 合并后的 UI 事件数组：
//   {type:"user",message:{role,content}},
//   {type:"assistant",message:{role,content}},
//   {type:"tool",toolName,args,result,toolCallId},
//   {type:"question",toolName,args,result:{answer},toolCallId},
//   {type:"usage",...}/type:"summary" 跳过不渲染。
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
    for (const ev of events) {
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
            const bubble = createMessageBubble('agent');
            bubble.dataset.rawText = content;
            bubble.innerHTML = md.render(content);
            document.getElementById('messages').appendChild(bubble);  // ← 必须挂到 DOM
        } else if (t === 'tool') {
            // MAJOR2: 读合并后字段 toolName/args/result（C++ 已配对）
            const toolName = ev.toolName || 'tool';
            const args = (ev.args && typeof ev.args === 'object') ? ev.args : {};
            let result = ev.result;
            // result 来自 C++ parseJsonStr（已 object）；防御性兼容历史 string 形态
            if (typeof result === 'string') {
                try { result = JSON.parse(result || '{}'); } catch (e) { result = {}; }
            }
            if (!result || typeof result !== 'object') { result = {}; }
            appendToolCall(toolName, args);
            appendToolResult(toolName, result);
        } else if (t === 'question') {
            // MAJOR5 + 契约9: ask_user 历史用 appendQuestion 渲染问题气泡，然后 DOM 操作
            const a = (ev.args && typeof ev.args === 'object') ? ev.args : {};
            appendQuestion(
                a.question || '',
                Array.isArray(a.options) ? a.options : [],
                a.submit_label || 'Submit',
                a.custom_placeholder || '',
                !!a.multi_select
            );
            // appendQuestion 把 qBubble 挂到 #messages 末尾（chat.js:334），取最后一个 .message-bubble.question
            const qBubble = document.querySelector('#messages .message-bubble.question:last-of-type');
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
        } else if (t === 'usage' || t === 'summary') {
            // 跳过（不渲染；token 由 C++ m_modelLabel/m_tokenLabel 显示，summary 一期不持久化渲染）
        }
    }
    scrollToBottom();
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
function appendError(message, errorType) {
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

    const iconEl = document.createElement('span');
    iconEl.className = 'error-icon';
    iconEl.textContent = icon;

    const textEl = document.createElement('span');
    textEl.className = 'error-text';
    textEl.textContent = message;

    errorDiv.appendChild(iconEl);
    errorDiv.appendChild(textEl);
    container.appendChild(errorDiv);

    container.scrollTop = container.scrollHeight;
}

// 初始化
init();
