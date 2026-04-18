#include "DACollapsiblePanel.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QStyle>
#include <QPainter>
#include <QFontMetrics>
#include "DAGlobals.h"

namespace DA
{

/**
 * @brief 可折叠面板的头部控件，负责绘制箭头指示器和标题文本
 */
class DACollapsiblePanelHeader : public QFrame
{
    Q_OBJECT
public:
    explicit DACollapsiblePanelHeader(const QString& title, QWidget* parent = nullptr)
        : QFrame(parent), mTitle(title)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void setTitle(const QString& title)
    {
        mTitle = title;
        update();
    }

    QString title() const { return mTitle; }

    void setExpanded(bool expanded)
    {
        mExpanded = expanded;
        update();
    }

    QSize sizeHint() const override
    {
        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(mTitle) + mArrowSize + 8 + mMargin * 2;
        int textHeight = fm.height() + 8;
        return QSize(textWidth, textHeight);
    }

Q_SIGNALS:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
        QFrame::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setClipRect(event->rect());

        // 绘制背景确保视觉一致
        QStyleOption opt;
        opt.initFrom(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

        // 绘制箭头
        QStyleOption arrowOpt;
        arrowOpt.initFrom(this);
        arrowOpt.rect = arrowRect();
        if (mExpanded) {
            style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrowOpt, &painter, this);
        } else {
            style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &arrowOpt, &painter, this);
        }

        // 绘制标题文本
        QFontMetrics fm(font());
        int textX = mArrowSize + 4 + mMargin;
        int textY = (height() - fm.height()) / 2 + fm.ascent();
        painter.drawText(textX, textY, titleWidth(), fm.height(), Qt::AlignLeft | Qt::AlignVCenter, mTitle);
    }

private:
    QRect arrowRect() const
    {
        return QRect(mMargin, (height() - mArrowSize) / 2, mArrowSize, mArrowSize);
    }

    int titleWidth() const
    {
        return width() - mArrowSize - 8 - mMargin * 2;
    }

    QString mTitle;
    bool mExpanded = true;
    int mArrowSize = 10;
    int mMargin = 6;
};

//===================================================
// DACollapsiblePanel::PrivateData
//===================================================

class DACollapsiblePanel::PrivateData
{
    DA_DECLARE_PUBLIC(DACollapsiblePanel)
public:
    PrivateData(DACollapsiblePanel* p);

    DACollapsiblePanelHeader* mHeaderWidget = nullptr;
    QWidget* mContentWidget = nullptr;
    QVBoxLayout* mContentLayout = nullptr;
    QVBoxLayout* mMainLayout = nullptr;

    bool mExpanded = true;
    QString mTitle;
};

DACollapsiblePanel::PrivateData::PrivateData(DACollapsiblePanel* p) : q_ptr(p)
{
    if (p->objectName().isEmpty()) {
        p->setObjectName(QStringLiteral("DACollapsiblePanel"));
    }

    // 创建主布局（垂直排列）
    mMainLayout = new QVBoxLayout(p);
    mMainLayout->setObjectName(QStringLiteral("mainLayout"));
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->setSpacing(0);

    // 创建头部控件
    mHeaderWidget = new DACollapsiblePanelHeader(QString(), p);
    mHeaderWidget->setObjectName(QStringLiteral("headerWidget"));
    mHeaderWidget->setExpanded(true);
    mMainLayout->addWidget(mHeaderWidget);
}

//===================================================
// DACollapsiblePanel
//===================================================

/**
 * @brief 构造函数（无标题）
 * @param[in] parent 父控件
 */
DACollapsiblePanel::DACollapsiblePanel(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    connect(d->mHeaderWidget, &DACollapsiblePanelHeader::clicked, this, &DACollapsiblePanel::onHeaderClicked);
}

/**
 * @brief 构造函数（带标题）
 * @param[in] title 面板标题
 * @param[in] parent 父控件
 */
DACollapsiblePanel::DACollapsiblePanel(const QString& title, QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mTitle = title;
    d->mHeaderWidget->setTitle(title);
    d->mHeaderWidget->setExpanded(true);
    connect(d->mHeaderWidget, &DACollapsiblePanelHeader::clicked, this, &DACollapsiblePanel::onHeaderClicked);
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
 * 内容控件将作为折叠/展开的目标。当面板收起时，contentWidget->setVisible(false)；
 * 展开时contentWidget->setVisible(true)。sizeHint据此动态调整高度。
 */
void DACollapsiblePanel::setContentWidget(QWidget* widget)
{
    DA_D(d);
    if (d->mContentWidget) {
        d->mMainLayout->removeWidget(d->mContentWidget);
        d->mContentWidget->deleteLater();
    }

    d->mContentWidget = widget ? widget : new QWidget(this);
    d->mContentWidget->setObjectName(QStringLiteral("contentWidget"));
    d->mContentLayout = new QVBoxLayout(d->mContentWidget);
    d->mContentLayout->setObjectName(QStringLiteral("contentLayout"));
    d->mContentLayout->setContentsMargins(4, 4, 4, 4);
    d->mContentLayout->setSpacing(4);
    d->mMainLayout->addWidget(d->mContentWidget);

    // 根据当前展开状态设置可见性
    d->mContentWidget->setVisible(d->mExpanded);
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
 */
void DACollapsiblePanel::setExpanded(bool expanded)
{
    DA_D(d);
    if (d->mExpanded == expanded) {
        return;
    }
    d->mExpanded = expanded;
    d->mHeaderWidget->setExpanded(expanded);
    if (d->mContentWidget) {
        d->mContentWidget->setVisible(expanded);
    }
    updateGeometry();
    emit expandedChanged(expanded);
}

/**
 * @brief 获取当前展开状态
 * @return true为展开，false为收起
 */
bool DACollapsiblePanel::isExpanded() const
{
    DA_DC(d);
    return d->mExpanded;
}

/**
 * @brief 设置面板标题
 * @param[in] title 新标题文本
 */
void DACollapsiblePanel::setTitle(const QString& title)
{
    DA_D(d);
    d->mTitle = title;
    d->mHeaderWidget->setTitle(title);
    emit titleChanged(title);
}

/**
 * @brief 获取面板标题
 * @return 当前标题文本
 */
QString DACollapsiblePanel::title() const
{
    DA_DC(d);
    return d->mTitle;
}

/**
 * @brief 计算推荐大小
 * 
 * 展开状态：头部高度 + 内容区域高度
 * 收起状态：仅头部高度
 */
QSize DACollapsiblePanel::sizeHint() const
{
    DA_DC(d);
    QSize hint = d->mHeaderWidget->sizeHint();
    if (d->mContentWidget && d->mExpanded) {
        QSize contentSize = d->mContentWidget->sizeHint();
        hint.setWidth(qMax(hint.width(), contentSize.width()));
        hint.setHeight(hint.height() + contentSize.height());
    }
    return hint;
}

/**
 * @brief 头部点击槽函数，切换展开/收起状态
 */
void DACollapsiblePanel::onHeaderClicked()
{
    DA_D(d);
    setExpanded(!d->mExpanded);
}

/**
 * @brief 窗口大小变化事件
 */
void DACollapsiblePanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 内容宽度自动跟随面板宽度调整
}

}  // namespace DA

#include "DACollapsiblePanel.moc"
