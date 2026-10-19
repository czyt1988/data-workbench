#include "DAVirtualScrollBar.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QToolTip>

namespace DA
{
// 按住箭头/轨道的连续滚动：首次延迟与重复间隔
static constexpr int kRepeatFirstDelayMs = 400;
static constexpr int kRepeatIntervalMs   = 60;

DAVirtualScrollBar::DAVirtualScrollBar(QWidget* parent) : QScrollBar(Qt::Vertical, parent)
{
    setFocusPolicy(Qt::StrongFocus);
    mRepeatTimer = new QTimer(this);
    mRepeatTimer->setInterval(kRepeatIntervalMs);
    connect(mRepeatTimer, &QTimer::timeout, this, &DAVirtualScrollBar::onRepeatTimeout);
}

DAVirtualScrollBar::~DAVirtualScrollBar()
{
}

/**
 * @brief 同步整表坐标信息
 * @param firstVisibleActualRow 视口首行的绝对行号
 * @param totalRows 数据总行数
 * @param viewportRows 一屏可见行数
 */
void DAVirtualScrollBar::setDatasetPosition(int firstVisibleActualRow, int totalRows, int viewportRows)
{
    const int total = qMax(0, totalRows);
    const int vp    = qMax(1, viewportRows);
    const int first = qBound(0, firstVisibleActualRow, qMax(0, total - 1));
    if (first == mFirstVisibleRow && total == mTotalRows && vp == mViewportRows) {
        return;
    }
    mFirstVisibleRow = first;
    mTotalRows       = total;
    mViewportRows    = vp;
    update();
}

int DAVirtualScrollBar::getDatasetFirstVisibleRow() const
{
    return mFirstVisibleRow;
}

int DAVirtualScrollBar::getDatasetTotalRows() const
{
    return mTotalRows;
}

int DAVirtualScrollBar::getDatasetViewportRows() const
{
    return mViewportRows;
}

bool DAVirtualScrollBar::isSeeking() const
{
    return mSeeking;
}

/**
 * @brief 按整表坐标构造样式选项
 *
 * 用「最大值=总行数-1、页步=一屏行数、滑块位置=视口首行」的假坐标喂给样式，
 * 让原生样式算出诚实的滑块长度与位置。
 * @return 样式选项
 */
QStyleOptionSlider DAVirtualScrollBar::datasetStyleOption() const
{
    QStyleOptionSlider opt;
    opt.initFrom(this);
    opt.orientation   = Qt::Vertical;
    opt.minimum       = 0;
    opt.maximum       = qMax(0, mTotalRows - 1);
    opt.pageStep      = qMax(1, mViewportRows);
    opt.singleStep    = 1;
    opt.sliderPosition = qBound(opt.minimum, mFirstVisibleRow, opt.maximum);
    opt.sliderValue    = opt.sliderPosition;
    opt.upsideDown     = invertedAppearance();
    opt.subControls    = QStyle::SC_All;
    opt.activeSubControls = mHoverSub;
    if (mSeeking) {
        opt.state |= QStyle::State_Sunken;
        opt.activeSubControls = QStyle::SC_ScrollBarSlider;
    }
    if (isEnabled() && underMouse()) {
        opt.state |= QStyle::State_MouseOver;
    }
    // 与 QScrollBar::initStyleOption 保持一致：透明（overlay）风格下需要 State_On 才绘制
    if (style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this)) {
        opt.state |= QStyle::State_On;
    }
    return opt;
}

/**
 * @brief 轨道矩形（取不到时退回整个控件矩形）
 * @param opt 样式选项
 * @return 轨道矩形
 */
QRect DAVirtualScrollBar::grooveRect(const QStyleOptionSlider& opt) const
{
    QRect gr = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, this);
    if (!gr.isValid() || gr.isEmpty()) {
        return rect();
    }
    return gr;
}

void DAVirtualScrollBar::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    QStyleOptionSlider opt = datasetStyleOption();
    style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p, this);
}

/**
 * @brief 控件内位置换算为整数行号
 *
 * 与 QScrollBarPrivate::pixelPosToRangeValue() 同一套算法，只是把坐标系换成整表。
 * @param pos 控件内 y 坐标
 * @return 该位置对应的绝对行号
 */
