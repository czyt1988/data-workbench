// DAMarkdownView rendering script
// Uses markdown-it (loaded from DAAgent qrc) + highlight.js (loaded from DAAgent qrc)
// + KaTeX (loaded from DAAgent qrc, same as chat.js)
var md = null;

function initMarkdown() {
    md = window.markdownit({
        html: false,
        breaks: true,
        linkify: true,
        highlight: function (str, lang) {
            if (lang && hljs.getLanguage(lang)) {
                try {
                    return '<pre><code class="hljs">' +
                        hljs.highlight(str, { language: lang }).value +
                        '</code></pre>';
                } catch (__) {
                }
            }
            return '<pre><code class="hljs">' + md.utils.escapeHtml(str) + '</code></pre>';
        }
    });
    setupMathRules(md);
}

// —— KaTeX 数学公式渲染（与 Agent/resources/chat.js 保持一致）——
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
        // output：'html'（查看器/HTML/PDF 导出）或 'mathml'（Word 导出，原生公式）
        var opts = {throwOnError: false, displayMode: !!displayMode};
        if (gKatexOutput === 'mathml') {
            opts.output = 'mathml';
        }
        return katex.renderToString(tex, opts);
    } catch (e) {
        return '<code>' + (displayMode ? '$$' + tex + '$$' : '$' + tex + '$') + '</code>';
    }
}

// —— 导出与图片内嵌支持（DAMarkdownView / DAMarkdownExporter 调用）——
// KaTeX 输出模式：'html'（默认，与查看器一致）/ 'mathml'（Word 导出，原生公式）
var gKatexOutput = 'html';

// 分片渲染缓冲：大体积内容（含内嵌 base64 图片）单次 runJavaScript 会超
// Chromium IPC 上限静默失败，C++ 侧分片累积后整体渲染（同页 FIFO 保序，
// 同 DAAgentWebChannel 的 loadHistoryPart 分片先例）
var gRenderBuffer = '';

function renderMarkdownBegin() {
    gRenderBuffer = '';
}

function renderMarkdownAppend(text) {
    gRenderBuffer += text;
}

function renderMarkdownEnd() {
    renderMarkdown(gRenderBuffer);
    gRenderBuffer = '';
}

// 导出渲染：结果暂存 gExportHtml（不写 DOM），返回总长度供 C++ 分片取回
var gExportHtml = '';

function exportRenderBegin(katexOutput) {
    gRenderBuffer = '';
    gKatexOutput = (katexOutput === 'mathml') ? 'mathml' : 'html';
}

function exportRenderEnd() {
    if (!md) {
        initMarkdown();
    }
    gExportHtml = md.render(gRenderBuffer);
    gRenderBuffer = '';
    gKatexOutput = 'html';  // 恢复默认，避免影响查看器后续渲染
    return gExportHtml.length;
}

function getExportHtmlChunk(offset, size) {
    // 分片边界避开 UTF-16 代理对（切口落在代理对中间会产生孤立代理项，
    // 经 IPC 序列化时字符损坏），C++ 侧按实际返回长度推进 offset
    var end = offset + size;
    if (end < gExportHtml.length) {
        var c = gExportHtml.charCodeAt(end - 1);
        if (c >= 0xD800 && c <= 0xDBFF) {
            end -= 1;
        }
    }
    return gExportHtml.substr(offset, end - offset);
}

function clearExportHtml() {
    gExportHtml = '';
}

function renderMarkdown(text) {
    if (!md) {
        initMarkdown();
    }
    var el = document.getElementById('content');
    if (el) {
        el.innerHTML = md.render(text);
        window.scrollTo(0, 0);
    }
}

function clearContent() {
    var el = document.getElementById('content');
    if (el) {
        el.innerHTML = '';
    }
}

// Initialize markdown-it on page load
initMarkdown();
