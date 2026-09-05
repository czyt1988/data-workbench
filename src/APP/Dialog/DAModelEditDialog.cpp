// DAModelEditDialog.cpp
#include "DAModelEditDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QPushButton>

namespace DA
{

/**
 * @brief 构造
 * @param id 初始模型 id
 * @param contextWindow 初始上下文窗口
 * @param maxOutputTokens 初始最大输出 token
 * @param existingIds 已存在的模型 id 列表
 * @param oldId 修改模式下的旧 id
 * @param parent 父窗口
 */
DAModelEditDialog::DAModelEditDialog(const QString& id, int contextWindow, int maxOutputTokens,
                                     const QStringList& existingIds, const QString& oldId,
                                     QWidget* parent)
    : QDialog(parent)
    , m_existingIds(existingIds)
    , m_oldId(oldId)
{
    buildUi();
    m_idEdit->setText(id);
    m_ctxSpin->setValue(contextWindow > 0 ? contextWindow : 262144);
    m_maxOutSpin->setValue(maxOutputTokens > 0 ? maxOutputTokens : 131072);
}

/** @brief 构建界面 */
void DAModelEditDialog::buildUi()
{
    setWindowTitle(tr("Model"));  // cn:模型
    QVBoxLayout* root = new QVBoxLayout(this);

    QFormLayout* form = new QFormLayout();
    m_idEdit = new QLineEdit(this);
    m_idEdit->setPlaceholderText(tr("model id, e.g. gpt-4o"));  // cn:模型 id，如 gpt-4o
    m_ctxSpin = new QSpinBox(this);
    m_ctxSpin->setRange(1024, 2097152);
    m_ctxSpin->setSingleStep(1024);
    m_ctxSpin->setSuffix(tr(" tokens"));  // cn: token
    m_ctxSpin->setValue(262144);
    m_maxOutSpin = new QSpinBox(this);
    m_maxOutSpin->setRange(1, 1048576);
    m_maxOutSpin->setSingleStep(128);
    m_maxOutSpin->setSuffix(tr(" tokens"));  // cn: token
    m_maxOutSpin->setValue(8192);
    form->addRow(tr("Model Id"), m_idEdit);              // cn:模型 id
    form->addRow(tr("Context Window"), m_ctxSpin);       // cn:上下文窗口
    form->addRow(tr("Max Output Tokens"), m_maxOutSpin); // cn:最大输出 token
    root->addLayout(form);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bb->button(QDialogButtonBox::Ok)->setText(tr("OK"));      // cn:确定
    bb->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    root->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &DAModelEditDialog::onOkClicked);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/** @brief OK 按钮点击：校验通过才 accept */
void DAModelEditDialog::onOkClicked()
{
    if (validate()) {
        accept();
    }
}

/** @brief 校验：模型 id 必填且不与其它模型重名 */
bool DAModelEditDialog::validate()
{
    QString id = m_idEdit->text().trimmed();
    if (id.isEmpty()) {
        m_idEdit->setFocus();
        return false;
    }
    // 重名校验：排除自身旧 id
    for (const QString& ex : std::as_const(m_existingIds)) {
        if (ex == id && ex != m_oldId) {
            m_idEdit->setFocus();
            return false;
        }
    }
    return true;
}

/** @brief 获取模型 id */
QString DAModelEditDialog::getModelId() const { return m_idEdit->text().trimmed(); }

/** @brief 获取上下文窗口 */
int DAModelEditDialog::getContextWindow() const { return m_ctxSpin->value(); }

/** @brief 获取最大输出 token */
int DAModelEditDialog::getMaxOutputTokens() const { return m_maxOutSpin->value(); }

}  // namespace DA
