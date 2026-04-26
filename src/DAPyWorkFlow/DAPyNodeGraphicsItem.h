#ifndef DAPYNODEGRAPHICSITEM_H
#define DAPYNODEGRAPHICSITEM_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyLinkPoint.h"
#include "DAGraphicsResizeableItem.h"
#include "DAPyGILGuard.h"
#include "DAPyNodeState.h"
#include <QIcon>
#include <QJsonObject>
#include <QGraphicsSceneMouseEvent>

class QDomDocument;
class QDomElement;
class QSvgRenderer;
class QGraphicsProxyWidget;

namespace DA
{

class DAPyNodeProxy;
class DAPyNodePalette;

/**
 * @brief Python工作流节点的图形项
 *
 * 用于在画布上渲染Python工作流节点，支持三种渲染模板模式：
 * - rect: 矩形模式，绘制圆角矩形和节点名称
 * - svg: SVG图标模式，从指定路径加载SVG图标
 * - widget: 嵌入Qt Widget模式
 *
 * 继承自DAGraphicsResizeableItem，使用paintBody()进行自定义绘制。
 * 支持状态颜色映射（通过DAPyNodePalette）和连接点渲染。
 *
 * @code
 * // 创建Python节点图形项
 * auto item = new DA::DAPyNodeGraphicsItem(proxy);
 * item->setRenderTemplate("rect");
 * item->setNodeName("MyNode");
 * @endcode
 *
 * @see DAGraphicsResizeableItem DAPyNodeProxy DAPyNodePalette
 */
class DAPYWORKFLOW_API DAPyNodeGraphicsItem : public DAGraphicsResizeableItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyNodeGraphicsItem)

public:
    /**
     * @brief 渲染模板类型
     */
    enum RenderTemplate
    {
        RectTemplate,   ///< 矩形模板
        SvgTemplate,    ///< SVG图标模板
        WidgetTemplate  ///< 嵌入Widget模板
    };

    /**
     * @brief Item类型标识
     */
    enum
    {
        Type = DA::ItemType_GraphicsNodeUserType + 1  ///< ItemType_DAPyNodeGraphicsItem
    };

    int type() const override
    {
        return (Type);
    }

public:
    // 构造/析构
    explicit DAPyNodeGraphicsItem(DAPyNodeProxy* proxy, QGraphicsItem* parent = nullptr);
    ~DAPyNodeGraphicsItem();

    // 渲染模板设置
    void setRenderTemplate(RenderTemplate tmpl);
    void setRenderTemplate(const QString& tmplName);
    RenderTemplate getRenderTemplate() const;
    QString getRenderTemplateName() const;

    // 节点数据
    DAPyNodeProxy* getProxy() const;
    void setProxy(DAPyNodeProxy* proxy);

    // 节点名称
    void setNodeName(const QString& name);
    QString getNodeName() const;

    // 图标设置（用于rect和widget模式）
    void setIcon(const QIcon& icon);
    QIcon getIcon() const;

    // SVG相关（用于svg模式）
    void setSvgPath(const QString& path);
    QString getSvgPath() const;
    bool loadSvg(const QString& path);

    // Widget相关（用于widget模式）
    void setWidget(QWidget* widget);
    QWidget* getWidget() const;

    // 节点状态
    DAPyNodeState getNodeState() const;
    void setNodeState(DAPyNodeState state);

    // 节点描述符（用于获取输入/输出信息）
    void setDescriptor(const QJsonObject& desc);
    QJsonObject getDescriptor() const;

    // 自定义绘制回调接口
    void setPaintCallback(const pybind11::object& callback);
    bool hasPaintCallback() const;
    void clearPaintCallback();

    // 连接点管理
    QList<DAPyLinkPoint> getInputLinkPoints() const;
    QList<DAPyLinkPoint> getOutputLinkPoints() const;
    void updateLinkPoints();

    // 保存/加载
    bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
    bool loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver) override;

    // 设置body尺寸（重写基类public虚函数，须保持public可见性）
    void setBodySize(const QSizeF& s) override;

    // 鼠标双击事件
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

Q_SIGNALS:
    /**
     * @brief 节点双击信号，通知上层（DAGui）弹出配置对话框
     * @param[in] proxy 双击的节点代理
     */
    void nodeDoubleClicked(DA::DAPyNodeProxy* proxy);

protected:
    // 绘制body（根据模板类型选择绘制方式）
    void paintBody(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget* widget,
                   const QRectF& bodyRect) override;

    // 绘制连接点
    void paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    // 绘制状态边框/背景
    void paintStateDecoration(QPainter* painter, const QRectF& bodyRect);

    // 各种模板绘制函数
    void paintRectTemplate(QPainter* painter, const QRectF& bodyRect);
    void paintSvgTemplate(QPainter* painter, const QRectF& bodyRect);
    void paintWidgetTemplate(QPainter* painter, const QRectF& bodyRect);

    // 生成连接点
    QList<DAPyLinkPoint> generateLinkPoints() const;

private:
    // 获取当前状态颜色
    QColor getStateColor() const;
    // 更新widget位置
    void updateWidgetGeometry();
};

}  // end of namespace DA

#endif  // DAPYNODEGRAPHICSITEM_H
