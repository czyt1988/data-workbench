#ifndef DATABLECHILDWINDOW_H
#define DATABLECHILDWINDOW_H
#include "DAGuiAPI.h"
#include <QPointer>
#include <QWidget>
#include <functional>

namespace DA
{
/**
 * @brief 表格子窗口框架（Excel 式浮层）
 *
 * 把任意 QWidget 贴入后成为表格里的一个可移动、可拖拽缩放的子窗口：
 * - 标题栏拖动移动、双击最大化/还原、右键菜单（最大化/还原/重置/关闭）；
 * - 4 角抓取块 + 4 边拖拽带缩放（子控件浮层没有系统边框，缩放把手必须自绘）；
 * - 全部几何变化夹取在 setBoundsProvider 给出的允许区域内（如主视口），不压滚动条。
 *
 * 框架拥有贴入的内容部件（setWidget 后由 Qt 父子关系接管生命周期）；
 * 「关闭」只发 closeRequested，由宿主决定隐藏或销毁。
 */
class DAGUI_API DATableChildWindow : public QWidget
{
    Q_OBJECT
public:
    // 返回宿主坐标系（parentWidget 坐标）下的允许矩形
    using BoundsProvider = std::function< QRect() >;

    explicit DATableChildWindow(QWidget* parent = nullptr);
    ~DATableChildWindow() override;

    // 贴入内容部件（重父化进框架，框架接管所有权并显示）
    void setWidget(QWidget* w);
    // 当前内容部件（未贴入返回 nullptr）
    QWidget* widget() const;
    // 取出内容部件（重父化到 nullptr，所有权归还调用方；框架随后可继续 setWidget）
    QWidget* takeWidget();

    // 标题栏文字
    void setTitle(const QString& title);
    QString title() const;

    // 允许区域提供者（空或返回无效矩形时回退 parentWidget()->rect()）
    void setBoundsProvider(const BoundsProvider& provider);

    // 最大化（充满允许区域）/还原；最大化期间记忆还原几何
    bool isMaximized() const;
    void setMaximized(bool on);
    void toggleMaximized();
    // 回到默认几何（默认尺寸 + 允许区域右上角）
    void resetToDefaultGeometry();
    // 把窗口完整夹回允许区域（位置与尺寸都要在界内；区域过小时只夹位置）
    void clampIntoBounds();
    // 默认尺寸（resetToDefaultGeometry 用；未设置时取当前尺寸）
    void setDefaultSize(const QSize& size);

Q_SIGNALS:
    // 用户点关闭（标题栏按钮/右键菜单/Esc）；框架自身不隐藏，由宿主处理
    void closeRequested();
    // 最大化状态切换
    void maximizedChanged(bool maximized);
    // 用户拖动/缩放/最大化/重置结束后的最终几何
    void geometryUserChanged();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    class TitleBar;
    class ResizeHandle;

    // 允许区域（无提供者或提供者返回无效矩形时回退 parentWidget()->rect()）
    QRect boundsRect() const;
    // 有效最小尺寸 = 显式 minimumSize 与布局 minimumSizeHint 逐轴取大
    QSize effectiveMinimumSize() const;
    // 重排 4 角抓取块与 4 边拖拽带，并置顶（保持在内容部件之上）
    void updateHandleGeometries();
    // 缩放手柄可见性（最大化时隐藏）
    void setHandlesVisible(bool on);
    // 标题栏右键菜单
    void showTitleContextMenu(const QPoint& globalPos);

private:
    TitleBar* mTitleBar { nullptr };
    QPointer< QWidget > mContent;
    BoundsProvider mBoundsProvider;
    QString mTitle;
    QSize mDefaultSize;
    QRect mRestoreGeometry;  ///< 最大化前的正常几何
    bool mMaximized { false };
    QList< ResizeHandle* > mHandles;
};

}  // namespace DA
#endif  // DATABLECHILDWINDOW_H
