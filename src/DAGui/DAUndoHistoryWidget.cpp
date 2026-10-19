#include "DAUndoHistoryWidget.h"
#include <QUndoGroup>
#include <QUndoView>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace DA
{

/**
 * @brief 构造：撤销深度调节行 + QUndoView
 * @param parent 父对象
 */
DAUndoHistoryWidget::DAUndoHistoryWidget(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(4, 4, 4, 4);
    // 深度调节行（R19：默认 20，面板可调大；配置持久化在 APP 侧）
    QHBoxLayout* limitRow = new QHBoxLayout();
    QLabel* label         = new QLabel(tr("Undo depth:"), this);  // cn:撤销深度：
    mSpinLimit            = new QSpinBox(this);
    mSpinLimit->setObjectName(QStringLiteral("undoLimitSpin"));
    mSpinLimit->setRange(1, 10000);
    mSpinLimit->setValue(20);
    mSpinLimit->setToolTip(tr("Maximum number of undo steps kept per table page (larger uses more memory for swap-type commands)"));  // cn:每个表格页保留的最大撤销步数（调大对转置类命令会增加内存占用）
    limitRow->addWidget(label);
    limitRow->addWidget(mSpinLimit);
    limitRow->addStretch();
    main->addLayout(limitRow);
    // 历史列表（QUndoView：跟随激活栈、点击条目回退/前进、显示命令描述）
    mView = new QUndoView(this);
    mView->setObjectName(QStringLiteral("undoHistoryView"));
    main->addWidget(mView, 1);
    connect(mSpinLimit, &QSpinBox::valueChanged, this, [ this ](int v) {
        if (mUpdatingLimit) {
            return;
        }
        Q_EMIT undoLimitChangeRequested(v);
    });
}

DAUndoHistoryWidget::~DAUndoHistoryWidget()
{
}

/**
 * @brief 绑定全局撤销组（视图自动跟随组内激活栈）
 * @param group QUndoGroup（非拥有，可空=显示空列表）
 */
void DAUndoHistoryWidget::setUndoGroup(QUndoGroup* group)
{
    if (mView) {
        mView->setGroup(group);
    }
}

QUndoGroup* DAUndoHistoryWidget::undoGroup() const
{
    return mView ? mView->group() : nullptr;
}

int DAUndoHistoryWidget::undoLimit() const
{
    return mSpinLimit ? mSpinLimit->value() : 20;
}

/**
 * @brief 设置深度显示值（APP 读配置注入；屏蔽信号避免回环写配置）
 * @param limit 撤销深度
 */
void DAUndoHistoryWidget::setUndoLimit(int limit)
{
    if (!mSpinLimit) {
        return;
    }
    mUpdatingLimit = true;
    mSpinLimit->setValue(qBound(mSpinLimit->minimum(), limit, mSpinLimit->maximum()));
    mUpdatingLimit = false;
}

}  // end of namespace DA
