#include <QtTest/QtTest>
#include <QApplication>
#include <QLabel>
#include <QPointer>
#include <QToolButton>
#include <QMouseEvent>
#include <memory>

#include "TableChildWindow/DATableChildWindow.h"
#include "TableChildWindow/DATableChildWindowContainer.h"

namespace
{
// 向部件投递鼠标事件（本地坐标与全局坐标保持一致，模拟真实拖动）
void sendMouse(QWidget* target, QEvent::Type type, const QPoint& local,
               Qt::MouseButton button = Qt::LeftButton)
{
    QMouseEvent ev(type, local, target->mapToGlobal(local), button,
                  type == QEvent::MouseButtonRelease ? Qt::NoButton : button,
                  Qt::NoModifier);
    QApplication::sendEvent(target, &ev);
}

// 找标题栏（无公开访问器的私有嵌套类，按布局位置取：标题栏占框架顶部）
QWidget* titleBarOf(DA::DATableChildWindow* frame)
{
    return frame->childAt(15, 10);
}

// 找右下角缩放手柄（按布局位置取）
QWidget* bottomRightHandleOf(DA::DATableChildWindow* frame)
{
    return frame->childAt(frame->width() - 5, frame->height() - 5);
}
}  // namespace

/**
 * @brief 表格子窗口框架/容器测试
 *
 * 覆盖：attach 贴入与夹取、标题栏拖动移动、右下角手柄缩放（最小尺寸与
 * 允许区域夹取）、最大化/还原、关闭请求（默认隐藏）、takeAndDetach
 * 归还内容、宿主尺寸变化后的统一夹回、windowFor 查找。
 */
class DATableChildWindowTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testAttachWrapsAndClamps();
    void testTitleBarDrag();
    void testCornerHandleResize();
    void testResizeRespectsMinimum();
    void testMaximizeRestore();
    void testCloseRequestedHides();
    void testTakeAndDetach();
    void testContainerClampsOnHostShrink();
    void testWindowFor();

private:
    QWidget mHost;
    std::unique_ptr< DA::DATableChildWindowContainer > mContainer;
    QRect mBounds { 10, 10, 700, 500 };  // 固定允许区域
};

void DATableChildWindowTest::initTestCase()
{
    mHost.resize(800, 600);
    mHost.show();
    QTest::qWait(30);
    mContainer = std::make_unique< DA::DATableChildWindowContainer >(&mHost);
    mContainer->setBoundsProvider([ this ]() { return mBounds; });
}

void DATableChildWindowTest::testAttachWrapsAndClamps()
{
    auto* content = new QLabel(QStringLiteral("content"), &mHost);
    const QPoint preferredPos(650, 30);  // x 超出允许区域右缘 → 夹取
    DA::DATableChildWindow* frame = mContainer->attach(content, QStringLiteral("title"),
                                                        QSize(300, 200), preferredPos);
    QVERIFY(frame);
    QVERIFY(frame->isVisible());
    // 内容重父化进框架
    QCOMPARE(content->parentWidget(), frame);
    QCOMPARE(frame->title(), QStringLiteral("title"));
    QCOMPARE(frame->widget(), content);
    // 位置夹回允许区域内，完整包含
    QVERIFY(mBounds.contains(frame->geometry()));
    QCOMPARE(frame->geometry().topLeft(), QPoint(mBounds.right() - 300 + 1, 30));
    QCOMPARE(mContainer->windows().size(), 1);
    mContainer->detach(frame);
    QTest::qWait(10);
    QVERIFY(mContainer->windows().isEmpty());
}

void DATableChildWindowTest::testTitleBarDrag()
{
    auto* content = new QLabel(QStringLiteral("content"), &mHost);
    DA::DATableChildWindow* frame = mContainer->attach(content, QString(), QSize(300, 200), QPoint(50, 40));
    QVERIFY(frame);
    const QRect before = frame->geometry();
    QWidget* titleBar = titleBarOf(frame);
    QVERIFY2(titleBar, "title bar not found at expected position");
    QVERIFY(titleBar->height() >= 20);

    // 拖动 +40/+20：整体位移
    const QPoint start(15, 10);
    sendMouse(titleBar, QEvent::MouseButtonPress, start);
    sendMouse(titleBar, QEvent::MouseMove, start + QPoint(40, 20));
    sendMouse(titleBar, QEvent::MouseButtonRelease, start + QPoint(40, 20));
    QCOMPARE(frame->geometry(), before.translated(40, 20));
    QVERIFY(mBounds.contains(frame->geometry()));

    // 拖出允许区域右下缘：只夹位置，不改尺寸
    const QRect clampedBefore = frame->geometry();
    sendMouse(titleBar, QEvent::MouseButtonPress, start);
    sendMouse(titleBar, QEvent::MouseMove, start + QPoint(4000, 4000));
    sendMouse(titleBar, QEvent::MouseButtonRelease, start + QPoint(4000, 4000));
    QCOMPARE(frame->geometry(), QRect(QPoint(mBounds.right() - clampedBefore.width() + 1,
                                             mBounds.bottom() - clampedBefore.height() + 1),
                                      clampedBefore.size()));
    mContainer->detach(frame);
    QTest::qWait(10);
}

