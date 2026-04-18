#include "DACollapsiblePanel.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QStyle>
#include <QPainter>
#include <QPaintEvent>
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
    explicit DACollapsiblePanelHeader(const QString& title, QWidget* parent = nullptr) : QFrame(parent), mTitle(title)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void setTitle(const QString& title)
    {
        mTitle = title;
        update();
    }

    QString title() const
    {
        return mTitle;
    }

    void setExpanded(bool expanded)
    {
        mExpanded = expanded;
        update();
    }

    void setStyleMode(DACollapsiblePanel::StyleMode mode)
    {
        mStyleMode = mode;
        update();
    }

    QSize sizeHint() const override
    {
        QFontMetrics fm(font());
        int h = fm.height() + 8;
        if (mStyleMode == DACollapsiblePanel::GroupBox) {
            // GroupBox模式下标题在边框线上，头部仅显示箭头
            return QSize(mArrowSize + mMargin * 2, h);
        }
        int textWidth = Qt5Qt6Compat_fontMetrics_width(fm, mTitle) + mArrowSize + 8 + mMargin * 2;
        return QSize(textWidth, h);
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

        // 绘制标题文本（GroupBox模式下标题由父控件在边框线上绘制，此处不绘制）
        if (mStyleMode != DACollapsiblePanel::GroupBox) {
            QFontMetrics fm(font());
            int textX = mArrowSize + 4 + mMargin;
            int textY = (height() - fm.height()) / 2;
            painter.drawText(textX, textY, titleWidth(), fm.height(), Qt::AlignLeft | Qt::AlignVCenter, mTitle);
        }
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
    int mMargin    = 6;
    DACollapsiblePanel::StyleMode mStyleMode = DACollapsiblePanel::Flat;
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
    QWidget* mContentWidget                 = nullptr;
    QVBoxLayout* mContentLayout             = nullptr;
    QVBoxLayout* mMainLayout                = nullptr;

    bool mExpanded = true;
    QString mTitle;
    DACollapsiblePanel::StyleMode mStyleMode = DACollapsiblePanel::Flat;
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
DACollapsiblePanel::DACollapsiblePanel(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    connect(d->mHeaderWidget, &DACollapsiblePanelHeader::clicked, this, &DACollapsiblePanel::onHeaderClicked);
}

/**
 * @brief 构造函数（带标题）
 * @param[in] title 面板标题
 * @param[in] parent 父控件
 */
DACollapsiblePanel::DACollapsiblePanel(const QString& title, QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
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

    // 检查内容控件是否已存在布局（如DAPropertyPanelWidget等自带布局的控件）
    QLayout* existingLayout = d->mContentWidget->layout();
    if (existingLayout) {
        // 已有布局，尝试复用为 QVBoxLayout
        d->mContentLayout = qobject_cast<QVBoxLayout*>(existingLayout);
        if (!d->mContentLayout) {
            // 非 QVBoxLayout 类型，重新创建（安全保护）
            d->mContentLayout = new QVBoxLayout(d->mContentWidget);
            d->mContentLayout->setObjectName(QStringLiteral("contentLayout"));
            d->mContentLayout->setContentsMargins(4, 4, 4, 4);
            d->mContentLayout->setSpacing(4);
        }
        // 若成功复用，保留原布局的边距与间距设置
    } else {
        // 无布局时创建默认 QVBoxLayout
        d->mContentLayout = new QVBoxLayout(d->mContentWidget);
        d->mContentLayout->setObjectName(QStringLiteral("contentLayout"));
        d->mContentLayout->setContentsMargins(4, 4, 4, 4);
        d->mContentLayout->setSpacing(4);
    }

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
 * @brief 设置面板样式模式
 * @param[in] mode 样式模式（Flat/GroupBox/Bordered）
 *
 * 根据样式模式调整边距：Flat模式无边距变化，GroupBox模式内容区域增加内边距，
 * Bordered模式整体增加外边距。
 */
void DACollapsiblePanel::setStyleMode(StyleMode mode)
{
    DA_D(d);
    if (d->mStyleMode == mode) {
        return;
    }
    d->mStyleMode = mode;
    d->mHeaderWidget->setStyleMode(mode);  // 传播样式模式到头部控件
    // 根据样式模式调整边距
    switch (mode) {
    case Flat:
        d->mMainLayout->setContentsMargins(0, 0, 0, 0);
        if (d->mContentLayout) {
            d->mContentLayout->setContentsMargins(4, 4, 4, 4);
        }
        break;
    case GroupBox:
        d->mMainLayout->setContentsMargins(0, 0, 0, 0);
        if (d->mContentLayout) {
            d->mContentLayout->setContentsMargins(8, 8, 8, 8);
        }
        break;
    case Bordered:
        d->mMainLayout->setContentsMargins(2, 2, 2, 2);
        if (d->mContentLayout) {
            d->mContentLayout->setContentsMargins(4, 4, 4, 4);
        }
        break;
    }
    update();       // 触发重绘
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
 * Flat模式使用默认QWidget渲染（保持向后兼容）；GroupBox模式在内容区域周围绘制边框，
 * 标题文本断开顶部边线；Bordered模式在整体面板周围绘制简单矩形边框。
 */
void DACollapsiblePanel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    DA_D(d);

    if (d->mStyleMode == Flat) {
        // Flat模式：无需自定义绘制，使用默认QWidget渲染
        // 保持向后兼容——与当前外观完全一致
        QStyleOption opt;
        opt.initFrom(this);
        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
        return;
    }

    QPainter painter(this);
    painter.setClipRect(event->rect());

    // 使用palette颜色——不使用硬编码颜色值
    QPalette pal = palette();

    if (d->mStyleMode == GroupBox) {
        // GroupBox样式：边框包围内容区域，标题文本断开顶部边线
        QPen borderPen(pal.color(QPalette::Window).lighter(120));
        borderPen.setWidth(1);
        painter.setPen(borderPen);

        // 计算内容区域矩形（头部下方）
        int headerHeight = d->mHeaderWidget ? d->mHeaderWidget->height() : 0;
        QRect contentRect = rect();
        contentRect.setTop(headerHeight);

        // 计算标题文本在顶部边线上的位置和宽度，用于断开边线
        int titleTextWidth = 0;
        int titleStartX   = 0;
        if (d->mHeaderWidget) {
            QFontMetrics fm(d->mHeaderWidget->font());
            int arrowSize = 10;
            int margin    = 6;
            titleStartX   = arrowSize + 4 + margin;
            titleTextWidth = Qt5Qt6Compat_fontMetrics_width(fm, d->mTitle);
            // 标题文本两侧添加间距以形成"断开"效果
            titleTextWidth += 8;   // 左右各4px间距
            titleStartX -= 4;      // 左侧4px间距
        }

        int topY = contentRect.top();

        // 顶部边线——左段（标题左侧）
        painter.drawLine(contentRect.left(), topY, titleStartX, topY);
        // 顶部边线——右段（标题右侧）
        painter.drawLine(titleStartX + titleTextWidth, topY, contentRect.right(), topY);

        // 左、右、底部边线（完整线条）
        painter.drawLine(contentRect.left(), topY, contentRect.left(), contentRect.bottom());
        painter.drawLine(contentRect.right(), topY, contentRect.right(), contentRect.bottom());
        painter.drawLine(contentRect.left(), contentRect.bottom(), contentRect.right(), contentRect.bottom());

        // 在顶部边线断口处绘制标题文本（GroupBox风格）
        if (d->mHeaderWidget && !d->mTitle.isEmpty()) {
            QFont titleFont = d->mHeaderWidget->font();
            painter.setFont(titleFont);
            painter.setPen(pal.color(QPalette::WindowText));
            QFontMetrics fm(titleFont);
            int textY = topY + fm.ascent() / 2 - fm.height() / 2;
            painter.drawText(titleStartX + 4,
                             textY,
                             titleTextWidth - 8,
                             fm.height(),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             d->mTitle);
        }
    } else if (d->mStyleMode == Bordered) {
        // Bordered样式：简单矩形边框包围整个面板
        QPen borderPen(pal.color(QPalette::Window).lighter(120));
        borderPen.setWidth(1);
        painter.setPen(borderPen);
        painter.drawRect(rect().adjusted(0, 0, -1, -1));  // -1确保边框在控件边界内
    }
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