int DAVirtualScrollBar::actualRowForPosition(int pos) const
{
    const QStyleOptionSlider opt = datasetStyleOption();
    if (opt.maximum <= opt.minimum) {
        return 0;
    }
    const QRect gr = grooveRect(opt);
    const QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
    const int sliderLength = qMax(0, sr.height());
    const int sliderMin    = gr.top();
    const int sliderMax    = gr.bottom() - sliderLength + 1;
    if (sliderMax <= sliderMin) {
        return opt.minimum;
    }
    return QStyle::sliderValueFromPosition(opt.minimum,
                                           opt.maximum,
                                           pos - sliderMin,
                                           sliderMax - sliderMin,
                                           opt.upsideDown);
}

/**
 * @brief 整数行号换算为滑块顶边位置
 * @param actualRow 绝对行号
 * @return 控件内 y 坐标（与 actualRowForPosition 互为逆运算）
 */
int DAVirtualScrollBar::positionForActualRow(int actualRow) const
{
    const QStyleOptionSlider opt = datasetStyleOption();
    const QRect gr               = grooveRect(opt);
    const QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
    const int sliderLength = qMax(0, sr.height());
    const int span         = qMax(0, gr.bottom() - sliderLength + 1 - gr.top());
    const int offset       = QStyle::sliderPositionFromValue(opt.minimum, opt.maximum, actualRow, span, opt.upsideDown);
    return gr.top() + offset;
}

/**
 * @brief 更新鼠标悬停的子控件（仅影响绘制，不改变任何状态）
 * @param pos 控件内坐标
 */
void DAVirtualScrollBar::updateHoverSubControl(const QPoint& pos)
{
    const QStyleOptionSlider opt = datasetStyleOption();
    const QStyle::SubControl sc  = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, pos, this);
    if (sc != mHoverSub) {
        mHoverSub = sc;
        update();
    }
}

/**
 * @brief 拖动中的位置提示
 * @param pos 控件内坐标
 */
void DAVirtualScrollBar::showDragHud(const QPoint& pos)
{
    const QLocale locale;
    const QString text = tr("Row %1 of %2")
                             .arg(locale.toString(static_cast< qlonglong >(mFirstVisibleRow) + 1),
                                  locale.toString(static_cast< qlonglong >(mTotalRows)));  // cn:第 %1 行 / 共 %2 行
    QToolTip::showText(mapToGlobal(QPoint(width(), pos.y())), text, this, QRect(), 3000);
}

void DAVirtualScrollBar::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton || mTotalRows <= 0) {
        e->ignore();
        return;
    }
    const QStyleOptionSlider opt = datasetStyleOption();
    const QStyle::SubControl sc  = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, e->pos(), this);
    switch (sc) {
    case QStyle::SC_ScrollBarSlider: {
        // 记录抓取点相对滑块顶边的偏移，拖动时保持抓取点吸附，避免滑块「跳一下」
        const QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
        mDragClickOffset = e->pos().y() - sr.top();
        mSeeking         = true;
        update();
        Q_EMIT seekRequested(actualRowForPosition(e->pos().y() - mDragClickOffset));
        showDragHud(e->pos());
        e->accept();
        break;
    }
    case QStyle::SC_ScrollBarAddLine:
        Q_EMIT stepRowsRequested(1);
        startRepeatTimer(sc);
        e->accept();
        break;
    case QStyle::SC_ScrollBarSubLine:
        Q_EMIT stepRowsRequested(-1);
        startRepeatTimer(sc);
        e->accept();
        break;
    case QStyle::SC_ScrollBarAddPage:
        Q_EMIT pageRowsRequested(1);
        startRepeatTimer(sc);
        e->accept();
        break;
    case QStyle::SC_ScrollBarSubPage:
        Q_EMIT pageRowsRequested(-1);
        startRepeatTimer(sc);
        e->accept();
        break;
    default:
        e->ignore();
        break;
    }
}

void DAVirtualScrollBar::mouseMoveEvent(QMouseEvent* e)
{
    if (mSeeking) {
        Q_EMIT seekRequested(actualRowForPosition(e->pos().y() - mDragClickOffset));
        showDragHud(e->pos());
        e->accept();
        return;
    }
    updateHoverSubControl(e->pos());
    e->accept();
}

