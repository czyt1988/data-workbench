// DAMarkdownView rendering script
// Uses markdown-it (loaded from DAAgent qrc) + highlight.js (loaded from DAAgent qrc)
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
