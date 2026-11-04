#include "Common/CodeEditor/DAPythonCodeEdit.h"
#include "DAPythonHighlighter.h"
#include "DAPyPythonCompletion.h"
#include "DAPythonSyntax.h"
// Qt
#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QModelIndex>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QStringListModel>
#include <QTextCursor>
#include <utility>

namespace DA
{
namespace
{
// 补全弹窗宽度上限（像素）
constexpr int c_maxPopupWidth = 460;
constexpr int c_minPopupWidth = 180;
}  // namespace

DAPythonCodeEdit::DAPythonCodeEdit(QWidget* parent) : QPlainTextEdit(parent)
{
    // 代码编辑器统一等宽字体；字号沿用界面默认，避免与其它控件不一致
    QFont codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    if (font().pointSizeF() > 0) {
        codeFont.setPointSizeF(font().pointSizeF());
    }
    setFont(codeFont);
    document()->setDefaultFont(codeFont);

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(QFontMetricsF(codeFont).horizontalAdvance(QLatin1Char(' ')) * 4.0);
    setPlaceholderText(tr("e.g. tmp = A * 2, then np.where(tmp > 0, tmp, 0)"));  // cn:示例：tmp = A * 2，然后 np.where(tmp > 0, tmp, 0)

    m_highlighter = new DAPythonHighlighter(document());

    m_model     = new QStringListModel(this);
    m_completer = new QCompleter(m_model, this);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setModelSorting(QCompleter::UnsortedModel);
    m_completer->setWrapAround(false);
    m_completer->popup()->setObjectName(QStringLiteral("pythonCompletionPopup"));
    connect(m_completer, QOverload< const QModelIndex& >::of(&QCompleter::activated), this,
            [ this ](const QModelIndex& idx) { insertCompletion(idx.data(Qt::DisplayRole).toString()); });

    connect(this, &QPlainTextEdit::textChanged, this, &DAPythonCodeEdit::onTextChanged);
}

DAPythonCodeEdit::~DAPythonCodeEdit()
{
}

/**
 * @brief 注入上下文词（如列名）
 * @param entries 显示文本 + 插入文本
 */
void DAPythonCodeEdit::setExtraEntries(const QList< DAPythonCompletionEntry >& entries)
{
    m_extraEntries = entries;
    m_insertTexts.clear();
    for (const DAPythonCompletionEntry& e : entries) {
        const QString insert = e.insertText.isEmpty() ? e.text : e.insertText;
        if (!m_insertTexts.contains(e.text)) {
            m_insertTexts.insert(e.text, insert);
        }
    }
}

void DAPythonCodeEdit::clearExtraEntries()
{
    m_extraEntries.clear();
    m_insertTexts.clear();
}

/**
 * @brief 在光标处插入文本并聚焦
 * @param text 待插入文本
 */
void DAPythonCodeEdit::insertCodeAtCursor(const QString& text)
{
    QTextCursor cur = textCursor();
    cur.insertText(text);
    setTextCursor(cur);
    setFocus();
}

void DAPythonCodeEdit::setCompletionEnabled(bool on)
{
    m_completionEnabled = on;
    if (!on && m_completer && m_completer->popup()) {
        m_completer->popup()->hide();
    }
}

bool DAPythonCodeEdit::isCompletionEnabled() const
{
    return m_completionEnabled;
}

/**
 * @brief 补全弹窗（测试/调试用）
 * @return 弹窗视图；QCompleter 未创建时返回 nullptr
 */
QAbstractItemView* DAPythonCodeEdit::completionPopup() const
{
    return m_completer ? m_completer->popup() : nullptr;
}

void DAPythonCodeEdit::keyPressEvent(QKeyEvent* e)
{
    // 补全弹窗打开时先处理接受/关闭/导航键
    if (m_completer && m_completer->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab: {
            QModelIndex idx = m_completer->popup()->currentIndex();
            if (!idx.isValid()) {
                idx = m_completer->popup()->model()->index(0, 0);
            }
            if (idx.isValid()) {
                insertCompletion(idx.data(Qt::DisplayRole).toString());
                e->accept();
                return;
            }
            break;
        }
        case Qt::Key_Escape:
            m_completer->popup()->hide();
            e->accept();
            return;
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            QApplication::sendEvent(m_completer->popup(), e);
            return;
        default:
            break;
        }
    }
    // Ctrl+Space 手动补全
    if (e->key() == Qt::Key_Space && (e->modifiers() & Qt::ControlModifier)) {
        updateCompletion(false);
        e->accept();
        return;
    }
    // Ctrl+Enter 提交（对话框接为“确定”）
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && (e->modifiers() & Qt::ControlModifier)) {
        Q_EMIT submitRequested();
        e->accept();
        return;
    }
    QPlainTextEdit::keyPressEvent(e);
}

