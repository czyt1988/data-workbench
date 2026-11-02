#include "DATableChildWindow.h"
// Qt
#include <QAction>
#include <QContextMenuEvent>
#include <QCursor>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QToolButton>
#include <QVBoxLayout>
#include <utility>

namespace DA
{

namespace
{
// 框架边距（1px 边框到内容的留白，同时是缩放手柄的抓取带区域）
constexpr int c_frameMargin = 6;
// 标题栏高度
constexpr int c_titleBarHeight = 24;
// 4 角抓取块边长
constexpr int c_cornerSize = 10;
// 4 边拖拽带厚度
constexpr int c_edgeThickness = 6;
// 默认几何离允许区域边缘的边距
constexpr int c_defaultMargin = 12;
// attach 未给首选尺寸时的兜底默认尺寸
constexpr QSize c_fallbackSize(400, 300);
// 关闭按钮尺寸
constexpr QSize c_closeButtonSize(20, 18);
// 标题文字左右留白
constexpr int c_titlePadding = 6;
// 中灰（DESIGN.md 普通边框/分隔线色）
constexpr QColor c_borderColor(0x72, 0x72, 0x72);
// 深蓝灰（DESIGN.md 抓取块描边，深浅主题下均可辨识）
constexpr QColor c_handleBorderColor(0x5D, 0x6D, 0x7E);

// 鼠标事件的全局坐标（Qt5/Qt6 兼容，与 DADataHeaderView 同法）
QPoint globalPosOf(const QMouseEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->globalPos();
#else
    return event->globalPosition().toPoint();
#endif
}
}  // namespace

// ============================================================
// DATableChildWindow::TitleBar
// ============================================================

/**
 * @brief 标题栏
 *
 * 自绘标题（加粗、超宽中段省略）与底部 1px 分隔线；承载关闭按钮。
 * 拖动/双击/右键菜单都转发给框架（DATableChildWindow）处理，
 * 文案统一走 DATableChildWindow 的 tr 上下文（本类无 Q_OBJECT）。
 */
class DATableChildWindow::TitleBar : public QWidget
{
public:
    explicit TitleBar(DATableChildWindow* frame)
        : QWidget(frame), mFrame(frame)
    {
        setAutoFillBackground(true);
        setBackgroundRole(QPalette::Button);
        setFixedHeight(c_titleBarHeight);
        setCursor(Qt::SizeAllCursor);
        setToolTip(DATableChildWindow::tr("Drag to move"));  // cn:拖动移动
        mCloseButton = new QToolButton(this);
        mCloseButton->setText(QStringLiteral("\u2715"));
        mCloseButton->setAutoRaise(true);
        mCloseButton->setFixedSize(c_closeButtonSize);
        mCloseButton->setToolTip(DATableChildWindow::tr("Close"));  // cn:关闭
        connect(mCloseButton, &QToolButton::clicked, mFrame, &DATableChildWindow::closeRequested);
    }

