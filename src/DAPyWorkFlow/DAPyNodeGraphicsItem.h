#ifndef DAPYNODEGRAPHICSITEM_H
#define DAPYNODEGRAPHICSITEM_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyLinkPoint.h"
#include "DAGraphicsResizeableItem.h"
#include "DAPyGILGuard.h"
#include "DAPyNodeState.h"
#include "DAPyNodeStyle.h"
#include "DANodeDescriptor.h"
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
    using RenderTemplate = DA::RenderTemplate;  ///< 向后兼容：引用命名空间级 enum class

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

    // Widget相关（用于widget模式）
    void setWidget(QWidget* widget);
    QWidget* getWidget() const;

    // 节点状态
    DAPyNodeState getNodeState() const;
    void setNodeState(DAPyNodeState state);

    // 节点描述符结构体（C++原生描述符）
    void setDescriptorStruct(const DANodeDescriptor& desc);
    const DANodeDescriptor& getDescriptorStruct() const;

    // 节点样式
    void setNodeStyle(const DANodeStyle& style);
    DANodeStyle& nodeStyle();
    const DANodeStyle& nodeStyle() const;

    // 自定义绘制回调接口
    void setPaintCallback(const pybind11::object& callback);
    bool hasPaintCallback() const;
    void clearPaintCallback();

    // 连接点管理
    QList< DAPyLinkPoint > getInputLinkPoints() const;
    QList< DAPyLinkPoint > getOutputLinkPoints() const;
    void updateLinkPoints();

    // 保存/加载
    bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
    bool loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver) override;

    // 设置body尺寸（重写基类public虚函数，须保持public可见性）
    void setBodySize(const QSizeF& s) override;

    // 边界矩形（扩展以容纳下方名称）
    QRectF boundingRect() const override;

    // 碰撞形状（椭圆时使用椭圆路径）
    QPainterPath shape() const override;

    // 鼠标双击事件
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    // 更新节点形状信息，在设置节点名字图标后，估算一个最优的尺寸
    void updateNodeBody();
    // 更新widget位置
    void updateWidgetGeometry();
    // 更新nodestyle位置
    void updateNodeStyleGeometry();
Q_SIGNALS:
    /**
     * @brief 节点双击信号，通知上层（DAGui）弹出配置对话框
     * @param[in] proxy 双击的节点代理
     */
    void nodeDoubleClicked(DA::DAPyNodeProxy* proxy);

protected:
    // 绘制body（根据模板类型选择绘制方式）
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;

    // 位置变化时刷新连接线
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;

    // 分组位置变化时刷新连接线
    void groupPositionChanged(const QPointF& pos) override;

    // 绘制连接点
    void paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    // 绘制状态边框/背景
    void paintStateDecoration(QPainter* painter, const QRectF& bodyRect);

    // 统一节点样式模板绘制（T7新增）
    void paintNodeStyleBody(QPainter* painter, const QRectF& bodyRect);

    // 各种模板绘制函数（旧版，已废弃）
    void paintWidgetTemplate(QPainter* painter, const QRectF& bodyRect);

    // 生成连接点
    QList< DAPyLinkPoint > generateLinkPoints() const;

private:
    // 刷新连接线位置（委托给场景的updateNodeLinkPositions）
    void updateLinkItems();
    // 获取当前状态颜色
    QColor getStateColor() const;
};

}  // end of namespace DA

#endif  // DAPYNODEGRAPHICSITEM_H
