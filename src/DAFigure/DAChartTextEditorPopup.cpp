#include "DAChartTextEditorPopup.h"
#include "DAFontEditPannelWidget.h"
#include "DAColorPickerButton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QTextEdit>
#include <QTextCursor>
#include <QKeyEvent>
#include <QHideEvent>
#include <QEvent>
#include <QSignalBlocker>
#include <QRegularExpression>
#include <QGuiApplication>
#include <QScreen>

namespace DA
{
/**
 * @brief 从QTextEdit::toHtml()的完整文档中提取body内层HTML片段
 *
 * QTextEdit::toHtml()输出完整HTML文档，此处仅保留body内部内容，
 * 丢弃其中body标签上的默认样式（字体等默认属性由QwtText::setFont接管），
 * 得到干净的富文本片段便于序列化
 * @param fullHtml 完整HTML文档
 * @return body内层HTML片段，提取失败时返回原文
 */
static QString extractBodyHtml(const QString& fullHtml)
{
    static QRegularExpression re(
        R"(<body[^>]*>(.*)</body>)",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption
    );
    QRegularExpressionMatch m = re.match(fullHtml);
    if (m.hasMatch()) {
        return m.captured(1).trimmed();
    }
    return fullHtml;
}

class DAChartTextEditorPopup::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartTextEditorPopup)
public:
    PrivateData(DAChartTextEditorPopup* p);
    void buildFontPanel(QHBoxLayout* lay);
    void buildButtonRow(QVBoxLayout* lay);

public:
    DAFontEditPannelWidget* mFontPanel { nullptr };  ///< 字体编辑面板(族/字号/粗斜下划线/文字颜色)
    DAColorPickerButton* mBgColorButton { nullptr };  ///< 文字背景色
    QToolButton* mBtnStrikeout { nullptr };           ///< 删除线
    QToolButton* mBtnSuperscript { nullptr };         ///< 上标
    QToolButton* mBtnSubscript { nullptr };           ///< 下标
    QToolButton* mBtnClearFormat { nullptr };         ///< 清除格式
    QTextEdit* mTextEdit { nullptr };                 ///< 富文本编辑区
    QPushButton* mBtnOk { nullptr };
    QPushButton* mBtnCancel { nullptr };
    bool mUpdating { false };  ///< setHtml等内部更新时阻断textChanged回环
};

DAChartTextEditorPopup::PrivateData::PrivateData(DAChartTextEditorPopup* p) : q_ptr(p)
{
}

/**
 * @brief 创建工具栏（字体面板+背景色+删除线/上下标/清除格式）
 * @param lay 工具栏水平布局
 */
void DAChartTextEditorPopup::PrivateData::buildFontPanel(QHBoxLayout* lay)
{
    mFontPanel = new DAFontEditPannelWidget(q_ptr);
    lay->addWidget(mFontPanel, 1);

    mBgColorButton = new DAColorPickerButton(q_ptr);
    mBgColorButton->setToolTip(QObject::tr("Text Background Color")  //cn:文字背景色
    );
    lay->addWidget(mBgColorButton);

    // 删除线：按钮文字自带删除线字体样式
    mBtnStrikeout = new QToolButton(q_ptr);
    {
        QFont f = mBtnStrikeout->font();
        f.setStrikeOut(true);
        mBtnStrikeout->setFont(f);
    }
    mBtnStrikeout->setText("S");
    mBtnStrikeout->setCheckable(true);
    mBtnStrikeout->setToolTip(QObject::tr("Strikethrough")  //cn:删除线
    );
    lay->addWidget(mBtnStrikeout);

    mBtnSuperscript = new QToolButton(q_ptr);
    mBtnSuperscript->setText(QStringLiteral("x\xC2\xB2"));  // x²
    mBtnSuperscript->setCheckable(true);
    mBtnSuperscript->setToolTip(QObject::tr("Superscript")  //cn:上标
    );
    lay->addWidget(mBtnSuperscript);

    mBtnSubscript = new QToolButton(q_ptr);
    mBtnSubscript->setText(QStringLiteral("x\xE2\x82\x82"));  // x₂
    mBtnSubscript->setCheckable(true);
    mBtnSubscript->setToolTip(QObject::tr("Subscript")  //cn:下标
    );
    lay->addWidget(mBtnSubscript);

    mBtnClearFormat = new QToolButton(q_ptr);
    mBtnClearFormat->setText(QObject::tr("Clear")  //cn:清除
    );
    mBtnClearFormat->setToolTip(QObject::tr("Clear character format of selected text")  //cn:清除选中文本的字符格式
    );
    lay->addWidget(mBtnClearFormat);
}

