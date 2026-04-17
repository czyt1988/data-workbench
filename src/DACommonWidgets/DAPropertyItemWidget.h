#ifndef DAPROPERTYITEMWIDGET_H
#define DAPROPERTYITEMWIDGET_H

#include "DACommonWidgetsAPI.h"
#include <QFrame>
#include <QString>

namespace DA
{
/**
 * @brief 属性项控件，支持Inline和Below两种布局模式
 * 
 * Inline模式：属性名标签在左侧（固定宽度，可换行），编辑Widget在右侧同一行（顶部对齐）
 * Below模式：属性名标签在顶部，编辑Widget在下方占满整行宽度
 * 
 * 描述文本作为属性名标签的tooltip显示。
 * 
 * @code
 * // Inline模式使用示例
 * DAPropertyItemWidget* item = new DAPropertyItemWidget(this);
 * item->setPropertyName("画笔颜色");
 * item->setPropertyDescription("设置曲线的画笔颜色");
 * item->setEditorWidget(new DAColorPickerButton(this));
 * item->setLayoutMode(DAPropertyItemWidget::InlineLayout);
 * @endcode
 * 
 * @see DAPropertyPanelWidget
 */
class DACOMMONWIDGETS_API DAPropertyItemWidget : public QFrame
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPropertyItemWidget)
    Q_PROPERTY(int propertyId READ propertyId WRITE setPropertyId)
    Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName)
    Q_PROPERTY(QString propertyDescription READ propertyDescription WRITE setPropertyDescription)
    Q_PROPERTY(LayoutMode layoutMode READ layoutMode WRITE setLayoutMode)
    Q_PROPERTY(bool frameVisible READ isFrameVisible WRITE setFrameVisible)
public:
    // 布局模式枚举
    enum LayoutMode {
        InlineLayout,  ///< Inline模式：左侧属性名 + 右侧编辑Widget
        BelowLayout    ///< Below模式：顶部属性名 + 下方编辑Widget
    };

    explicit DAPropertyItemWidget(QWidget* parent = nullptr);
    ~DAPropertyItemWidget();

    // 属性ID
    int propertyId() const;
    void setPropertyId(int id);

    // 属性名称
    QString propertyName() const;
    void setPropertyName(const QString& name);

    // 属性描述（作为tooltip）
    QString propertyDescription() const;
    void setPropertyDescription(const QString& desc);

    // 编辑Widget
    QWidget* editorWidget() const;
    void setEditorWidget(QWidget* widget);

    // 布局模式
    LayoutMode layoutMode() const;
    void setLayoutMode(LayoutMode mode);

    // Frame边界可见性
    bool isFrameVisible() const;
    void setFrameVisible(bool visible);

    // 属性名标签宽度（仅Inline模式有效）
    void setNameLabelWidth(int width);

Q_SIGNALS:
    /**
     * @brief 编辑值变化信号
     * @param propertyId 属性ID
     */
    void valueChanged(int propertyId);

private:
    void updateLayout();
};
}  // namespace DA
#endif // DAPROPERTYITEMWIDGET_H