// DAModelFetchDialog.cpp
#include "DAModelFetchDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QDialogButtonBox>

namespace DA
{

/**
 * @brief 构造
 * @param modelIds 获取到的模型 id 列表
 * @param parent 父窗口
 */
DAModelFetchDialog::DAModelFetchDialog(const QStringList& modelIds, QWidget* parent)
    : QDialog(parent)
{
    buildUi();
    // 默认全部勾选
    for (const QString& id : modelIds) {
        QListWidgetItem* item = new QListWidgetItem(id, mList);
        item->setCheckState(Qt::Checked);
    }
}

/** @brief 构建界面 */
void DAModelFetchDialog::buildUi()
{
    setWindowTitle(tr("Available Models"));  // cn:可用模型
    setMinimumSize(360, 320);
    QVBoxLayout* root = new QVBoxLayout(this);

    root->addWidget(new QLabel(tr("Select models to add:")));  // cn:选择要添加的模型：

    mList = new QListWidget(this);
    root->addWidget(mList, 1);

    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* selAll = new QPushButton(tr("Select All"), this);      // cn:全选
    QPushButton* deselAll = new QPushButton(tr("Deselect All"), this);  // cn:全不选
    btnRow->addWidget(selAll);
    btnRow->addWidget(deselAll);
    btnRow->addStretch();
    root->addLayout(btnRow);
    connect(selAll, &QPushButton::clicked, this, &DAModelFetchDialog::onSelectAll);
    connect(deselAll, &QPushButton::clicked, this, &DAModelFetchDialog::onDeselectAll);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bb->button(QDialogButtonBox::Ok)->setText(tr("OK"));      // cn:确定
    bb->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    root->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/** @brief 全选 */
void DAModelFetchDialog::onSelectAll()
{
    for (int i = 0; i < mList->count(); ++i) {
        mList->item(i)->setCheckState(Qt::Checked);
    }
}

/** @brief 全不选 */
void DAModelFetchDialog::onDeselectAll()
{
    for (int i = 0; i < mList->count(); ++i) {
        mList->item(i)->setCheckState(Qt::Unchecked);
    }
}

/** @brief 返回勾选的模型 id 列表 */
QStringList DAModelFetchDialog::getSelectedModelIds() const
{
    QStringList out;
    for (int i = 0; i < mList->count(); ++i) {
        QListWidgetItem* item = mList->item(i);
        if (item->checkState() == Qt::Checked) {
            out.append(item->text());
        }
    }
    return out;
}

}  // namespace DA