/**
 * @brief 创建编辑区与确认按钮行
 * @param lay 主垂直布局
 */
void DAChartTextEditorPopup::PrivateData::buildButtonRow(QVBoxLayout* lay)
{
    mTextEdit = new QTextEdit(q_ptr);
    mTextEdit->setAcceptRichText(true);
    mTextEdit->setMinimumHeight(80);
    mTextEdit->setMinimumWidth(340);
    lay->addWidget(mTextEdit, 1);

    QHBoxLayout* btnLay = new QHBoxLayout();
    btnLay->addStretch(1);
    mBtnCancel = new QPushButton(QObject::tr("Cancel")  //cn:取消
                                  ,
                                  q_ptr);
    btnLay->addWidget(mBtnCancel);
    mBtnOk = new QPushButton(QObject::tr("OK")  //cn:确定
                             ,
                             q_ptr);
    mBtnOk->setDefault(true);
    btnLay->addWidget(mBtnOk);
    lay->addLayout(btnLay);
}

//===================================================
// DAChartTextEditorPopup
//===================================================

/**
 * @brief 构造函数
 * @param parent 父窗口（弹窗独立于绘图窗口，通常传nullptr）
 */
DAChartTextEditorPopup::DAChartTextEditorPopup(QWidget* parent) : QWidget(parent, Qt::Popup), DA_PIMPL_CONSTRUCT
{
    buildUI();
}

/**
 * @brief 析构函数
 */
DAChartTextEditorPopup::~DAChartTextEditorPopup()
{
}

/**
 * @brief 构建界面与信号连接
 */
void DAChartTextEditorPopup::buildUI()
{
    DA_D(d);
    setAttribute(Qt::WA_DeleteOnClose, false);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(6, 6, 6, 6);
    mainLay->setSpacing(4);

    // 工具栏
    QHBoxLayout* toolLay = new QHBoxLayout();
    toolLay->setContentsMargins(0, 0, 0, 0);
    toolLay->setSpacing(2);
    d->buildFontPanel(toolLay);
    mainLay->addLayout(toolLay);

    // 编辑区与按钮
    d->buildButtonRow(mainLay);

    // 字体面板 -> QTextEdit
    connect(d->mFontPanel, &DAFontEditPannelWidget::currentFontChanged, this, &DAChartTextEditorPopup::onFontChanged);
    connect(
        d->mFontPanel, &DAFontEditPannelWidget::currentFontColorChanged, this, &DAChartTextEditorPopup::onFontColorChanged);
    // 背景色
    connect(d->mBgColorButton, &DAColorPickerButton::colorChanged, this, &DAChartTextEditorPopup::onBackgroundColorChanged);
    // 删除线/上标/下标/清除格式
    connect(d->mBtnStrikeout, &QToolButton::clicked, this, [ this ](bool on) {
        QTextCharFormat fmt;
        fmt.setFontStrikeOut(on);
        mergeCharFormat(fmt);
    });
    connect(d->mBtnSuperscript, &QToolButton::clicked, this, [ this ](bool on) {
        QTextCharFormat fmt;
        fmt.setVerticalAlignment(on ? QTextCharFormat::AlignSuperScript : QTextCharFormat::AlignNormal);
        mergeCharFormat(fmt);
    });
    connect(d->mBtnSubscript, &QToolButton::clicked, this, [ this ](bool on) {
        QTextCharFormat fmt;
        fmt.setVerticalAlignment(on ? QTextCharFormat::AlignSubScript : QTextCharFormat::AlignNormal);
        mergeCharFormat(fmt);
    });
    connect(d->mBtnClearFormat, &QToolButton::clicked, this, [ this, d ]() {
        QTextCursor cursor = d->mTextEdit->textCursor();
        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
        }
        cursor.setCharFormat(QTextCharFormat());
        d->mTextEdit->setTextCursor(cursor);
        Q_EMIT textChanged(toHtml());
    });
    // QTextEdit -> 面板状态回显
    connect(d->mTextEdit, &QTextEdit::currentCharFormatChanged, this, &DAChartTextEditorPopup::onTextEditCharFormatChanged);
    connect(d->mTextEdit, &QTextEdit::textChanged, this, &DAChartTextEditorPopup::onTextChanged);
    // 确认/取消
    connect(d->mBtnOk, &QPushButton::clicked, this, &DAChartTextEditorPopup::onAccept);
    connect(d->mBtnCancel, &QPushButton::clicked, this, &DAChartTextEditorPopup::onReject);
    // Esc拦截（Qt::Popup默认Esc行为无法区分确认/取消，这里统一转为取消）
    d->mTextEdit->installEventFilter(this);
}

