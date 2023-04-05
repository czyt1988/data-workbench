#ifndef DAWORKFLOWGRAPHICSSCENE_H
#define DAWORKFLOWGRAPHICSSCENE_H
#include <QGraphicsScene>
#include "DAGuiAPI.h"
#include <QHash>
#include <QUndoStack>
#include "DAWorkFlow.h"
#include "DANodeGraphicsScene.h"
#include "DAAbstractNodeGraphicsItem.h"
class QGraphicsSceneWheelEvent;
namespace DA
{
class DAGraphicsResizeablePixmapItem;
class DAGUI_API DAWorkFlowGraphicsScene : public DANodeGraphicsScene
{
    Q_OBJECT
public:
    /**
     * @brief 鼠标动作标记
     */
    enum MouseActionFlag
    {
        NoMouseAction,  ///< 无鼠标动作
        StartAddRect,   ///< 开始添加矩形
        StartAddText    ///< 开始添加文本
    };
    Q_ENUM(MouseActionFlag)
public:
    DAWorkFlowGraphicsScene(QObject* parent = 0);
    ~DAWorkFlowGraphicsScene();
    //设置鼠标动作，一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
    void setMouseAction(MouseActionFlag mf, bool continuous = false);
    MouseActionFlag getMouseAction() const;
    //鼠标动作是否连续执行
    bool isMouseActionContinuoue() const;
    //===================================================
    //背景图相关操作
    //===================================================
    //设置背景item，如果外部调用getBackgroundPixmapItem并删除，需要通过此函数把保存的item设置为null
    void setBackgroundPixmapItem(DAGraphicsResizeablePixmapItem* item);
    DAGraphicsResizeablePixmapItem* removeBackgroundPixmapItem();
    //允许item跟随背景图移动
    void enableItemMoveWithBackground(bool on);
    //是否item跟随背景图移动
    bool isEnableItemMoveWithBackground() const;
    //添加一个背景图,如果多次调用，此函数返回的QGraphicsPixmapItem* 是一样的，也就是只会创建一个QGraphicsPixmapItem*
    DAGraphicsResizeablePixmapItem* setBackgroundPixmap(const QPixmap& pixmap);
    //获取背景图item，如果没有设置返回一个nullptr
    DAGraphicsResizeablePixmapItem* getBackgroundPixmapItem();

    //设置文本字体
    QFont getDefaultTextFont() const;
    void setDefaultTextFont(const QFont& f);
    //设置文本颜色
    QColor getDefaultTextColor() const;
    void setDefaultTextColor(const QColor& c);

protected:
    DAGraphicsResizeablePixmapItem* ensureGetBackgroundPixmapItem();
    DAGraphicsResizeablePixmapItem* createBackgroundPixmapItem();

protected:
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent* event) override;
    virtual void dropEvent(QGraphicsSceneDragDropEvent* event) override;

    //鼠标点击事件
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
    //鼠标释放
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
signals:
    /**
     * @brief 鼠标动作已经执行完毕
     * @param mf 已经执行完的鼠标动作
     */
    void mouseActionFinished(DA::DAWorkFlowGraphicsScene::MouseActionFlag mf);
private slots:
#if DA_USE_QGRAPHICSOBJECT
    void backgroundPixmapItemXChanged();
    void backgroundPixmapItemYChanged();
    void backgroundPixmapItemBodyItemBodySizeChanged(const QSizeF& oldsize, const QSizeF& newsize);
#endif
private:
    DAGraphicsResizeablePixmapItem* _backgroundPixmapItem;
    MouseActionFlag _mouseAction;          ///< 鼠标动作
    bool _isMouseActionContinuoue;         ///<鼠标动作是否连续执行
    bool m_enableItemMoveWithBackground;   ///< 允许item跟随背景图移动
    QColor m_textColor;                    ///< 文本的颜色
    QFont m_textFont;                      ///<文本的字体
    QPointF _backgroundPixmapItemLastPos;  //背景图移动前的坐标
    QPointF _pixmapResizeLastPos;          //背景图拖动缩放前的坐标
};
}  // end of namespace DA
#endif  // GGRAPHICSSCENE_H
