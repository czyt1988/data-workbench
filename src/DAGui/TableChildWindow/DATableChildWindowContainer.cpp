#include "DATableChildWindowContainer.h"
#include "DATableChildWindow.h"
// Qt
#include <QEvent>
#include <QPoint>
#include <QSize>
#include <QWidget>
#include <utility>

namespace DA
{

namespace
{
// attach 未给首选位置时离允许区域右上角的边距
constexpr int c_defaultMargin = 12;
// attach 未给首选尺寸且内容无有效 sizeHint 时的兜底尺寸
constexpr QSize c_fallbackSize(400, 300);
}  // namespace

/**
 * @brief 构造函数
 *
 * 宿主部件是子窗口的视觉父级（子窗口几何即宿主坐标系）；本容器自身
 * 只做管理，不创建任何覆盖层部件。宿主尺寸变化经事件过滤器触发
 * 全量子窗口夹取。
 * @param host 宿主部件（子窗口的父级）
 * @param parent QObject 父级
 */
DATableChildWindowContainer::DATableChildWindowContainer(QWidget* host, QObject* parent)
    : QObject(parent), mHost(host)
{
    if (mHost) {
        mHost->installEventFilter(this);
    }
}

DATableChildWindowContainer::~DATableChildWindowContainer()
{
}

/**
 * @brief 宿主部件
 */
QWidget* DATableChildWindowContainer::host() const
{
    return mHost.data();
}

/**
 * @brief 设置允许区域提供者
 */
void DATableChildWindowContainer::setBoundsProvider(const BoundsProvider& provider)
{
    mBoundsProvider = provider;
    clampAllWindows();
}

/**
 * @brief 贴入任意部件，变成表格里的子窗口
 *
 * 创建 DATableChildWindow 框架（父级 = 宿主），内容重父化进框架；
 * 首选尺寸缺失时取内容 sizeHint，首选位置缺失时放允许区域右上角；
 * 全部收敛到允许区域内后显示置顶。
 * @param content 内容部件（空返回 nullptr）
 * @param title 标题栏文字
 * @param preferredSize 首选尺寸（无效时取内容 sizeHint/兜底尺寸）
 * @param preferredPos 首选位置（宿主坐标；无效时取允许区域右上角）
 * @return 贴入后的子窗口框架
 */
DATableChildWindow* DATableChildWindowContainer::attach(QWidget* content,
                                                        const QString& title,
                                                        const QSize& preferredSize,
                                                        const QPoint& preferredPos)
{
    if (!mHost || !content) {
        return nullptr;
    }
    DATableChildWindow* w = new DATableChildWindow(mHost);
    // 允许区域提供者经 QPointer 兜底：容器先于框架析构时返回无效矩形 → 框架回退父部件矩形
    QPointer< DATableChildWindowContainer > guard(this);
    w->setBoundsProvider([ guard ]() -> QRect {
        return guard ? guard->boundsRect() : QRect();
    });
    w->setDefaultSize(preferredSize);
    w->setWidget(content);
    w->setTitle(title);
    // 几何：首选尺寸 → 内容 sizeHint → 兜底尺寸，收敛到允许区域
    QSize s = preferredSize.isValid() && !preferredSize.isEmpty()
                  ? preferredSize
                  : (content->sizeHint().isValid() && !content->sizeHint().isEmpty() ? content->sizeHint()
                                                                                    : c_fallbackSize);
    const QRect bounds = boundsRect();
    if (bounds.isValid()) {
        s = s.boundedTo(bounds.size());
    }
    QPoint pos = preferredPos;
    if (bounds.isValid()) {
        if (pos.isNull()) {
            pos = QPoint(qMax(bounds.left(), bounds.right() - s.width() + 1 - c_defaultMargin),
                         qMax(bounds.top(), bounds.top() + c_defaultMargin));
        }
        pos.setX(qBound(bounds.left(), pos.x(), qMax(bounds.left(), bounds.right() - s.width() + 1)));
        pos.setY(qBound(bounds.top(), pos.y(), qMax(bounds.top(), bounds.bottom() - s.height() + 1)));
    }
    w->setGeometry(QRect(pos, s));
    // 关闭默认语义：隐藏（复用型宿主可在返回的框架上另行接线）
    connect(w, &DATableChildWindow::closeRequested, w, &QWidget::hide);
    mWindows.append(w);
    w->show();
    w->raise();
    Q_EMIT windowAttached(w);
    return w;
}

/**
 * @brief 摘除并销毁子窗口（连同内容部件）
 */
void DATableChildWindowContainer::detach(DATableChildWindow* w)
{
    if (!w) {
        return;
    }
    for (int i = 0; i < mWindows.size(); ++i) {
        if (mWindows.at(i).data() == w) {
            mWindows.removeAt(i);
            break;
        }
    }
    Q_EMIT windowDetached(w);
    w->deleteLater();
}

/**
 * @brief 摘除子窗口并归还内容部件
 *
 * @return 内容部件（重父化到 nullptr；无效窗口返回 nullptr）
 */
QWidget* DATableChildWindowContainer::takeAndDetach(DATableChildWindow* w)
{
    if (!w) {
        return nullptr;
    }
    QWidget* content = w->takeWidget();
    detach(w);
    return content;
}

/**
 * @brief 当前全部子窗口（已析构的自动剔除）
 */
QList< DATableChildWindow* > DATableChildWindowContainer::windows() const
{
    QList< DATableChildWindow* > res;
    res.reserve(mWindows.size());
    for (const QPointer< DATableChildWindow >& w : std::as_const(mWindows)) {
        if (w) {
            res.append(w.data());
        }
    }
    return res;
}

/**
 * @brief 内容部件对应的子窗口
 */
DATableChildWindow* DATableChildWindowContainer::windowFor(const QWidget* content) const
{
    if (!content) {
        return nullptr;
    }
    for (const QPointer< DATableChildWindow >& w : std::as_const(mWindows)) {
        if (w && w->widget() == content) {
            return w.data();
        }
    }
    return nullptr;
}

/**
 * @brief 监听几何源
 *
 * 主视口/窗格容器等的 Move/Resize/Show/Hide（滚动条显隐、冻结分栏位移、
 * 页面缩放都会触发）→ 全部子窗口夹回允许区域。重复监听安全。
 * @param w 几何源部件
 */
void DATableChildWindowContainer::watchGeometrySource(QWidget* w)
{
    if (!w) {
        return;
    }
    for (const QPointer< QWidget >& p : std::as_const(mWatched)) {
        if (p.data() == w) {
            return;  // 防重复
        }
    }
    mWatched.append(w);
    w->installEventFilter(this);
}

/**
 * @brief 立即把全部子窗口夹回允许区域
 */
void DATableChildWindowContainer::clampAllWindows()
{
    const QList< DATableChildWindow* > all = windows();
    for (DATableChildWindow* w : all) {
        w->clampIntoBounds();
    }
}

/**
 * @brief 几何源/宿主的尺寸与显隐变化 → 全部子窗口夹回允许区域
 */
bool DATableChildWindowContainer::eventFilter(QObject* watched, QEvent* event)
{
    if (event) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
        case QEvent::Show:
        case QEvent::Hide:
            clampAllWindows();
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(watched, event);
}

/**
 * @brief 允许区域
 */
QRect DATableChildWindowContainer::boundsRect() const
{
    if (mBoundsProvider) {
        const QRect r = mBoundsProvider();
        if (r.isValid()) {
            return r;
        }
    }
    if (mHost) {
        return QRect(QPoint(0, 0), mHost->size());
    }
    return QRect();
}

}  // namespace DA
