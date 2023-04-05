#ifndef DAMESSAGELOGVIEWWIDGET_H
#define DAMESSAGELOGVIEWWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
class QMenu;
namespace Ui
{
class DAMessageLogViewWidget;
}

namespace DA
{
class DAMessageLogsModel;
class DAMessageLogsSortFilterProxyModel;
/**
 * @brief 用于显示全局的消息
 */
class DAGUI_API DAMessageLogViewWidget : public QWidget
{
    Q_OBJECT

public:
    DAMessageLogViewWidget(QWidget* parent = nullptr);
    ~DAMessageLogViewWidget();
    //是否显示debug
    bool isEnableShowDebugMsg() const;
    //是否显示Warning
    bool isEnableShowWarningMsg() const;
    //是否显示Critical
    bool isEnableShowCriticalMsg() const;
    //是否显示Fatal
    bool isEnableShowFatalMsg() const;
    //是否显示Info
    bool isEnableShowInfoMsg() const;
public slots:
    //对debug的显示设置
    void setEnableShowDebugMsg(bool on);
    //对Warning的显示设置
    void setEnableShowWarningMsg(bool on);
    //对Critical的显示设置
    void setEnableShowCriticalMsg(bool on);
    //对Fatal的显示设置
    void setEnableShowFatalMsg(bool on);
    //对Info的显示设置
    void setEnableShowInfoMsg(bool on);
    //点击
    void onTableViewItemClicked(const QModelIndex& index);
    //清空所有
    void clearAll();
    //复制选中的消息到剪切板
    void copySelectionMessageToClipBoard();
    //选中所有
    void selectAll();

protected:
    //多语言切换的捕获
    virtual void changeEvent(QEvent* event) override;
    //所有的文本都在此函数设置
    void retranslateUi();
    //处理快捷键
    virtual void keyPressEvent(QKeyEvent* event) override;

public:
    /**
     * @brief 定义了窗口的内部action，外部程序可以通过此枚举及@ref getAction 函数获取对应的action
     */
    enum MessageLogActions
    {
        ActionInfo,
        ActionWarning,
        ActionCritial,
        ActionClear,
        ActionCopy
    };
    //获取action
    QAction* getAction(MessageLogActions ac) const;

protected:
    QAction* createAction(const char* objname, const char* iconpath, bool checkable = false, bool checked = false);
private slots:
    //自定义菜单
    void onCustomContextMenuRequested(const QPoint& pos);

private:
    void buildMenu();

private:
    Ui::DAMessageLogViewWidget* ui;
    DAMessageLogsModel* _model;
    DAMessageLogsSortFilterProxyModel* _sortFilterModel;
    QAction* _actionMessageLogShowInfo;
    QAction* _actionMessageLogShowWarning;
    QAction* _actionMessageLogShowCritical;
    QAction* _actionMessageLogClear;    ///< 清空消息
    QAction* _actionCopySelectMessage;  ///< 复制选中的消息
    QMenu* _menu;
};
}  // namespace DA
#endif  // DAMESSAGELOGVIEWWIDGET_H
