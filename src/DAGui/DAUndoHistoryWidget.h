#ifndef DAUNDOHISTORYWIDGET_H
#define DAUNDOHISTORYWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"

class QUndoGroup;
class QUndoView;
class QSpinBox;

namespace DA
{
/**
 * @brief 撤销历史停靠面板（005 T1.21，widget 在 DAGui、APP 只做挂载接线，D0）
 *
 * 内容用 Qt 内置 QUndoView 绑到全局 QUndoGroup（DAInterface 的 DACommandInterface
 * 持有，APP 注入——DAGui 不依赖 DAInterface/L4，只认 QUndoGroup）：跟随当前激活
 * 页面的撤销栈、点任意条目前进/回退到该状态、显示命令描述（文案来自各命令 setText
 * 的 tr()，红线5：面板里不允许空白条目）。
 *
 * 撤销深度调节（R19）：面板提供 QSpinBox 入口，改动只发 undoLimitChangeRequested
 * 信号——配置读取/写回（DA_CONFIG_KEY_UNDO_STACK_LIMIT，dawork-config.xml）与对各
 * 数据页 QUndoStack 的施加都在 APP 侧完成（D0 分层：严禁 DAGui→APP 反向依赖，
 * DAGui 只暴露 undoLimit()/setUndoLimit(int) 口径的接口）。
 */
class DAGUI_API DAUndoHistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DAUndoHistoryWidget(QWidget* parent = nullptr);
    ~DAUndoHistoryWidget();

    // 绑定全局撤销组（APP 挂载时注入；视图自动跟随组内激活栈）
    void setUndoGroup(QUndoGroup* group);
    QUndoGroup* undoGroup() const;

    // 撤销深度（SpinBox 当前值；APP 启动读配置后经 setUndoLimit 注入，不触发信号）
    int undoLimit() const;
    void setUndoLimit(int limit);

Q_SIGNALS:
    // 用户在面板调整了撤销深度（APP 接线：写回配置并施加到全部数据页撤销栈）
    void undoLimitChangeRequested(int limit);

private:
    QUndoView* mView { nullptr };
    QSpinBox* mSpinLimit { nullptr };
    bool mUpdatingLimit { false };  ///< setUndoLimit 期间屏蔽 valueChanged 回环
};
}  // end of namespace DA
#endif  // DAUNDOHISTORYWIDGET_H