void DAPythonCodeEdit::focusOutEvent(QFocusEvent* e)
{
    if (m_completer && m_completer->popup()) {
        m_completer->popup()->hide();
    }
    QPlainTextEdit::focusOutEvent(e);
}

void DAPythonCodeEdit::showCompletion()
{
    updateCompletion(false);
}

void DAPythonCodeEdit::onTextChanged()
{
    if (m_updating || !m_completionEnabled) {
        return;
    }
    if (!hasFocus()) {
        return;
    }
    updateCompletion(true);
}

/**
 * @brief 构建并弹出补全列表
 * @param autoTrigger true=输入触发（空前缀且非成员上下文时不弹）；false=手动触发
 */
void DAPythonCodeEdit::updateCompletion(bool autoTrigger)
{
    if (!m_completer || !m_completionEnabled) {
        return;
    }
    QString prefix;
    const bool inBacktick = cursorInsideBacktick(&prefix);
    int startAbs          = 0;
    QString baseKey;
    if (inBacktick) {
        startAbs = textCursor().position() - prefix.size();
    } else {
        prefix = currentPrefix(&startAbs);
        baseKey = memberBaseKey();
    }
    if (autoTrigger && prefix.isEmpty() && baseKey.isEmpty()) {
        m_completer->popup()->hide();
        return;
    }
    QStringList candidates = buildCandidates(inBacktick, baseKey);
    if (candidates.isEmpty()) {
        m_completer->popup()->hide();
        return;
    }
    m_inBacktick      = inBacktick;
    m_completionStart = startAbs;
    m_model->setStringList(candidates);
    m_completer->setCompletionPrefix(prefix);

    // 弹窗宽度按最长候选估算，避免窄条
    const QFontMetrics fm(font());
    int width = c_minPopupWidth;
    for (const QString& s : candidates) {
        width = qMax(width, fm.horizontalAdvance(s) + 32);
        if (width >= c_maxPopupWidth) {
            width = c_maxPopupWidth;
            break;
        }
    }
    QRect rect = cursorRect();
    rect.setWidth(width);
    m_completer->popup()->setMinimumWidth(width);
    m_completer->complete(rect);
}

/**
 * @brief 当前光标前的标识符前缀
 * @param startAbs 输出前缀起始文档位置
 * @return 前缀文本
 */
QString DAPythonCodeEdit::currentPrefix(int* startAbs) const
{
    const QString text = toPlainText();
    const int pos      = textCursor().position();
    int start          = pos;
    while (start > 0 && isWordChar(text.at(start - 1))) {
        --start;
    }
    if (startAbs) {
        *startAbs = start;
    }
    return text.mid(start, pos - start);
}

/**
 * @brief 光标前是否为 “.” 成员访问，返回可识别的对象 key
 *
 * 支持 df. / pd. / np. / pd.DataFrame. / pd.Series. / 列名. / 列名.str. / 列名.dt.
 * @return 对象 key（空 = 非成员上下文）
 */
QString DAPythonCodeEdit::memberBaseKey() const
{
    const QString text = toPlainText();
    const int pos      = textCursor().position();
    int start          = pos;
    while (start > 0 && isWordChar(text.at(start - 1))) {
        --start;
    }
    if (start <= 0 || text.at(start - 1) != QLatin1Char('.')) {
        return {};
    }
    // 点号前是对象链（标识符/点），如 df、A.str、pd.DataFrame
    int chainBegin = start - 1;
    while (chainBegin > 0) {
        const QChar c = text.at(chainBegin - 1);
        if (c.isLetterOrNumber() || c == QLatin1Char('_') || c == QLatin1Char('.')) {
            --chainBegin;
        } else {
            break;
        }
    }
    QString chain = text.mid(chainBegin, start - 1 - chainBegin);
    while (chain.endsWith(QLatin1Char('.'))) {
        chain.chop(1);
    }
    if (chain.isEmpty()) {
        return {};
    }
    if (chain == QLatin1String("pd")) {
        return QStringLiteral("pd");
    }
    if (chain == QLatin1String("np")) {
        return QStringLiteral("np");
    }
    if (chain == QLatin1String("df") || chain == QLatin1String("pd.DataFrame")) {
        return QStringLiteral("DataFrame");
    }
    if (chain == QLatin1String("pd.Series")) {
        return QStringLiteral("Series");
    }
    if (chain.endsWith(QLatin1String(".str"))) {
        const QString base = chain.left(chain.size() - 4);
        if (base == QLatin1String("Series") || base == QLatin1String("pd.Series") || isColumnWord(base)) {
            return QStringLiteral("Series.str");
        }
    }
    if (chain.endsWith(QLatin1String(".dt"))) {
        const QString base = chain.left(chain.size() - 3);
        if (base == QLatin1String("Series") || base == QLatin1String("pd.Series") || isColumnWord(base)) {
            return QStringLiteral("Series.dt");
        }
    }
    if (isColumnWord(chain)) {
        return QStringLiteral("Series");
    }
    return {};
}

