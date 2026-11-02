#ifndef DATABLECHILDWINDOWCONTAINER_H
#define DATABLECHILDWINDOWCONTAINER_H
#include "DAGuiAPI.h"
#include <QList>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <functional>

class QWidget;

namespace DA
{
class DATableChildWindow;

/**
 * @brief 表格子窗口容器
 *
 * 表格页级的子窗口管理者（QObject，不是覆盖式 QWidget——覆盖层会挡住
 * 表格的鼠标事件，子窗口直接父挂在宿主部件上）。任意 QWidget 经
 * attach() 贴入后变成表格里的一个 DATableChildWindow：可拖动移动、
 * 8 向手柄缩放、最大化/还原，全程夹取在允许区域内（如主视口），
 * 不压表头与滚动条。
 *
 * - attach()/detach()/takeAndDetach() 管理生命周期；
 * - boundsProvider 给出允许区域（如主视口在宿主坐标系下的矩形）；
 * - watchGeometrySource 监听几何源（视口/窗格容器），其移动/缩放/
 *   显隐后统一把所有子窗口夹回允许区域；
 * - 子窗口点「关闭」默认由容器隐藏（closeRequested → hide），复用型
 *   宿主（快速绘图）可在窗口实例上再接线自定义关闭语义。
 */
class DAGUI_API DATableChildWindowContainer : public QObject
{
    Q_OBJECT
public:
    // 返回宿主坐标系下的允许矩形
    using BoundsProvider = std::function< QRect() >;

    explicit DATableChildWindowContainer(QWidget* host, QObject* parent = nullptr);
    ~DATableChildWindowContainer() override;

    // 宿主部件（子窗口的视觉父级）
    QWidget* host() const;

    // 允许区域提供者（空或返回无效矩形时回退 host->rect()）
    void setBoundsProvider(const BoundsProvider& provider);

    // 贴入任意部件 → 变成表格里的子窗口（content 重父化进框架，框架接管所有权）
    // preferredSize/preferredPos 缺省时取内容 sizeHint 与允许区域右上角
    DATableChildWindow* attach(QWidget* content,
                               const QString& title,
                               const QSize& preferredSize = QSize(),
                               const QPoint& preferredPos = QPoint());
    // 摘除并销毁子窗口（连同内容部件）；无效窗口安全返回
    void detach(DATableChildWindow* w);
    // 摘除子窗口但把内容部件归还（重父化到 nullptr，调用方接管所有权）
    QWidget* takeAndDetach(DATableChildWindow* w);
    // 当前全部子窗口
    QList< DATableChildWindow* > windows() const;
    // 内容部件对应的子窗口（未贴入返回 nullptr）
    DATableChildWindow* windowFor(const QWidget* content) const;

    // 监听几何源（主视口/窗格容器）：其 Move/Resize/Show/Hide → 全部子窗口夹回允许区域
    void watchGeometrySource(QWidget* w);
    // 立即把全部子窗口夹回允许区域
    void clampAllWindows();

Q_SIGNALS:
    // 子窗口已贴入
    void windowAttached(DA::DATableChildWindow* w);
    // 子窗口即将销毁（detach/deleteLater 前）
    void windowDetached(DA::DATableChildWindow* w);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 允许区域（提供者缺失/无效时回退 host->rect()）
    QRect boundsRect() const;

private:
    QPointer< QWidget > mHost;
    BoundsProvider mBoundsProvider;
    QList< QPointer< DATableChildWindow > > mWindows;
    QList< QPointer< QWidget > > mWatched;
};

}  // namespace DA
#endif  // DATABLECHILDWINDOWCONTAINER_H
