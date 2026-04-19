#include "DACollapsiblePanel.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include "DAGlobals.h"

namespace DA
{

//===================================================
// DACollapsiblePanel::PrivateData
//===================================================

class DACollapsiblePanel::PrivateData
{
    DA_DECLARE_PUBLIC(DACollapsiblePanel)
public:
    PrivateData(DACollapsiblePanel* p);

    QWidget* mContentWidget                  = nullptr;
    DACollapsiblePanel::StyleMode mStyleMode = DACollapsiblePanel::Flat;
};

DACollapsiblePanel::PrivateData::PrivateData(DACollapsiblePanel* p) : q_ptr(p)
{
    if (p->objectName().isEmpty()) {
        p->setObjectName(QStringLiteral("DACollapsiblePanel"));
    }
}

//===================================================
// DACollapsiblePanel
//===================================================

/**
 * @brief 构造函数（无标题）
 * @param[in] parent 父控件
 *
 * 默认展开状态（setChecked(true)），因为ctkCollapsibleGroupBox默认是折叠的(checked=false)，
 * 需要显式设置为展开以保持与原有DACollapsiblePanel的行为一致。
 * 默认Flat模式（setFlat(true)），无边框。
 */
DACollapsiblePanel::DACollapsiblePanel(QWidget* parent) : DACollapsibleGroupBox(parent), DA_PIMPL_CONSTRUCT
{
    setChecked(true);  // ctkCollapsibleGroupBox默认checked=false(折叠)，显式设为展开
    setFlat(true);     // 默认Flat模式，无边框
    // 连接父类toggled信号到expandedChanged信号
    // toggled(bool)的参数含义：checked=true→展开=true，与expanded语义一致
    connect(this, &QGroupBox::toggled, this, &DACollapsiblePanel::expandedChanged);
}

/**
 * @brief 构造函数（带标题）
 * @param[in] title 面板标题
 * @param[in] parent 父控件
 *
 * 默认展开状态，标题通过父类DACollapsibleGroupBox::setTitle设置。
 */
DACollapsiblePanel::DACollapsiblePanel(const QString& title, QWidget* parent)
    : DACollapsibleGroupBox(parent), DA_PIMPL_CONSTRUCT
{
    setChecked(true);  // 默认展开
    setFlat(true);     // 默认Flat模式
    DACollapsibleGroupBox::setTitle(title);
    connect(this, &QGroupBox::toggled, this, &DACollapsiblePanel::expandedChanged);
}

/**
 * @brief 析构函数
 */
DACollapsiblePanel::~DACollapsiblePanel()
{
}

/**
 * @brief 设置内容区域控件
 * @param[in] widget 内容控件（若为nullptr则创建默认容器）
 *
 * 内容控件将作为QGroupBox布局中的子控件，折叠/展开由ctkCollapsibleGroupBox的
 * childEvent/eventFilter机制自动管理可见性，无需手动调用setVisible。
 */
void DACollapsiblePanel::setContentWidget(QWidget* widget)
{
    DA_D(d);
    if (d->mContentWidget) {
        QLayout* lay = layout();
        if (lay) {
            lay->removeWidget(d->mContentWidget);
        }
        d->mContentWidget->deleteLater();
    }

    d->mContentWidget = widget ? widget : new QWidget(this);
    d->mContentWidget->setObjectName(QStringLiteral("contentWidget"));

    // 确保QGroupBox有布局（QGroupBox不会自动创建布局）
    if (!layout()) {
        QVBoxLayout* contentLayout = new QVBoxLayout(this);
        contentLayout->setObjectName(QStringLiteral("contentLayout"));
        contentLayout->setContentsMargins(4, 4, 4, 4);
        contentLayout->setSpacing(4);
    }

    layout()->addWidget(d->mContentWidget);
}

/**
 * @brief 获取内容区域控件
 * @return 当前内容控件指针
 */
QWidget* DACollapsiblePanel::getContentWidget() const
{
    DA_DC(d);
    return d->mContentWidget;
}

/**
 * @brief 设置展开/收起状态
 * @param[in] expanded true为展开，false为收起
 *
 * 通过调用父类的setCollapsed实现折叠控制。
 * expanded与collapsed是反向关系：expanded=true → collapsed=false → checked=true。
 * 不需要手动emit expandedChanged，因为toggled信号会自动触发expandedChanged。
 */
void DACollapsiblePanel::setExpanded(bool expanded)
{
    setCollapsed(!expanded);
}

/**
 * @brief 获取当前展开状态
 * @return true为展开，false为收起
 *
 * expanded与collapsed是反向关系：expanded = !collapsed = isChecked()
 */
bool DACollapsiblePanel::isExpanded() const
{
    return !collapsed();
}

/**
 * @brief 设置面板标题
 * @param[in] title 新标题文本
 *
 * 调用父类DACollapsibleGroupBox::setTitle设置标题，并手动emit titleChanged信号
 * （QGroupBox没有titleChanged信号，需要手动触发）。
 */
void DACollapsiblePanel::setTitle(const QString& title)
{
    DACollapsibleGroupBox::setTitle(title);
    Q_EMIT titleChanged(title);
}

/**
 * @brief 获取面板标题
 * @return 当前标题文本
 */
QString DACollapsiblePanel::title() const
{
    return DACollapsibleGroupBox::title();
}

/**
 * @brief 设置面板样式模式
 * @param[in] mode 样式模式（Flat/GroupBox/Bordered）
 *
 * 根据样式模式调整QGroupBox的flat属性和布局边距：
 * - Flat模式：setFlat(true)，无边框，布局边距最小
 * - GroupBox模式：setFlat(false)，使用QGroupBox原生边框绘制
 * - Bordered模式：setFlat(true)抑制原生边框，paintEvent自绘矩形边框，布局有外边距
 */
void DACollapsiblePanel::setStyleMode(StyleMode mode)
{
    DA_D(d);
    if (d->mStyleMode == mode) {
        return;
    }
    d->mStyleMode = mode;

    // 根据样式模式设置flat属性和布局边距
    switch (mode) {
    case Flat:
        setFlat(true);
        if (layout()) {
            layout()->setContentsMargins(0, 0, 0, 0);
        }
        break;
    case GroupBox:
        setFlat(false);
        if (layout()) {
            layout()->setContentsMargins(0, 0, 0, 0);
        }
        break;
    case Bordered:
        setFlat(true);  // 抑制QGroupBox原生边框，paintEvent自绘
        if (layout()) {
            layout()->setContentsMargins(2, 2, 2, 2);
        }
        break;
    }

    update();  // 触发重绘
    updateGeometry();
    emit styleModeChanged(mode);
}

/**
 * @brief 获取当前样式模式
 * @return 当前样式模式
 */
DACollapsiblePanel::StyleMode DACollapsiblePanel::styleMode() const
{
    DA_DC(d);
    return d->mStyleMode;
}

/**
 * @brief 绘制事件，根据样式模式绘制边框
 * @param[in] event 绘制事件
 *
 * - Flat模式：调用QGroupBox::paintEvent（setFlat=true，QGroupBox绘制标题和箭头，无边框）
 * - GroupBox模式：调用QGroupBox::paintEvent（setFlat=false，QGroupBox绘制原生GroupBox边框和标题）
 * - Bordered模式：先调用QGroupBox::paintEvent（setFlat=true绘制标题和箭头），再自绘矩形边框
 */
void DACollapsiblePanel::paintEvent(QPaintEvent* event)
{
    DA_D(d);

    // 先调用父类QGroupBox::paintEvent绘制标题和箭头指示器
    QGroupBox::paintEvent(event);

    // Bordered模式额外自绘矩形边框包围整个面板（标题+内容）
    if (d->mStyleMode == Bordered) {
        QPainter painter(this);
        painter.setClipRect(event->rect());

        QPalette pal = palette();
        QPen borderPen(pal.color(QPalette::Window).lighter(120));
        borderPen.setWidth(1);
        painter.setPen(borderPen);
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
}

}  // namespace DA
