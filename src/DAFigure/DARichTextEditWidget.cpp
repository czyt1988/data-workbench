#include "DARichTextEditWidget.h"
#include "DAFontEditPannelWidget.h"
#include "DAColorPickerButton.h"

#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QTextCursor>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QRegularExpression>

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
    static QRegularExpression re(R"(<body[^>]*>(.*)</body>)",
                                 QRegularExpression::DotMatchesEverythingOption
                                     | QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(fullHtml);
    if (m.hasMatch()) {
        return m.captured(1).trimmed();
    }
    return fullHtml;
}

class DARichTextEditWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DARichTextEditWidget)
public:
    PrivateData(DARichTextEditWidget* p);
    void buildFontPanel(QHBoxLayout* lay);

    DAFontEditPannelWidget* mFontPanel { nullptr };  ///< 字体编辑面板(族/字号/粗斜下划线/文字颜色)
    DAColorPickerButton* mBgColorButton { nullptr };  ///< 文字背景色
    QToolButton* mBtnStrikeout { nullptr };           ///< 删除线
    QToolButton* mBtnSuperscript { nullptr };         ///< 上标
    QToolButton* mBtnSubscript { nullptr };           ///< 下标
    QToolButton* mBtnClearFormat { nullptr };         ///< 清除格式
    QTextEdit* mTextEdit { nullptr };                 ///< 富文本编辑区
    bool mUpdating { false };                         ///< setHtml等内部更新时阻断textChanged回环
};

DARichTextEditWidget::PrivateData::PrivateData(DARichTextEditWidget* p) : q_ptr(p)
{
}

/**
 * @brief 创建工具栏（字体面板+背景色+删除线/上下标/清除格式）
 * @param lay 工具栏水平布局
 */
void DARichTextEditWidget::PrivateData::buildFontPanel(QHBoxLayout* lay)
{
    mFontPanel = new DAFontEditPannelWidget(q_ptr);
    // 弹窗工具栏高度受限，字体面板使用单排布局
    mFontPanel->setLayoutMode(DAFontEditPannelWidget::SingleRowLayout);
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

//===================================================
// DARichTextEditWidget
//===================================================

/**
 * @brief 构造富文本编辑控件
 * @param parent 父控件
 */
DARichTextEditWidget::DARichTextEditWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    buildUI();
}

/**
 * @brief 析构
 */
DARichTextEditWidget::~DARichTextEditWidget()
{
}

/**
 * @brief 构建界面与信号连接
 */
void DARichTextEditWidget::buildUI()
{
    DA_D(d);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(4);

    // 工具栏
    QHBoxLayout* toolLay = new QHBoxLayout();
    toolLay->setContentsMargins(0, 0, 0, 0);
    toolLay->setSpacing(2);
    d->buildFontPanel(toolLay);
    mainLay->addLayout(toolLay);

    // 编辑区
    d->mTextEdit = new QTextEdit(this);
    d->mTextEdit->setAcceptRichText(true);
    d->mTextEdit->setMinimumHeight(80);
    d->mTextEdit->setMinimumWidth(340);
    mainLay->addWidget(d->mTextEdit, 1);

    // 字体面板 -> QTextEdit
    connect(d->mFontPanel, &DAFontEditPannelWidget::currentFontChanged, this, &DARichTextEditWidget::onFontChanged);
    connect(d->mFontPanel, &DAFontEditPannelWidget::currentFontColorChanged, this, &DARichTextEditWidget::onFontColorChanged);
    // 背景色
    connect(d->mBgColorButton, &DAColorPickerButton::colorChanged, this, &DARichTextEditWidget::onBackgroundColorChanged);
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
    connect(d->mTextEdit, &QTextEdit::currentCharFormatChanged, this, &DARichTextEditWidget::onTextEditCharFormatChanged);
    connect(d->mTextEdit, &QTextEdit::textChanged, this, &DARichTextEditWidget::onTextChanged);
}

/**
 * @brief 设置初始富文本内容
 * @param html 富文本HTML片段
 */
void DARichTextEditWidget::setHtml(const QString& html)
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
QString DARichTextEditWidget::toHtml() const
{
    return extractBodyHtml(d_ptr->mTextEdit->toHtml());
}

/**
 * @brief 获取纯文本内容
 * @return 纯文本
 */
QString DARichTextEditWidget::toPlainText() const
{
    return d_ptr->mTextEdit->toPlainText();
}

/**
 * @brief 获取内部编辑区
 * @return QTextEdit
 */
QTextEdit* DARichTextEditWidget::textEdit() const
{
    return d_ptr->mTextEdit;
}

/**
 * @brief 应用字符格式到当前选中段
 * @param fmt 字符格式
 */
void DARichTextEditWidget::mergeCharFormat(const QTextCharFormat& fmt)
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
void DARichTextEditWidget::onFontChanged(const QFont& f)
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
void DARichTextEditWidget::onFontColorChanged(const QColor& c)
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
void DARichTextEditWidget::onBackgroundColorChanged(const QColor& c)
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
void DARichTextEditWidget::onTextEditCharFormatChanged(const QTextCharFormat& fmt)
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
void DARichTextEditWidget::onTextChanged()
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    Q_EMIT textChanged(toHtml());
}

}  // namespace DA
