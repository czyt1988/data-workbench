let chatBridge = null;
let md = null;
let currentAgentMsg = null;  // 当前正在流式输出的消息 DOM 节点
let renderTimer = null;      // appendToken 的防抖计时器
const RENDER_DEBOUNCE_MS = 50;  // 最多每 50ms 重新渲染一次

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

// —— 被 C++ 调用的函数 ——
// 通过 evaluateJavaScript 调用

function appendUserMessage(text) {
    let bubble = createMessageBubble('user');
    bubble.textContent = text;
    document.getElementById('messages').appendChild(bubble);
    scrollToBottom();
}

function appendToken(text) {
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
    if (currentAgentMsg) {
        currentAgentMsg.dataset.rawText = fullText;
        currentAgentMsg.innerHTML = md.render(fullText);
        currentAgentMsg = null;
    }
    scrollToBottom();
}

function appendToolCall(toolName, args) {
    let bubble = createMessageBubble('tool');
    bubble.innerHTML = '<span class="tool-name">🔧 ' + escapeHtml(toolName) +
                       '</span><pre>' + escapeHtml(JSON.stringify(args, null, 2)) + '</pre>';
    document.getElementById('messages').appendChild(bubble);
    scrollToBottom();
}

function appendToolResult(toolName, result) {
    let bubble = createMessageBubble('tool');
    bubble.innerHTML = '<span class="tool-name">✅ ' + escapeHtml(toolName) + '</span>' +
                       '<pre>' + escapeHtml(JSON.stringify(result, null, 2)) + '</pre>';
    document.getElementById('messages').appendChild(bubble);
    scrollToBottom();
}

function appendQuestion(text, options) {
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
}

function escapeHtml(text) {
    let div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// 初始化
init();
