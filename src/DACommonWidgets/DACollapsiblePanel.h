#ifndef DACOLLAPSIBLEPANEL_H
#define DACOLLAPSIBLEPANEL_H

#include "DACommonWidgetsAPI.h"
#include "DACollapsibleGroupBox.h"

namespace DA
{

/**
 * @brief 可折叠面板控件，继承自DACollapsibleGroupBox，复用ctkCollapsibleGroupBox的成熟折叠机制
 *
 * 支持点击标题区域展开/收起内容区域，并提供多种样式模式。
 * 继承DACollapsibleGroupBox（ctkCollapsibleGroupBox → QGroupBox），复用CTK的折叠逻辑和箭头指示器，
 * 不再使用自定义头部控件。适用于DAPropertyPanelWidget的分组嵌套。
 * 支持三种样式模式：Flat（无边框默认外观）、GroupBox（边框+标题断开顶部边线）、Bordered（矩形边框）。
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
 *
 * // 设置样式模式
 * panel->setStyleMode(DACollapsiblePanel::GroupBox);  // GroupBox风格边框
 * panel->setStyleMode(DACollapsiblePanel::Bordered);  // 简单矩形边框
 * panel->setStyleMode(DACollapsiblePanel::Flat);      // 无边框默认外观
 * @endcode
 *
 * @see DAPropertyPanelWidget, DACollapsibleGroupBox
 */
class DACOMMONWIDGETS_API DACollapsiblePanel : public DACollapsibleGroupBox
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DACollapsiblePanel)

    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(StyleMode styleMode READ styleMode WRITE setStyleMode NOTIFY styleModeChanged)

public:
    /**
     * @brief 面板样式模式
     */
    enum StyleMode
    {
        Flat,     ///< 无边框，当前默认外观
        GroupBox, ///< 类似QGroupBox，边框包围内容区域，标题断开顶部边线
        Bordered  ///< 简单矩形边框包围整个面板（标题+内容）
    };
    Q_ENUM(StyleMode)

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

    // 样式模式
    void setStyleMode(StyleMode mode);
    StyleMode styleMode() const;

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

    /**
     * @brief 样式模式变化信号
     * @param mode 新的样式模式
     */
    void styleModeChanged(StyleMode mode);

protected:
    void paintEvent(QPaintEvent* event) override;
};

}  // namespace DA

#endif  // DACOLLAPSIBLEPANEL_H