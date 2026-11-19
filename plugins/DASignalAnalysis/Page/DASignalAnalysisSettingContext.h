#ifndef DASIGNALANALYSISSETTINGCONTEXT_H
#define DASIGNALANALYSISSETTINGCONTEXT_H
#include <QObject>
#include <QList>
#include <QPointer>
#include "DASettingContextProvider.h"
#include "Page/DASignalAnalysisView.h"

class QUndoStack;
namespace DA
{
class DASignalAnalysisUI;
class DAFigureWidget;

/**
 * @brief 信号分析页的设置上下文提供者
 *
 * 信号分析页（DASignalAnalysisPage）经 DADockingAreaInterface 注册本提供者后，
 * 该页 dock 被激活时由本类接管右侧设置面板、图元树、Ribbon 上下文标签与窗口底部工具栏；
 * 切走时还原宿主绘图区的目标。当前子图（2D/3D）或信号标签切换时刷新设置面板 target。
 */
class DASignalAnalysisSettingContext : public QObject, public DASettingContextProvider
{
    Q_OBJECT
public:
    explicit DASignalAnalysisSettingContext(DASignalAnalysisUI* ui, QObject* parent = nullptr);
    ~DASignalAnalysisSettingContext() override;

    // 该 dock 成为激活页：显示设置 dock、切设置页/图元树/上下文标签/底部工具栏
    void activateSettingContext() override;
    // 失活：隐藏上下文标签与底部工具栏，图元树/设置面板恢复宿主绘图区
    void deactivateSettingContext() override;
    // 当前信号 figure 的撤销栈
    QUndoStack* contextUndoStack() const override;
    // 绘图操作目标：当前信号 figure；无视图时返回 nullptr（宿主据此显示/隐藏内置「绘图操作」上下文）
    DAFigureWidget* chartContextFigure() const override;

    // 激活状态下刷新设置面板 target 与图元树当前 figure（切信号/子图时调用）
    void refreshTarget();
    // 把右侧 3D 设置面板切到当前三维瀑布图元（丝带「瀑布设置…」快捷入口）
    void showWaterfallItemSetting();
    // 是否处于激活状态
    bool isActive() const;

private Q_SLOTS:
    // 信号标签切换：重绑当前视图并刷新 target
    void onPageCurrentSignalChanged();
    // 当前 2D/3D 子图切换：刷新设置面板 target
    void onViewSubChartChanged();

private:
    // 绑定当前视图的子图变化信号（重绑前先断开）
    void bindCurrentView();
    void unbindCurrentView();
    // 按当前子图类型切设置页并设 target（2D 页 / 3D 页）
    void applySettingTarget();
    // 图元树目标 = 当前信号 figure；恢复时由宿主接管
    void updateChartManageTarget();
    // 取当前信号视图（页面未创建返回 nullptr）
    DASignalAnalysisView* currentView() const;

private:
    QPointer< DASignalAnalysisUI > mUI;
    QPointer< DASignalAnalysisView > mView;
    QMetaObject::Connection mViewConnection;
    // 上次注入图元树的 figure 列表：列表同构时只切当前项，不重建 combo/树视图（D1-09）
    QList< DAFigureWidget* > mFigureListCache;
    bool mActive { false };
};
}  // namespace DA

#endif  // DASIGNALANALYSISSETTINGCONTEXT_H
