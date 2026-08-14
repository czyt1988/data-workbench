#include "DAAgentEditorDialog.h"
#include "DAMarkdownHighlighter.h"
// Qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QStyle>
#include <QMessageBox>
#include <QFont>

namespace DA
{
DAAgentEditorDialog::DAAgentEditorDialog(const QString& title,
                                         const QString& content,
                                         const QStringList& existingTitles,
                                         const QString& oldTitle,
                                         QWidget* parent)
    : QDialog(parent), m_existingTitles(existingTitles), m_oldTitle(oldTitle)
{
    buildUi();
    m_titleEdit->setText(title);
    m_contentEdit->setPlainText(content);
    onTextChanged();
    setWindowTitle(oldTitle.isEmpty() ? tr("New Agent")  //cn:新增 Agent
                                      : tr("Edit Agent")); //cn:编辑 Agent
}

void DAAgentEditorDialog::buildUi()
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // 标题输入
    auto* titleLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel(tr("Title:"), this);  //cn:标题：
    titleLabel->setMinimumWidth(40);
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("Enter agent title"));  //cn:请输入 Agent 标题
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(m_titleEdit);
    mainLayout->addLayout(titleLayout);

    // 内容编辑器
    auto* contentLabel = new QLabel(tr("Prompt content:"), this);  //cn:提示词内容：
    mainLayout->addWidget(contentLabel);

    m_contentEdit = new QPlainTextEdit(this);
    QFont monoFont("Consolas");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(10);
    m_contentEdit->setFont(monoFont);
    m_contentEdit->setPlaceholderText(tr("Write prompt content here (Markdown supported)"));  //cn:在此编写提示词内容（支持 Markdown）
    m_highlighter = new DAMarkdownHighlighter(m_contentEdit->document());
    mainLayout->addWidget(m_contentEdit, 1);

    // 底部按钮
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    auto* saveBtn = new QPushButton(tr("Save"), this);    //cn:保存
    saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);  //cn:取消
    cancelBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    resize(640, 520);

    connect(saveBtn, &QPushButton::clicked, this, &DAAgentEditorDialog::onSaveClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_titleEdit, &QLineEdit::textChanged, this, &DAAgentEditorDialog::onTextChanged);
    connect(m_contentEdit, &QPlainTextEdit::textChanged, this, &DAAgentEditorDialog::onTextChanged);
}

void DAAgentEditorDialog::onTextChanged()
{
    // 占位：可在此刷新保存按钮可用态，当前始终可用，由 validate 兜底
}

bool DAAgentEditorDialog::validate()
{
    QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, tr("Tip"), tr("Title cannot be empty"));  //cn:提示 //cn:标题不能为空
        m_titleEdit->setFocus();
        return false;
    }
    // 重名检测：排除自身旧标题
    for (const QString& t : std::as_const(m_existingTitles)) {
        if (t == title && t != m_oldTitle) {
            QMessageBox::warning(this, tr("Tip"),  //cn:提示
                                  tr("An agent named \"%1\" already exists, please choose another title").arg(title));  //cn:已存在同名 Agent「%1」，请更换标题
            m_titleEdit->setFocus();
            return false;
        }
    }
    return true;
}

void DAAgentEditorDialog::onSaveClicked()
{
    if (!validate()) {
        return;
    }
    accept();
}

QString DAAgentEditorDialog::getTitle() const
{
    return m_titleEdit->text().trimmed();
}

QString DAAgentEditorDialog::getContent() const
{
    return m_contentEdit->toPlainText();
}
} // namespace DA