/**
 * @brief 光标是否位于未闭合的反引号列名内
 * @param prefix 输出反引号后的前缀文本
 * @return 位于反引号内返回 true
 */
bool DAPythonCodeEdit::cursorInsideBacktick(QString* prefix) const
{
    const QTextCursor cur = textCursor();
    const QString block   = cur.block().text();
    const int col         = cur.positionInBlock();
    int count             = 0;
    int last              = -1;
    for (int i = 0; i < col && i < block.size(); ++i) {
        if (block.at(i) == QLatin1Char('`')) {
            ++count;
            last = i;
        }
    }
    if (count % 2 == 0 || last < 0) {
        return false;
    }
    if (prefix) {
        *prefix = block.mid(last + 1, col - last - 1);
    }
    return true;
}

/**
 * @brief 组装候选词
 * @param inBacktick 是否在反引号列名内
 * @param baseKey 成员访问对象 key（空 = 非成员上下文）
 * @return 候选项（已去重，保持优先级顺序）
 */
QStringList DAPythonCodeEdit::buildCandidates(bool inBacktick, const QString& baseKey) const
{
    QStringList res;
    if (inBacktick) {
        // 反引号内只补列名，插入时不重复加反引号
        for (const DAPythonCompletionEntry& e : m_extraEntries) {
            if (!res.contains(e.text)) {
                res.append(e.text);
            }
        }
        return res;
    }
    if (!baseKey.isEmpty()) {
        return DAPyPythonCompletion::membersOf(baseKey);
    }
    QSet< QString > seen;
    // 1) 上下文词（列名）优先
    for (const DAPythonCompletionEntry& e : m_extraEntries) {
        if (!seen.contains(e.text)) {
            res.append(e.text);
            seen.insert(e.text);
        }
    }
    // 2) 文档内已有标识符
    static const QRegularExpression wordRx(QStringLiteral("[\\p{L}_][\\p{L}\\p{N}_]*"),
                                           QRegularExpression::UseUnicodePropertiesOption);
    const QString code = toPlainText();
    QRegularExpressionMatchIterator it = wordRx.globalMatch(code);
    while (it.hasNext()) {
        const QString word = it.next().captured();
        if (!seen.contains(word)) {
            res.append(word);
            seen.insert(word);
        }
    }
    // 3) Python 关键字 / 内置名
    const QStringList kws = DAPyPythonCompletion::keywords();
    for (const QString& k : kws) {
        if (!seen.contains(k)) {
            res.append(k);
            seen.insert(k);
        }
    }
    const QStringList builtins = DAPyPythonCompletion::builtins();
    for (const QString& b : std::as_const(builtins)) {
        if (!seen.contains(b)) {
            res.append(b);
            seen.insert(b);
        }
    }
    return res;
}

/**
 * @brief 用候选文本替换当前补全前缀
 * @param displayText 候选显示文本
 */
void DAPythonCodeEdit::insertCompletion(const QString& displayText)
{
    if (m_completionStart < 0) {
        return;
    }
    QTextCursor cur = textCursor();
    const int end   = cur.position();
    if (m_completionStart > end) {
        m_completionStart = -1;
        return;
    }
    cur.setPosition(m_completionStart);
    cur.setPosition(end, QTextCursor::KeepAnchor);
    QString insert;
    if (m_inBacktick) {
        insert = displayText;
        // 后面没有现成的右反引号时自动补一个，避免列名后还要手输
        const QString code = toPlainText();
        if (end >= code.size() || code.at(end) != QLatin1Char('`')) {
            insert += QLatin1Char('`');
        }
    } else {
        insert = m_insertTexts.value(displayText, displayText);
    }
    m_updating           = true;
    cur.insertText(insert);
    setTextCursor(cur);
    m_updating        = false;
    m_completionStart = -1;
    if (m_completer && m_completer->popup()) {
        m_completer->popup()->hide();
    }
}

bool DAPythonCodeEdit::isWordChar(const QChar& c) const
{
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

/**
 * @brief 文本是否是注入的列名
 */
bool DAPythonCodeEdit::isColumnWord(const QString& word) const
{
    for (const DAPythonCompletionEntry& e : m_extraEntries) {
        if (e.text == word) {
            return true;
        }
    }
    return false;
}
}  // namespace DA
