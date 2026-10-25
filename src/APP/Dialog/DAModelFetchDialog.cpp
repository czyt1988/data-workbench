// DAModelFetchDialog.cpp
#include "DAModelFetchDialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "DACheckListWidget.h"

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
    QVector< DAItemEntry > entries;
    entries.reserve(modelIds.size());
    for (const QString& id : modelIds) {
        entries.append(DAItemEntry { id, QVariant() });
    }
    mList->setItems(entries);
    mList->setCheckedEntries(entries);
}

/** @brief 构建界面 */
void DAModelFetchDialog::buildUi()
{
    setWindowTitle(tr("Available Models"));  // cn:可用模型
    setMinimumSize(360, 320);
    QVBoxLayout* root = new QVBoxLayout(this);

    root->addWidget(new QLabel(tr("Select models to add:")));  // cn:选择要添加的模型：

    mList = new DACheckListWidget(this);
    mList->setSelectAllVisible(true);
    mList->setInvertVisible(true);
    root->addWidget(mList, 1);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bb->button(QDialogButtonBox::Ok)->setText(tr("OK"));          // cn:确定
    bb->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    root->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/** @brief 返回勾选的模型 id 列表 */
QStringList DAModelFetchDialog::getSelectedModelIds() const
{
    QStringList out;
    const QVector< DAItemEntry > checked = mList->checkedEntries();
    for (const DAItemEntry& e : checked) {
        out.append(e.text);
    }
    return out;
}

}  // namespace DA