/**
 * @brief 设置初始富文本内容
 * @param html 富文本HTML片段
 */
void DAChartTextEditorPopup::setHtml(const QString& html)
{
    DA_D(d);
    QSignalBlocker blocker(d->mTextEdit);
    d->mTextEdit->setHtml(html);
    d->mTextEdit->selectAll();
    // 全选状态下字体面板同步为首字符格式，便于用户直接输入覆盖
    QTextCharFormat fmt = d->mTextEdit->currentCharFormat();
    blocker.unblock();
    onTextEditCharFormatChanged(fmt);
}

/**
 * @brief 获取富文本内容
 * @return body内层HTML片段
 */
QString DAChartTextEditorPopup::toHtml() const
{
    return extractBodyHtml(d_ptr->mTextEdit->toHtml());
}

/**
 * @brief 获取纯文本内容
 * @return 纯文本
 */
QString DAChartTextEditorPopup::toPlainText() const
{
    return d_ptr->mTextEdit->toPlainText();
}

/**
 * @brief 在锚点附近弹出弹窗
 *
 * 候选方位优先级：左下 -> 右下 -> 左上 -> 右上，
 * 每个候选要求完整位于屏幕可用区域内，全部不满足时夹取到屏幕内
 * @param globalAnchor 锚点全局坐标
 */
void DAChartTextEditorPopup::popupAt(const QPoint& globalAnchor)
{
    adjustSize();
    const QSize sz = size();
    QScreen* screen = QGuiApplication::screenAt(globalAnchor);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        move(globalAnchor);
        execShow();
        return;
    }
    const QRect avail = screen->availableGeometry();
    const int margin  = 4;
    const QList< QPoint > candidates = {
        QPoint(globalAnchor.x() - sz.width() - margin, globalAnchor.y() + margin),   // 左下
        QPoint(globalAnchor.x() + margin, globalAnchor.y() + margin),                // 右下
        QPoint(globalAnchor.x() - sz.width() - margin, globalAnchor.y() - sz.height() - margin),  // 左上
        QPoint(globalAnchor.x() + margin, globalAnchor.y() - sz.height() - margin),  // 右上
    };
    for (const QPoint& c : candidates) {
        if (avail.contains(QRect(c, sz))) {
            move(c);
            execShow();
            return;
        }
    }
    // 全部越界，夹取到屏幕可用区域内
    int x = qBound(avail.left(), globalAnchor.x() - sz.width() - margin, avail.right() - sz.width());
    int y = qBound(avail.top(), globalAnchor.y() + margin, avail.bottom() - sz.height());
    move(x, y);
    execShow();
}

/**
 * @brief 显示弹窗并聚焦编辑区
 */
void DAChartTextEditorPopup::execShow()
{
    DA_D(d);
    show();
    raise();
    activateWindow();
    d->mTextEdit->setFocus(Qt::PopupFocusReason);
}

/**
 * @brief 拦截Esc转为取消
 * @param e 按键事件
 */
void DAChartTextEditorPopup::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        onReject();
        return;
    }
    QWidget::keyPressEvent(e);
}

/**
 * @brief 隐藏事件，判定关闭原因
 *
 * hideEvent是所有关闭路径的汇聚点：
 * - 点击外部导致的关闭不经过任何槽，状态仍为None，视为确认
 * @param e 隐藏事件
 */