    // 关闭按钮右侧位置（绘制标题可用宽度用）
    int closeButtonLeft() const
    {
        return mCloseButton ? mCloseButton->geometry().left() : width();
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        if (mCloseButton) {
            mCloseButton->move(width() - mCloseButton->width() - 2,
                               (height() - mCloseButton->height()) / 2);
        }
        update();
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        // 标题：加粗 + 中段省略（快速绘图常带长列名清单）
        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        p.setPen(palette().color(QPalette::WindowText));
        const QRect textRect(c_titlePadding, 0,
                             qMax(0, closeButtonLeft() - 2 * c_titlePadding), height());
        const QString text = p.fontMetrics().elidedText(mFrame->title(), Qt::ElideMiddle, textRect.width());
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
        // 底部 1px 分隔线（与框架边框同色）
        p.setPen(QPen(c_borderColor, 1));
        p.drawLine(0, height() - 1, width(), height() - 1);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && mFrame) {
            mFrame->raise();
            if (mFrame->isMaximized()) {
                // 最大化状态下拖动：先还原，并让光标落在标题栏同一比例位置（Excel 同款语义）
                const QPoint global = globalPosOf(event);
                const double ratio = double(global.x() - mFrame->geometry().left())
                                     / qMax(1, mFrame->width());
                mFrame->setMaximized(false);
                mFrame->move(global.x() - qRound(ratio * mFrame->width()), global.y() - height() / 2);
                mFrame->clampIntoBounds();
            }
            mDragging      = true;
            mPressGlobal   = globalPosOf(event);
            mPressWindowPos = mFrame->pos();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (mDragging && mFrame) {
            const QPoint target = mPressWindowPos + globalPosOf(event) - mPressGlobal;
            const QRect bounds  = mFrame->boundsRect();
            if (bounds.isValid()) {
                // 只夹位置不改尺寸：拖动窗口永不越界（也永不压住滚动条）
                const int x = qBound(bounds.left(), target.x(),
                                     qMax(bounds.left(), bounds.right() - mFrame->width() + 1));
                const int y = qBound(bounds.top(), target.y(),
                                     qMax(bounds.top(), bounds.bottom() - mFrame->height() + 1));
                mFrame->move(x, y);
            } else {
                mFrame->move(target);
            }
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (mDragging && event->button() == Qt::LeftButton) {
            mDragging = false;
            if (mFrame) {
                Q_EMIT mFrame->geometryUserChanged();
            }
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && mFrame) {
            mFrame->toggleMaximized();
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent* event) override
    {
        if (mFrame) {
            mFrame->showTitleContextMenu(event->globalPos());
            event->accept();
            return;
        }
        QWidget::contextMenuEvent(event);
    }

private:
    DATableChildWindow* mFrame;  ///< 非拥有（框架本体）
    QToolButton* mCloseButton { nullptr };
    bool mDragging { false };        ///< 标题栏拖动进行中
    QPoint mPressGlobal;             ///< 拖动起始全局坐标
    QPoint mPressWindowPos;          ///< 拖动起始框架位置（宿主坐标）
};

// ============================================================
// DATableChildWindow::ResizeHandle
// ============================================================

/**
 * @brief 缩放手柄
 *
 * 4 角抓取块（自绘白底深边小方块，光标对角缩放）+ 4 边拖拽带（不自绘，
 * 光标横/纵缩放）。拖动时被拖的边跟随光标，对侧边锚定；逐段夹取最小
 * 尺寸与允许区域。
 */
class DATableChildWindow::ResizeHandle : public QWidget
{
public:
    explicit ResizeHandle(DATableChildWindow* frame, Qt::Edges edges)
        : QWidget(frame), mFrame(frame), mEdges(edges)
    {
        setCursor(cursorFor(mEdges));
        setMouseTracking(false);
    }

    // 按框架当前尺寸重算自身几何（框架 resizeEvent 调用）
    void applyGeometry(int w, int h)
    {
        const int c = c_cornerSize;
        const int e = c_edgeThickness;
        if (mEdges == (Qt::TopEdge | Qt::LeftEdge)) {
            setGeometry(0, 0, c, c);
        } else if (mEdges == (Qt::TopEdge | Qt::RightEdge)) {
            setGeometry(w - c, 0, c, c);
        } else if (mEdges == (Qt::BottomEdge | Qt::LeftEdge)) {
            setGeometry(0, h - c, c, c);
        } else if (mEdges == (Qt::BottomEdge | Qt::RightEdge)) {
            setGeometry(w - c, h - c, c, c);
        } else if (mEdges == Qt::TopEdge) {
            setGeometry(c, 0, qMax(0, w - 2 * c), e);
        } else if (mEdges == Qt::BottomEdge) {
            setGeometry(c, h - e, qMax(0, w - 2 * c), e);
        } else if (mEdges == Qt::LeftEdge) {
            setGeometry(0, c, e, qMax(0, h - 2 * c));
        } else if (mEdges == Qt::RightEdge) {
            setGeometry(w - e, c, e, qMax(0, h - 2 * c));
        }
        raise();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        // 只有角抓取块绘制；边拖拽带只提供光标提示
        const bool isCorner = (mEdges == (Qt::TopEdge | Qt::LeftEdge) || mEdges == (Qt::TopEdge | Qt::RightEdge)
                               || mEdges == (Qt::BottomEdge | Qt::LeftEdge)
                               || mEdges == (Qt::BottomEdge | Qt::RightEdge));
        if (!isCorner) {
            return;
        }
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(QPen(c_handleBorderColor, 1));
        p.setBrush(palette().color(QPalette::Base));
        p.drawRect(QRect(1, 1, width() - 3, height() - 3));
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && mFrame && !mFrame->isMaximized()) {
            mFrame->raise();
            mDragging     = true;
            mPressGlobal  = globalPosOf(event);
            mStartGeometry = mFrame->geometry();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (mDragging && mFrame) {
            applyDrag(globalPosOf(event) - mPressGlobal);
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (mDragging && event->button() == Qt::LeftButton) {
            mDragging = false;
            if (mFrame) {
                Q_EMIT mFrame->geometryUserChanged();
            }
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

private:
    // 光标形状按边方向
    static Qt::CursorShape cursorFor(Qt::Edges edges)
    {
        if (edges == (Qt::TopEdge | Qt::LeftEdge) || edges == (Qt::BottomEdge | Qt::RightEdge)) {
            return Qt::SizeFDiagCursor;
        }
        if (edges == (Qt::TopEdge | Qt::RightEdge) || edges == (Qt::BottomEdge | Qt::LeftEdge)) {
            return Qt::SizeBDiagCursor;
        }
        if (edges == Qt::LeftEdge || edges == Qt::RightEdge) {
            return Qt::SizeHorCursor;
        }
        return Qt::SizeVerCursor;
    }

    // 按最小尺寸约束夹取目标几何（拖动边固定于对侧锚定边）
    void enforceMinimum(QRect& r) const
    {
        if (!mFrame) {
            return;
        }
        const QSize minS = mFrame->effectiveMinimumSize();
        if (r.width() < minS.width()) {
            if (mEdges & Qt::LeftEdge) {
                r.setLeft(r.right() - minS.width() + 1);
            } else {
                r.setRight(r.left() + minS.width() - 1);
            }
        }
        if (r.height() < minS.height()) {
            if (mEdges & Qt::TopEdge) {
                r.setTop(r.bottom() - minS.height() + 1);
            } else {
                r.setBottom(r.top() + minS.height() - 1);
            }
        }
    }

    // 执行一次拖动位移：目标边跟随光标，逐段夹最小尺寸与允许区域
    void applyDrag(const QPoint& delta)
    {
        QRect r = mStartGeometry;
        if (mEdges & Qt::LeftEdge) {
            r.setLeft(mStartGeometry.left() + delta.x());
        }
        if (mEdges & Qt::TopEdge) {
            r.setTop(mStartGeometry.top() + delta.y());
        }
        if (mEdges & Qt::RightEdge) {
            r.setRight(mStartGeometry.right() + delta.x());
        }
        if (mEdges & Qt::BottomEdge) {
            r.setBottom(mStartGeometry.bottom() + delta.y());
        }
        // 1) 最小尺寸（缩放不把内容压碎）
        enforceMinimum(r);
        // 2) 允许区域（被拖的边不能越过界 → 缩放永不压住滚动条）
        const QRect bounds = mFrame->boundsRect();
        if (bounds.isValid()) {
            if (mEdges & Qt::LeftEdge) {
                r.setLeft(qMax(bounds.left(), r.left()));
            }
            if (mEdges & Qt::TopEdge) {
                r.setTop(qMax(bounds.top(), r.top()));
            }
            if (mEdges & Qt::RightEdge) {
                r.setRight(qMin(bounds.right(), r.right()));
            }
            if (mEdges & Qt::BottomEdge) {
                r.setBottom(qMin(bounds.bottom(), r.bottom()));
            }
            // 3) 区域过窄/过矮时再夹一次最小尺寸（区域比最小尺寸还小时允许溢出，锚定边贴界）
            enforceMinimum(r);
            if (r.left() < bounds.left()) {
                r.moveLeft(bounds.left());
            }
            if (r.top() < bounds.top()) {
                r.moveTop(bounds.top());
            }
        }
        if (r != mFrame->geometry()) {
            mFrame->setGeometry(r);
        }
    }

private:
    DATableChildWindow* mFrame;  ///< 非拥有（框架本体）
    Qt::Edges mEdges;
    bool mDragging { false };   ///< 手柄拖动进行中
    QPoint mPressGlobal;         ///< 拖动起始全局坐标
    QRect mStartGeometry;        ///< 拖动起始框架几何（宿主坐标）
};

// ============================================================
// DATableChildWindow
// ============================================================

/**
 * @brief 构造函数
 *
 * 竖排布局 [标题栏][内容]，1px 中灰边框自绘；8 个缩放手柄按
 * resizeEvent 布位并保持在内容之上。
 * @param parent 宿主部件（子窗口几何即宿主坐标系，不进任何布局）
 */
DATableChildWindow::DATableChildWindow(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("DATableChildWindow"));
    setAutoFillBackground(true);
    setFocusPolicy(Qt::ClickFocus);
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(c_frameMargin, c_frameMargin, c_frameMargin, c_frameMargin);
    root->setSpacing(2);
    mTitleBar = new TitleBar(this);
    root->addWidget(mTitleBar);
    // 缩放手柄：4 角（可见抓取块）+ 4 边（光标拖拽带）
    mHandles.reserve(8);
    mHandles.append(new ResizeHandle(this, Qt::TopEdge | Qt::LeftEdge));
    mHandles.append(new ResizeHandle(this, Qt::TopEdge | Qt::RightEdge));
    mHandles.append(new ResizeHandle(this, Qt::BottomEdge | Qt::LeftEdge));
    mHandles.append(new ResizeHandle(this, Qt::BottomEdge | Qt::RightEdge));
    mHandles.append(new ResizeHandle(this, Qt::TopEdge));
    mHandles.append(new ResizeHandle(this, Qt::BottomEdge));
    mHandles.append(new ResizeHandle(this, Qt::LeftEdge));
    mHandles.append(new ResizeHandle(this, Qt::RightEdge));
    hide();  // 贴入（container attach）时再定位显示
}

DATableChildWindow::~DATableChildWindow()
{
}

/**
 * @brief 贴入内容部件
 *
 * 重父化进框架布局（占据标题栏以下全部空间）并显示；已贴入的内容先
 * 归还调用方（takeWidget，不销毁）。内容上的鼠标按下会把框架置顶。
 * @param w 内容部件（空则忽略）
 */
void DATableChildWindow::setWidget(QWidget* w)
{
    if (!w || w == mContent) {
        return;
    }
    takeWidget();  // 旧内容归还调用方，不静默销毁
    mContent = w;
    w->setParent(this);
    w->removeEventFilter(this);
    w->installEventFilter(this);
    if (QVBoxLayout* lay = qobject_cast< QVBoxLayout* >(layout())) {
        lay->addWidget(w, 1);
    }
    w->show();
    updateHandleGeometries();
}

/**
 * @brief 当前内容部件
 */
QWidget* DATableChildWindow::widget() const
{
    return mContent.data();
}

/**
 * @brief 取出内容部件（所有权归还调用方）
 */
QWidget* DATableChildWindow::takeWidget()
{
    QWidget* w = mContent.data();
    if (!w) {
        return nullptr;
    }
    w->removeEventFilter(this);
    w->setParent(nullptr);
    mContent = nullptr;
    return w;
}

/**
 * @brief 标题栏文字
 */
void DATableChildWindow::setTitle(const QString& title)
{
    if (mTitle == title) {
        return;
    }
    mTitle = title;
    if (mTitleBar) {
        mTitleBar->update();
    }
}

/**
 * @brief 标题栏文字
 */
QString DATableChildWindow::title() const
{
    return mTitle;
}

/**
 * @brief 设置允许区域提供者
 */
void DATableChildWindow::setBoundsProvider(const BoundsProvider& provider)
{
    mBoundsProvider = provider;
}

/**
 * @brief 最大化状态
 *
 * 覆写 QWidget::isMaximized（子控件浮层没有系统最大化态，用「充满允许区域」
 * 的等价语义：标题栏双击切换、最大化期间隐藏缩放手柄）。
 */
bool DATableChildWindow::isMaximized() const
{
    return mMaximized;
}

/**
 * @brief 最大化（充满允许区域）/还原
 */
void DATableChildWindow::setMaximized(bool on)
{
    if (on == mMaximized) {
        return;
    }
    const QRect bounds = boundsRect();
    if (on) {
        mRestoreGeometry = geometry();  // 记住正常几何，还原时恢复
        mMaximized        = true;
        if (bounds.isValid() && geometry() != bounds) {
            setGeometry(bounds);
        }
    } else {
        mMaximized = false;
        if (mRestoreGeometry.isValid()) {
            setGeometry(mRestoreGeometry);
            clampIntoBounds();
        }
    }
    setHandlesVisible(!mMaximized);
    Q_EMIT maximizedChanged(mMaximized);
}

/**
 * @brief 切换最大化/还原（标题栏双击、右键菜单共用）
 */
void DATableChildWindow::toggleMaximized()
{
    setMaximized(!mMaximized);
}

/**
 * @brief 回到默认几何
 *
 * 默认尺寸（setDefaultSize，未设置取当前尺寸，再兜底 c_fallbackSize）
 * 收敛到允许区域与最小尺寸内，放到允许区域右上角。
 */
void DATableChildWindow::resetToDefaultGeometry()
{
    const QRect bounds = boundsRect();
    QSize s = mDefaultSize.isValid() ? mDefaultSize : size();
    if (s.isEmpty() || !s.isValid()) {
        s = c_fallbackSize;
    }
    if (bounds.isValid()) {
        s = s.boundedTo(bounds.size());
    }
    s = s.expandedTo(effectiveMinimumSize());
    QPoint pos(0, 0);
    if (bounds.isValid()) {
        pos = QPoint(qMax(bounds.left(), bounds.right() - s.width() + 1 - c_defaultMargin),
                     qMax(bounds.top(), bounds.top() + c_defaultMargin));
    }
    if (mMaximized) {
        mMaximized = false;
        setHandlesVisible(true);
        Q_EMIT maximizedChanged(false);
    }
    setGeometry(QRect(pos, s));
    clampIntoBounds();
    Q_EMIT geometryUserChanged();
}

/**
 * @brief 把窗口完整夹回允许区域
 *
 * 尺寸超出区域时先收敛到区域（但不小于最小尺寸），再夹位置；
 * 区域比最小尺寸还小时只夹位置（左上对齐）。
 */
void DATableChildWindow::clampIntoBounds()
{
    const QRect bounds = boundsRect();
    if (!bounds.isValid()) {
        return;
    }
    if (mMaximized) {
        if (geometry() != bounds) {
            setGeometry(bounds);
        }
        return;
    }
    const QSize minS = effectiveMinimumSize();
    QRect r = geometry();
    if (r.width() > qMax(minS.width(), bounds.width())) {
        r.setWidth(qMax(minS.width(), bounds.width()));
    }
    if (r.height() > qMax(minS.height(), bounds.height())) {
        r.setHeight(qMax(minS.height(), bounds.height()));
    }
    const int x = qBound(bounds.left(), r.left(), qMax(bounds.left(), bounds.right() - r.width() + 1));
    const int y = qBound(bounds.top(), r.top(), qMax(bounds.top(), bounds.bottom() - r.height() + 1));
    r.moveTo(x, y);
    if (r != geometry()) {
        setGeometry(r);
    }
}

/**
 * @brief 记忆默认尺寸
 */
void DATableChildWindow::setDefaultSize(const QSize& size)
{
    mDefaultSize = size;
}

/**
 * @brief 允许区域
 *
 * 提供者缺失或返回无效矩形时回退 parentWidget()->rect()。
 */
QRect DATableChildWindow::boundsRect() const
{
    if (mBoundsProvider) {
        const QRect r = mBoundsProvider();
        if (r.isValid()) {
            return r;
        }
    }
    if (const QWidget* p = parentWidget()) {
        return QRect(QPoint(0, 0), p->size());
    }
    return QRect();
}

/**
 * @brief 有效最小尺寸
 *
 * 显式 minimumSize（>0 的轴）与布局 minimumSizeHint 逐轴取大；
 * 直接 setGeometry 不经布局约束，缩放路径要显式用它夹取。
 */
QSize DATableChildWindow::effectiveMinimumSize() const
{
    const QSize ms  = minimumSize();
    const QSize msh = minimumSizeHint().isValid() ? minimumSizeHint() : QSize(0, 0);
    return QSize(qMax(ms.width() > 0 ? ms.width() : 0, msh.width()),
                 qMax(ms.height() > 0 ? ms.height() : 0, msh.height()));
}

/**
 * @brief 重排缩放手柄并置顶
 */
void DATableChildWindow::updateHandleGeometries()
{
    const int w = width();
    const int h = height();
    for (ResizeHandle* handle : std::as_const(mHandles)) {
        handle->applyGeometry(w, h);
    }
}

/**
 * @brief 缩放手柄可见性
 */
void DATableChildWindow::setHandlesVisible(bool on)
{
    for (ResizeHandle* handle : std::as_const(mHandles)) {
        handle->setVisible(on);
    }
}

/**
 * @brief 标题栏右键菜单
 */
void DATableChildWindow::showTitleContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    QAction* maximizeAction = menu.addAction(mMaximized ? tr("Restore")  // cn:还原
                                                        : tr("Maximize"));  // cn:最大化
    menu.addSeparator();
    QAction* resetAction = menu.addAction(tr("Reset position and size"));  // cn:重置位置与大小
    menu.addSeparator();
    QAction* closeAction = menu.addAction(tr("Close"));  // cn:关闭
    QAction* chosen       = menu.exec(globalPos);
    if (chosen == maximizeAction) {
        toggleMaximized();
    } else if (chosen == resetAction) {
        resetToDefaultGeometry();
    } else if (chosen == closeAction) {
        Q_EMIT closeRequested();
    }
}

/**
 * @brief 尺寸变化时重排缩放手柄
 */
void DATableChildWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateHandleGeometries();
}

/**
 * @brief 自绘 1px 中灰边框（子控件浮层没有系统边框）
 */
void DATableChildWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(c_borderColor, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRect(0, 0, width() - 1, height() - 1));
}

/**
 * @brief Esc 请求关闭（焦点在框架/标题栏上时；内容部件自己的 Esc 由内容处理）
 */
void DATableChildWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        Q_EMIT closeRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

/**
 * @brief 框架本体（边距区域）按下时置顶
 */
bool DATableChildWindow::event(QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        raise();
    }
    return QWidget::event(event);
}

/**
 * @brief 内容部件按下鼠标时把框架置顶
 */
bool DATableChildWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == mContent.data() && event->type() == QEvent::MouseButtonPress) {
        raise();
    }
    return QWidget::eventFilter(watched, event);
}

}  // namespace DA
