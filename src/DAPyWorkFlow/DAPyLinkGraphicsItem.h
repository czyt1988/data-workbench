#ifndef DAPYLINKGRAPHICSITEM_H
#define DAPYLINKGRAPHICSITEM_H
#include "DAPyWorkFlowAPI.h"
#include "DAGraphicsLinkItem.h"
#include <QTimer>

namespace DA
{

class DAPyNodeGraphicsItem;
class DAPythonSignalHandler;

/**
 * @brief Python工作流连接线图形项
 *
 * 用于在Python工作流中连接两个节点，支持三种连线样式：
 * - 贝塞尔曲线（默认）
 * - 直线
 * - 肘形连接（直角折线）
 *
 * 提供数据流动画效果和连接验证功能。
 *
 * @code
 * // 创建Python连接线
 * auto linkItem = new DA::DAPyLinkGraphicsItem();
 * linkItem->setStartScenePosition(startPos);
 * linkItem->setEndScenePosition(endPos);
 * linkItem->setLinkLineStyle(DA::DAGraphicsLinkItem::LinkLineBezier);
 * @endcode
 *
 * @see DAGraphicsLinkItem DAPyNodeGraphicsItem DAPythonSignalHandler
 */
class DAPYWORKFLOW_API DAPyLinkGraphicsItem : public DAGraphicsLinkItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyLinkGraphicsItem)
public:
    /**
     * @brief Item类型标识
     */
    enum
    {
        Type = DA::ItemType_GraphicsNodeUserType + 2  ///< ItemType_DAPyNodeLinkGraphicsItem
    };

    int type() const override
    {
        return (Type);
    }

public:
    DAPyLinkGraphicsItem(QGraphicsItem* parent = nullptr);
    ~DAPyLinkGraphicsItem();

    // 设置/获取数据流状态
    void setDataFlowing(bool flowing);
    bool isDataFlowing() const;

    // 设置Python信号处理器（用于接收数据流通知）
    void setSignalHandler(DAPythonSignalHandler* handler);
    DAPythonSignalHandler* getSignalHandler() const;

    // 设置连接的源节点和目标节点
    void setFromNode(DAPyNodeGraphicsItem* node, const QString& outputName);
    void setToNode(DAPyNodeGraphicsItem* node, const QString& inputName);
    DAPyNodeGraphicsItem* getFromNode() const;
    DAPyNodeGraphicsItem* getToNode() const;

    // 获取连接点信息
    QString getFromOutputName() const;
    QString getToInputName() const;

    // 在将要结束链接的回调，验证数据类型的兼容性
    virtual bool willCompleteLink() override;

    // 绘图函数，添加数据流动画效果
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    // 保存/加载
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
    virtual bool loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver) override;

protected:
    // 数据流动画定时器回调
    void onDataFlowTimer();

    // 获取数据流高亮颜色
    QColor getDataFlowHighlightColor() const;

    // 绘制数据流高亮效果
    void paintDataFlowHighlight(QPainter* painter, const QStyleOptionGraphicsItem* option, const QPainterPath& linkPath);

private:
    // 检查数据类型兼容性
    bool checkDataTypeCompatibility(const QString& outputType, const QString& inputType) const;
};

}  // end of namespace DA

#endif  // DAPYLINKGRAPHICSITEM_H