void DAChartTextEditorPopup::hideEvent(QHideEvent* e)
{
    DA_D(d);
    QWidget::hideEvent(e);
    if (!d->mUpdating) {
        // 状态未被accept/reject设置，说明是点击外部关闭 -> 确认保留
        d->mUpdating = true;  // 防止重复发射
        Q_EMIT accepted();
    }
    d->mUpdating = false;
}

/**
 * @brief 事件过滤器，拦截编辑区的Esc
 * @param obj 监听对象
 * @param e 事件
 * @return 是否拦截
 */
bool DAChartTextEditorPopup::eventFilter(QObject* obj, QEvent* e)
{
    DA_D(d);
    if (obj == d->mTextEdit && e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast< QKeyEvent* >(e);
        if (ke->key() == Qt::Key_Escape) {
            onReject();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}

/**
 * @brief 应用字符格式到当前选中段
 * @param fmt 字符格式
 */
void DAChartTextEditorPopup::mergeCharFormat(const QTextCharFormat& fmt)
{
    DA_D(d);
    QTextCursor cursor = d->mTextEdit->textCursor();
    if (!cursor.hasSelection()) {
        // 无选中：作用于当前空字符，对后续输入生效
        d->mTextEdit->mergeCurrentCharFormat(fmt);
    } else {
        cursor.mergeCharFormat(fmt);
        d->mTextEdit->mergeCurrentCharFormat(fmt);
    }
    d->mTextEdit->setTextCursor(cursor);
}

/**
 * @brief 字体变化（字体族/字号/加粗/斜体/下划线）
 * @param f 新字体
 */
void DAChartTextEditorPopup::onFontChanged(const QFont& f)
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    QTextCharFormat fmt;
    fmt.setFont(f);
    mergeCharFormat(fmt);
    d->mTextEdit->setFocus(Qt::PopupFocusReason);
}

/**
 * @brief 文字颜色变化
 * @param c 新颜色
 */
void DAChartTextEditorPopup::onFontColorChanged(const QColor& c)
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    QTextCharFormat fmt;
    fmt.setForeground(c);
    mergeCharFormat(fmt);
    d->mTextEdit->setFocus(Qt::PopupFocusReason);
}

/**
 * @brief 文字背景色变化
 * @param c 新颜色，无效颜色(QColor())表示清除背景
 */
void DAChartTextEditorPopup::onBackgroundColorChanged(const QColor& c)
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    QTextCharFormat fmt;
    if (c.isValid()) {
        fmt.setBackground(c);
    } else {
        fmt.clearBackground();
    }
    mergeCharFormat(fmt);
    d->mTextEdit->setFocus(Qt::PopupFocusReason);
}

/**
 * @brief 编辑区当前格式变化，回显到字体面板与格式按钮
 * @param fmt 当前字符格式
 */
void DAChartTextEditorPopup::onTextEditCharFormatChanged(const QTextCharFormat& fmt)
{
    DA_D(d);
    QSignalBlocker blocker(this);  // 阻断本widget所有信号防止回环
    d->mUpdating = true;

    QFont f = fmt.font();
    d->mFontPanel->setCurrentFont(f);
    QColor fc = fmt.foreground().color();
    d->mFontPanel->setCurrentFontColor(fc.isValid() ? fc : Qt::black);

    QColor bg = fmt.background().color();
    d->mBgColorButton->setColor(bg);

    d->mBtnStrikeout->setChecked(fmt.fontStrikeOut());
    d->mBtnSuperscript->setChecked(fmt.verticalAlignment() == QTextCharFormat::AlignSuperScript);
    d->mBtnSubscript->setChecked(fmt.verticalAlignment() == QTextCharFormat::AlignSubScript);

    d->mUpdating = false;
}

/**
 * @brief 编辑区文本变化，发射textChanged
 */
void DAChartTextEditorPopup::onTextChanged()
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    Q_EMIT textChanged(toHtml());
}

/**
 * @brief 确认编辑
 */
void DAChartTextEditorPopup::onAccept()
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    d->mUpdating = true;  // hideEvent不再重复发射
    Q_EMIT accepted();
    hide();
    d->mUpdating = false;
}

/**
 * @brief 取消编辑
 */
void DAChartTextEditorPopup::onReject()
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    d->mUpdating = true;
    Q_EMIT rejected();
    hide();
    d->mUpdating = false;
}

}  // namespace DA