void DAVirtualScrollBar::mouseReleaseEvent(QMouseEvent* e)
{
    stopRepeatTimer();
    if (mSeeking) {
        mSeeking = false;
        QToolTip::hideText();
        update();
        Q_EMIT seekFinished();
    }
    e->accept();
}

void DAVirtualScrollBar::leaveEvent(QEvent* e)
{
    Q_UNUSED(e);
    if (mHoverSub != QStyle::SC_None) {
        mHoverSub = QStyle::SC_None;
        update();
    }
    QScrollBar::leaveEvent(e);
}

/**
 * @brief 滚动条获得焦点时的键位：按整表坐标处理 Home/End/翻页/单步
 * @param e 键盘事件
 */
void DAVirtualScrollBar::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_Up:
        Q_EMIT stepRowsRequested(-1);
        e->accept();
        return;
    case Qt::Key_Down:
        Q_EMIT stepRowsRequested(1);
        e->accept();
        return;
    case Qt::Key_PageUp:
        Q_EMIT pageRowsRequested(-1);
        e->accept();
        return;
    case Qt::Key_PageDown:
        Q_EMIT pageRowsRequested(1);
        e->accept();
        return;
    case Qt::Key_Home:
        Q_EMIT seekRequested(0);
        e->accept();
        return;
    case Qt::Key_End:
        Q_EMIT seekRequested(qMax(0, mTotalRows - 1));
        e->accept();
        return;
    default:
        break;
    }
    QScrollBar::keyPressEvent(e);
}

void DAVirtualScrollBar::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu menu(this);
    QAction* actGoto = menu.addAction(tr("Go To Row...")  // cn:定位到行...
    );
    menu.addSeparator();
    QAction* actFirst = menu.addAction(tr("First Row")  // cn:首行
    );
    QAction* actLast = menu.addAction(tr("Last Row")  // cn:尾行
    );
    QAction* picked = menu.exec(e->globalPos());
    if (picked == actGoto) {
        Q_EMIT gotoRowRequested();
    } else if (picked == actFirst) {
        Q_EMIT seekRequested(0);
    } else if (picked == actLast) {
        Q_EMIT seekRequested(qMax(0, mTotalRows - 1));
    }
    e->accept();
}

void DAVirtualScrollBar::startRepeatTimer(QStyle::SubControl sc)
{
    mRepeatSub   = sc;
    mRepeatTicks = 0;
    mRepeatTimer->start(kRepeatFirstDelayMs);
}

void DAVirtualScrollBar::stopRepeatTimer()
{
    if (mRepeatTimer->isActive()) {
        mRepeatTimer->stop();
    }
    mRepeatSub   = QStyle::SC_None;
    mRepeatTicks = 0;
}

/**
 * @brief 按住箭头/轨道时的连续滚动
 */
void DAVirtualScrollBar::onRepeatTimeout()
{
    ++mRepeatTicks;
    if (mRepeatTicks == 1) {
        mRepeatTimer->setInterval(kRepeatIntervalMs);
    } else if (mRepeatTicks > 1) {
        // 滚动条已经滚到数据集两端时不必继续空转
        const bool needMore = (mRepeatSub == QStyle::SC_ScrollBarAddLine || mRepeatSub == QStyle::SC_ScrollBarAddPage)
                                  ? (mFirstVisibleRow < mTotalRows - 1)
                                  : (mFirstVisibleRow > 0);
        if (!needMore) {
            stopRepeatTimer();
            return;
        }
    }
    switch (mRepeatSub) {
    case QStyle::SC_ScrollBarAddLine:
        Q_EMIT stepRowsRequested(1);
        break;
    case QStyle::SC_ScrollBarSubLine:
        Q_EMIT stepRowsRequested(-1);
        break;
    case QStyle::SC_ScrollBarAddPage:
        Q_EMIT pageRowsRequested(1);
        break;
    case QStyle::SC_ScrollBarSubPage:
        Q_EMIT pageRowsRequested(-1);
        break;
    default:
        stopRepeatTimer();
        break;
    }
}
}  // end DA