void DATableChildWindowTest::testCornerHandleResize()
{
    auto* content = new QLabel(QStringLiteral("content"), &mHost);
    DA::DATableChildWindow* frame = mContainer->attach(content, QString(), QSize(300, 200), QPoint(50, 40));
    QVERIFY(frame);
    QWidget* handle = bottomRightHandleOf(frame);
    QVERIFY2(handle, "bottom-right resize handle not found");
    QVERIFY(handle->isVisible());

    // 右下角往左上拖 60/40：宽高各缩 60/40，左上角不动
    const QRect before = frame->geometry();
    const QPoint center(handle->width() / 2, handle->height() / 2);
    sendMouse(handle, QEvent::MouseButtonPress, center);
    sendMouse(handle, QEvent::MouseMove, center - QPoint(60, 40));
    sendMouse(handle, QEvent::MouseButtonRelease, center - QPoint(60, 40));
    QCOMPARE(frame->geometry(),
             QRect(before.topLeft(), QSize(before.width() - 60, before.height() - 40)));
    QVERIFY(mBounds.contains(frame->geometry()));

    // 右下角拖过允许区域右下缘：尺寸在右下缘处停止
    const QRect before2 = frame->geometry();
    sendMouse(handle, QEvent::MouseButtonPress, center);
    sendMouse(handle, QEvent::MouseMove, center + QPoint(4000, 4000));
    sendMouse(handle, QEvent::MouseButtonRelease, center + QPoint(4000, 4000));
    QCOMPARE(frame->geometry().bottomRight(), mBounds.bottomRight());
    QVERIFY(mBounds.contains(frame->geometry()));
    mContainer->detach(frame);
    QTest::qWait(10);
}

void DATableChildWindowTest::testResizeRespectsMinimum()
{
    auto* content = new QLabel(QStringLiteral("content"), &mHost);
    content->setMinimumSize(280, 200);
    DA::DATableChildWindow* frame = mContainer->attach(content, QString(), QSize(500, 400), QPoint(60, 50));
    QVERIFY(frame);
    QVERIFY(frame->width() >= 280 + 12);
    QWidget* handle = bottomRightHandleOf(frame);
    QVERIFY(handle);

    // 往左上拖出最小尺寸：宽高夹在有效最小尺寸
    const QPoint center(handle->width() / 2, handle->height() / 2);
    sendMouse(handle, QEvent::MouseButtonPress, center);
    sendMouse(handle, QEvent::MouseMove, center - QPoint(4000, 4000));
    sendMouse(handle, QEvent::MouseButtonRelease, center - QPoint(4000, 4000));
    QVERIFY(frame->width() >= 280);
    QVERIFY(frame->height() >= 200);
    QVERIFY(mBounds.contains(frame->geometry()));
    mContainer->detach(frame);
    QTest::qWait(10);
}

void DATableChildWindowTest::testMaximizeRestore()
{
    auto* content = new QLabel(QStringLiteral("content"), &mHost);
    DA::DATableChildWindow* frame = mContainer->attach(content, QString(), QSize(300, 200), QPoint(50, 40));
    QVERIFY(frame);
    const QRect normal = frame->geometry();
    QSignalSpy maximizedSpy(frame, &DA::DATableChildWindow::maximizedChanged);

    frame->setMaximized(true);
    QVERIFY(frame->isMaximized());
    QCOMPARE(frame->geometry(), mBounds);
    QCOMPARE(maximizedSpy.size(), 1);
    QCOMPARE(maximizedSpy.takeFirst().at(0).toBool(), true);
    // 最大化期间手柄隐藏（不能再缩放）
    QVERIFY(!bottomRightHandleOf(frame) || !bottomRightHandleOf(frame)->isVisible());

    frame->setMaximized(false);
    QVERIFY(!frame->isMaximized());
    QCOMPARE(frame->geometry(), normal);
    QCOMPARE(maximizedSpy.size(), 1);
    QCOMPARE(maximizedSpy.takeFirst().at(0).toBool(), false);

    // 双击标题栏切换（toggleMaximized 路径）
    QWidget* titleBar = titleBarOf(frame);
    QVERIFY(titleBar);
    sendMouse(titleBar, QEvent::MouseButtonDblClick, QPoint(15, 10));
    QVERIFY(frame->isMaximized());
    QCOMPARE(frame->geometry(), mBounds);
    mContainer->detach(frame);
    QTest::qWait(10);
}

