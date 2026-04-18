#ifndef DACOLLAPSIBLEPANEL_H
#define DACOLLAPSIBLEPANEL_H

#include "DACommonWidgetsAPI.h"
#include <QWidget>
#include <QString>
#include <QFrame>
#include <QLabel>
#include <QStyleOption>

namespace DA
{

// Forward declaration
class DACollapsiblePanelHeader;

/**
 * @brief 可折叠面板控件，支持点击头部展开/收起内容区域
 *
 * 基于QWidget的轻量级折叠容器，使用setVisible+sizeHint实现折叠机制，
 * 不依赖QGroupBox，无动画。适用于DAPropertyPanelWidget的分组嵌套。
 *
 * @code
 * // 创建折叠面板
 * DACollapsiblePanel* panel = new DACollapsiblePanel("数据设置", parent);
 *
 * // 获取内容区域并添加子控件
 * QWidget* content = panel->getContentWidget();
 * QVBoxLayout* layout = new QVBoxLayout(content);
 * layout->addWidget(someWidget);
 *
 * // 程序化控制展开/收起
 * panel->setExpanded(false);  // 收起
 * panel->setExpanded(true);   // 展开
 * @endcode
 *
 * @see DAPropertyPanelWidget
 */
class DACOMMONWIDGETS_API DACollapsiblePanel : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DACollapsiblePanel)

    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

public:
    // 构造函数
    explicit DACollapsiblePanel(QWidget* parent = nullptr);
    explicit DACollapsiblePanel(const QString& title, QWidget* parent = nullptr);
    ~DACollapsiblePanel();

    // 内容区域管理
    void setContentWidget(QWidget* widget);
    QWidget* getContentWidget() const;

    // 展开/收起控制
    void setExpanded(bool expanded);
    bool isExpanded() const;

    // 标题
    void setTitle(const QString& title);
    QString title() const;

    // 大小提示
    QSize sizeHint() const override;

Q_SIGNALS:
    /**
     * @brief 展开状态变化信号
     * @param expanded true为展开，false为收起
     */
    void expandedChanged(bool expanded);

    /**
     * @brief 标题变化信号
     * @param title 新标题文本
     */
    void titleChanged(const QString& title);

private Q_SLOTS:
    void onHeaderClicked();

protected:
    void resizeEvent(QResizeEvent* event) override;
};

}  // namespace DA

#endif  // DACOLLAPSIBLEPANEL_H
