#include "DATableNoteEditDialog.h"
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>

namespace DA
{

/**
 * @brief 构造
 * @param parent 父窗口
 */
DATableNoteEditDialog::DATableNoteEditDialog(QWidget* parent) : QDialog(parent)
{
    setupUI();
}

DATableNoteEditDialog::~DATableNoteEditDialog()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）：文本编辑 + 保存/删除/取消
 */
void DATableNoteEditDialog::setupUI()
{
    setWindowTitle(tr("Cell Note"));  // cn:单元格批注
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout* main = new QVBoxLayout(this);
    mEdit             = new QPlainTextEdit(this);
    mEdit->setObjectName(QStringLiteral("noteEditText"));
    mEdit->setPlaceholderText(tr("Enter note text..."));  // cn:输入批注文本...
    mEdit->setMinimumSize(280, 120);
    main->addWidget(mEdit);
    QHBoxLayout* btns = new QHBoxLayout();
    mBtnDelete        = new QPushButton(tr("Delete"), this);  // cn:删除
    mBtnDelete->setObjectName(QStringLiteral("noteDeleteBtn"));
    mBtnDelete->hide();  // 仅已有批注时显示
    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    box->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("noteSaveBtn"));
    btns->addWidget(mBtnDelete);
    btns->addStretch();
    btns->addWidget(box);
    main->addLayout(btns);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(mBtnDelete, &QPushButton::clicked, this, &DATableNoteEditDialog::onDeleteClicked);
}

void DATableNoteEditDialog::setNoteText(const QString& text)
{
    mEdit->setPlainText(text);
}

QString DATableNoteEditDialog::noteText() const
{
    return mEdit->toPlainText();
}

void DATableNoteEditDialog::setExistingNote(bool existing)
{
    mBtnDelete->setVisible(existing);
}

bool DATableNoteEditDialog::isDeleteRequested() const
{
    return mDeleteRequested;
}

/**
 * @brief 点「删除」：标记删除意图并按接受关闭（调用方据此生成删除命令）
 */
void DATableNoteEditDialog::onDeleteClicked()
{
    mDeleteRequested = true;
    accept();
}

/**
 * @brief 跟随单元格定位：弹层左上角贴单元格右下角，出屏时夹回屏幕内
 * @param globalCellCorner 单元格右下角全局坐标
 */
void DATableNoteEditDialog::popupNear(const QPoint& globalCellCorner)
{
    ensurePolished();
    const QSize sz = sizeHint();
    QPoint pos     = globalCellCorner;
    QScreen* scr   = QGuiApplication::screenAt(globalCellCorner);
    if (!scr) {
        scr = QGuiApplication::primaryScreen();
    }
    if (scr) {
        const QRect avail = scr->availableGeometry();
        if (pos.x() + sz.width() > avail.right()) {
            pos.setX(qMax(avail.left(), avail.right() - sz.width()));
        }
        if (pos.y() + sz.height() > avail.bottom()) {
            pos.setY(qMax(avail.top(), globalCellCorner.y() - sz.height()));
        }
    }
    move(pos);
}

}  // end of namespace DA