void DATableChildWindowTest::testCloseRequestedHides()
{
    auto* content = new QLabel(QStringLiteral("content"), &mHost);
    DA::DATableChildWindow* frame = mContainer->attach(content, QString(), QSize(300, 200), QPoint(50, 40));
    QVERIFY(frame);
    QSignalSpy closeSpy(frame, &DA::DATableChildWindow::closeRequested);
    // 标题栏上的关闭按钮
    QList< QToolButton* > buttons = frame->findChildren< QToolButton* >();
    QCOMPARE(buttons.size(), 1);
    QTest::mouseClick(buttons.first(), Qt::LeftButton);
    QCOMPARE(closeSpy.size(), 1);
    // 容器默认语义：关闭 = 隐藏（实例保留）
    QVERIFY(!frame->isVisible());
    QVERIFY(!mContainer->windows().isEmpty());
    mContainer->detach(frame);
    QTest::qWait(10);
}

void DATableChildWindowTest::testTakeAndDetach()
{
    auto* content = new QLabel(QStringLiteral("content"), &mHost);
    DA::DATableChildWindow* frame = mContainer->attach(content, QString(), QSize(300, 200), QPoint(50, 40));
    QVERIFY(frame);
    QPointer< DA::DATableChildWindow > guard(frame);
    QWidget* taken = mContainer->takeAndDetach(frame);
    QCOMPARE(taken, content);
    QVERIFY(!taken->parentWidget());  // 所有权归还调用方
    QVERIFY(mContainer->windows().isEmpty());
    QTest::qWait(20);
    QVERIFY(guard.isNull());  // 框架已销毁
    delete content;  // 内容归调用方，测试内手动回收
}

void DATableChildWindowTest::testContainerClampsOnHostShrink()
{
    // 允许区域跟随宿主尺寸（模拟主视口随表格收缩）
    QWidget host;
    host.resize(900, 700);
    host.show();
    QTest::qWait(20);
    DA::DATableChildWindowContainer container(&host);
    container.setBoundsProvider([ &host ]() { return QRect(0, 0, host.width(), host.height()); });
    auto* content = new QLabel(QStringLiteral("content"), &host);
    DA::DATableChildWindow* frame = container.attach(content, QString(), QSize(400, 300), QPoint(400, 300));
    QVERIFY(frame);
    QVERIFY(QRect(0, 0, host.width(), host.height()).contains(frame->geometry()));

    // 宿主收缩 → 容器把子窗口夹回新允许区域
    host.resize(420, 320);
    const QRect newBounds(0, 0, host.width(), host.height());
    QVERIFY(newBounds.contains(frame->geometry()));
    container.clampAllWindows();
    QVERIFY(newBounds.contains(frame->geometry()));
}

void DATableChildWindowTest::testWindowFor()
{
    auto* contentA = new QLabel(QStringLiteral("a"), &mHost);
    auto* contentB = new QLabel(QStringLiteral("b"), &mHost);
    DA::DATableChildWindow* frameA = mContainer->attach(contentA, QString(), QSize(200, 150), QPoint(20, 20));
    DA::DATableChildWindow* frameB = mContainer->attach(contentB, QString(), QSize(200, 150), QPoint(260, 20));
    QVERIFY(frameA && frameB);
    QCOMPARE(mContainer->windowFor(contentA), frameA);
    QCOMPARE(mContainer->windowFor(contentB), frameB);
    QCOMPARE(mContainer->windowFor(nullptr), nullptr);
    mContainer->detach(frameA);
    mContainer->detach(frameB);
    QTest::qWait(10);
    QVERIFY(mContainer->windows().isEmpty());
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DATableChildWindowTest tc;
    const int rc = QTest::qExec(&tc, argc, argv);
    std::fflush(stdout);
    std::fflush(stderr);
    return rc;
}

#include "main.moc"
