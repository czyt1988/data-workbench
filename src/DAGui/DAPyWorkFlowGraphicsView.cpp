#include "DAPyWorkFlowGraphicsView.h"
#include <QDragEnterEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QDomDocument>
#include <QDomElement>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include "DAPyNodeProxy.h"
#include "DANodeMimeData.h"
#include "DAPyNodeFactory.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include "DAPyWorkFlow.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAGraphicsItemGroup.h"
#include "DAXmlHelper.h"
#include "DAGraphicsPixmapItem.h"
#include "da_qt5qt6_compat.hpp"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

class DAPyWorkFlowGraphicsView::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowGraphicsView)
public:
    PrivateData(DAPyWorkFlowGraphicsView* p);
    void beginDragCopy();
    void endDragCopy(const QPoint& releasePoint);
    bool isDragingCopy() const;

public:
    bool mCtrlPressed { false };             ///< 记录ctrl是否按下
    bool mMouseLeftButtonPressed { false };  ///< 记录鼠标左键是否按下
    bool mStartDragCopy { false };           ///< 标记 开始拖拽复制
    QPoint mMouseLeftButtonPressedPos;       ///< 记录ctrl按下后鼠标点击的位置
    /**
     *@brief 被拖入的节点,这个是记录节点被拖入的指针，这个指针不为空，代表这个拖入是拖入到节点上
     */
    DAPyNodeGraphicsItem* mDragInNodeItem { nullptr };
};
DAPyWorkFlowGraphicsView::PrivateData::PrivateData(DAPyWorkFlowGraphicsView* p) : q_ptr(p)
{
}

void DAPyWorkFlowGraphicsView::PrivateData::beginDragCopy()
{
    mStartDragCopy = true;
    q_ptr->copySelectItems();
}

void DAPyWorkFlowGraphicsView::PrivateData::endDragCopy(const QPoint& releasePoint)
{
    mStartDragCopy = false;
    // 计算偏移量
    QPointF startScenePos = q_ptr->mapToScene(mMouseLeftButtonPressedPos);
    QPointF endScenePos   = q_ptr->mapToScene(releasePoint);
    QPointF offset        = endScenePos - startScenePos;
    q_ptr->pasteByOffset(offset);
}

bool DAPyWorkFlowGraphicsView::PrivateData::isDragingCopy() const
{
    return mStartDragCopy;
}
//===================================================
// DAPyWorkFlowGraphicsView
//===================================================
DAPyWorkFlowGraphicsView::DAPyWorkFlowGraphicsView(QWidget* parent) : DAGraphicsView(parent), DA_PIMPL_CONSTRUCT
{
    setAcceptDrops(true);
    centerOn(0, 0);
    setRenderHint(QPainter::Antialiasing, true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

DAPyWorkFlowGraphicsView::DAPyWorkFlowGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : DAGraphicsView(scene, parent), DA_PIMPL_CONSTRUCT
{
    setAcceptDrops(true);
    centerOn(0, 0);
    setRenderHint(QPainter::Antialiasing, true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

DAPyWorkFlowGraphicsView::~DAPyWorkFlowGraphicsView()
{
    //    DAPyWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
    //    if (sc) {
    //        sc->clear();
    //    }
    //    qDebug() << "destory DAPyWorkFlowGraphicsView";
}

/**
 * @brief DAPyWorkFlowGraphicsView不负责workflow的所有权
 * @param wf
 */
void DAPyWorkFlowGraphicsView::setWorkFlow(DAPyWorkFlow* wf)
{
    // TODO: DAPyWorkFlowScene uses setPyWorkflow(const pybind11::object&)
    // Once DAPyWorkFlow provides a pybind11::object accessor, delegate to scene
    Q_UNUSED(wf);
}

/**
 * @brief DAPyWorkFlowGraphicsView::workflow
 * @return
 */
DAPyWorkFlow* DAPyWorkFlowGraphicsView::getWorkflow()
{
    // TODO: DAPyWorkFlowScene uses getPyWorkflow() returning pybind11::object
    // Once DAPyWorkFlow provides conversion, delegate to scene
    return nullptr;
}

void DAPyWorkFlowGraphicsView::setUndoStackActive()
{
    DAPyWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
    if (sc) {
        sc->setUndoStackActive();
    }
}

QUndoStack* DAPyWorkFlowGraphicsView::getUndoStack()
{
    DAPyWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
    if (sc) {

        return &(sc->undoStack());
    }
    return nullptr;
}

/**
 * @brief 获取DAPyWorkFlowGraphicsScene
 * @return
 */
DAPyWorkFlowGraphicsScene* DAPyWorkFlowGraphicsView::getWorkFlowGraphicsScene()
{
    return qobject_cast< DAPyWorkFlowGraphicsScene* >(scene());
}

/**
 * @brief 获取当前view视图下的scene中心
 * @return
 */
QPointF DAPyWorkFlowGraphicsView::getViewCenterMapToScene() const
{
    auto r = viewport()->rect().center();
    return mapToScene(r);
}

/**
 * @brief 把item移动到屏幕中心
 * @param item
 */
void DAPyWorkFlowGraphicsView::moveItemToViewCenter(QGraphicsItem* item)
{
    QPointF c = getViewCenterMapToScene();
    auto br   = item->boundingRect();
    c.rx() -= (br.width() / 2);
    c.ry() -= (br.height() / 2);
    item->setPos(c);
}

/**
 * @brief 复制选中条目，如果没有选中返回false
 * @return
 */
bool DAPyWorkFlowGraphicsView::copySelectItems()
{
    QList< DAGraphicsItem* > its = selectedDAItems();
    if (its.size() <= 0) {
        return false;
    }
    copyItems(its, true);
    return true;
}

void DAPyWorkFlowGraphicsView::copyItems(const QList< DAGraphicsItem* >& its, bool isCopy)
{
    DAXmlHelper xml;
    QDomDocument doc;
    QDomElement rootEle = xml.makeClipBoardElement(its, QStringLiteral("da-clip"), &doc, isCopy);
    doc.appendChild(rootEle);
    QMimeData* mimeData = new QMimeData;
    mimeData->setData(QStringLiteral("text/da-xml"), doc.toByteArray());
    QClipboard* clipboard = QApplication::clipboard();
    if (clipboard) {
        clipboard->setMimeData(mimeData);
        //		qDebug().noquote() << "copy items,da-xml:" << doc.toString();
    }
}

/**
 * @brief 剪切
 */
void DAPyWorkFlowGraphicsView::cutSelectItems()
{
    //	qDebug() << "DAPyWorkFlowEditWidget::cutSelectItems";
    QList< DAGraphicsItem* > its = selectedDAItems();
    copyItems(its, false);
    // 复制完成后要删除
    auto scene = getWorkFlowGraphicsScene();
    getUndoStack()->beginMacro(tr("cut"));
    for (DAGraphicsItem* i : std::as_const(its)) {
        if (DAPyNodeGraphicsItem* ni = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
            scene->removePyNodeItem_(ni);
        } else {
            scene->removeItem_(i);
        }
    }
    getUndoStack()->endMacro();
}

QList< QGraphicsItem* > DAPyWorkFlowGraphicsView::paste()
{
    QList< QGraphicsItem* > res;
    DAPyWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
    //! 首先获取剪切板信息
    QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard) {
        qDebug() << "can not get clipboard";
        return res;
    }
    QDomDocument doc;
    const QMimeData* mimeData = clipboard->mimeData();

    // 优先判断是否有text/da-xml
    QByteArray daxmlData = mimeData->data(QStringLiteral("text/da-xml"));
    if (daxmlData.size() > 0) {
        //		qDebug() << "clipboard paste text/da-xml";
        if (!doc.setContent(daxmlData)) {
            qInfo() << tr("Unrecognized mime formats:%1,paste failed").arg(mimeData->formats().join(","));  // cn:无法识别的mime类型:%1,粘贴失败
            return res;
        }
        QDomElement rootEle = doc.firstChildElement(QStringLiteral("da-clip"));
        if (rootEle.isNull()) {
            qInfo() << tr("Unsupported pasted content");  // cn:不支持的粘贴内容
            return res;
        }
        DAXmlHelper xml;
        if (!xml.loadClipBoardElement(&rootEle, sc)) {
            qInfo() << tr("An exception occurred during the process of parsing and pasting content");  // cn:解析粘贴内容过程出现异常
            return res;
        }
        //!把原来选中的取消选中，把粘贴的选中
        clearSelection();
        res = xml.getAllDealItems();
        setSelectionState(res, true);
    } else if (mimeData->hasImage()) {
        // 粘贴图片
        //		qDebug() << "clipboard paste Image";
        QImage image = qvariant_cast< QImage >(mimeData->imageData());
        if (image.isNull()) {
            return res;
        }
        DAGraphicsPixmapItem* pixmapItem = new DAGraphicsPixmapItem();
        pixmapItem->setPixmap(QPixmap::fromImage(image));
        sc->addItem_(pixmapItem);
        if (pixmapItem) {
            res.append(pixmapItem);
        }
    } else if (mimeData->hasText()) {
        // 粘贴文本
        QString textData = mimeData->text();
        //		qDebug() << "clipboard paste Text:" << textData;
        // 有可能选中了多个文件,多个文件会用/n分割，这里不处理
        QUrl url(textData);
        if (url.isValid() && url.scheme() == QStringLiteral("file")) {
            // 转换为本地文件路径
            QString filePath = url.toLocalFile();
            //			qDebug() << "clipboard paste local file:" << filePath;
            // QFileInfo fi(filePath);
            //! 1.首先判断是否是project工程，如果是工程的话，直接把工程复制进来
            // TODO

            //! 2.如果不是工程，判断是否是图片
            auto pixmapItem = new DAGraphicsPixmapItem();
            pixmapItem->setPixmap(QPixmap::fromImage(QImage(filePath)));
            sc->addItem_(pixmapItem);
            if (pixmapItem) {
                res.append(pixmapItem);
            }
        } else {
            // 单纯复制文本，直接生成一个文本框
        }
    }
    Q_EMIT pastedItems(res);
    return res;
}

void DAPyWorkFlowGraphicsView::pasteToViewCenter()
{
    const QList< QGraphicsItem* > loadedItems = paste();
    QPointF viewCenterPoint                   = getViewCenterMapToScene();
    QRectF range                              = calcItemsSceneRange(loadedItems);
    // 计算偏移
    QPointF offset = viewCenterPoint - range.center();
    offsetItems(loadedItems, offset);
}

void DAPyWorkFlowGraphicsView::pasteByOffset(const QPointF& offset)
{
    const QList< QGraphicsItem* > loadedItems = paste();
    // 进行偏移
    offsetItems(loadedItems, offset);
}

/**
 * @brief 设置item的选中状态
 * @param items
 * @param isSelect
 */
void DAPyWorkFlowGraphicsView::setSelectionState(const QList< QGraphicsItem* >& items, bool isSelect)
{
    auto scene = getWorkFlowGraphicsScene();
    if (!scene) {
        return;
    }
    scene->setSelectionState(items, isSelect);
}

/**
 * @brief 取消选中
 */
void DAPyWorkFlowGraphicsView::clearSelection()
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return;
    }
    secen->clearSelection();
}

/**
 * @brief 全选
 */
void DAPyWorkFlowGraphicsView::selectAll()
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return;
    }
    secen->selectAll();
}

/**
 * @brief 计算item所包含的范围，这个范围存入xml中，以便让scene第一时间知道总体范围
 * @param its
 * @return
 */
QRectF DAPyWorkFlowGraphicsView::calcItemsSceneRange(const QList< QGraphicsItem* >& its)
{
    if (its.empty()) {
        return QRectF();
    }
    QRectF range = its.first()->sceneBoundingRect();
    for (int i = 1; i < its.size(); ++i) {
        range = range.united(its[ i ]->sceneBoundingRect());
    }
    return range;
}

QList< QGraphicsItem* > DAPyWorkFlowGraphicsView::cast(const QList< DAGraphicsItem* >& its)
{
    QList< QGraphicsItem* > res;
    res.reserve(its.size());
    for (DAGraphicsItem* i : its) {
        res.append(static_cast< QGraphicsItem* >(i));
    }
    return res;
}

/**
 * @brief 所有item偏移一个距离,连接线不进行偏移
 * @param its
 * @param offset
 */
void DAPyWorkFlowGraphicsView::offsetItems(const QList< QGraphicsItem* >& its, const QPointF& offset)
{
    // 进行偏移让所有item回到视图中心
    for (QGraphicsItem* i : its) {
        if (DAGraphicsLinkItem* link = dynamic_cast< DAGraphicsLinkItem* >(i)) {
            Q_UNUSED(link);
            // 连接线不进行偏移，自动调整
            continue;
        }
        if (i) {
            i->setPos(i->pos() + offset);
        }
    }
}

/**
 * @brief 创建节点（不带回退功能）
 * @note 注意，节点会记录在工作流中,如果返回的是nullptr，则不会记录
 * @param md
 * @param viewPos view的位置
 * @return
 */
DAPyNodeGraphicsItem* DAPyWorkFlowGraphicsView::createNode(const DAPyNodeMetaData& md, const QPoint& viewPos)
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return nullptr;
    }
    return secen->createPyNode(md, mapToScene(viewPos));
}

DAPyNodeGraphicsItem* DAPyWorkFlowGraphicsView::createNode_(const DAPyNodeMetaData& md, const QPoint& viewPos)
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return nullptr;
    }
    return secen->createPyNode_(md, mapToScene(viewPos));
}

void DAPyWorkFlowGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (!isPadding()) {

        // ctrl按下，移动的不下发到scene
        if (d_ptr->mCtrlPressed && d_ptr->mMouseLeftButtonPressed) {
            QPoint pos = event->pos();
            pos -= d_ptr->mMouseLeftButtonPressedPos;
            // 按下了ctrl，且移动距离大于系统拖拽距离
            if (pos.manhattanLength() > QApplication::startDragDistance()) {
                setCursor(Qt::DragCopyCursor);
                // copySelectItems在beginDragCopy中调用
                d_ptr->beginDragCopy();
                return;
            } else {
                event->accept();
                return;
            }
        }
    }

    DAGraphicsView::mouseMoveEvent(event);
}

void DAPyWorkFlowGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        d_ptr->mMouseLeftButtonPressedPos = event->pos();
        d_ptr->mMouseLeftButtonPressed    = true;
    }
    DAGraphicsView::mousePressEvent(event);
}

void DAPyWorkFlowGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        d_ptr->mMouseLeftButtonPressed = false;
        resetCursor();
        if (d_ptr->isDragingCopy()) {
            d_ptr->endDragCopy(event->pos());
        }
    }
    DAGraphicsView::mouseReleaseEvent(event);
}

void DAPyWorkFlowGraphicsView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control) {
        d_ptr->mCtrlPressed = true;
    }
    DAGraphicsView::keyPressEvent(event);
}

void DAPyWorkFlowGraphicsView::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control) {
        d_ptr->mCtrlPressed = false;
        resetCursor();
    }
    DAGraphicsView::keyReleaseEvent(event);
}

void DAPyWorkFlowGraphicsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (d_ptr->mDragInNodeItem) {
        d_ptr->mDragInNodeItem = nullptr;
    }
    if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
        // 说明有节点的meta数据拖入
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
    DAGraphicsView::dragEnterEvent(event);
}

void DAPyWorkFlowGraphicsView::dragMoveEvent(QDragMoveEvent* event)
{
    // 嵌套过深的优化：
    //  auto sc = getWorkFlowGraphicsScene();
    //  if (sc) {
    //  	QGraphicsItem* it = itemAt(event->pos());
    //  	if (it) {
    //  		if(DAPyNodeGraphicsItem* nodeItem = dynamic_cast<DAPyNodeGraphicsItem*>(it)){
    //  			if(nodeItem->acceptDragOn())
    //  		}
    //  	}
    //  }
    //  event->acceptProposedAction();

    do {
        auto sc = getWorkFlowGraphicsScene();
        if (!sc) {
            break;
        }
        auto viewPos      = DA::compat::eventPos(event);
        QGraphicsItem* it = itemAt(viewPos);
        if (!it) {
            break;
        }
        DAPyNodeGraphicsItem* nodeItem = dynamic_cast< DAPyNodeGraphicsItem* >(it);
        if (!nodeItem) {
            break;
        }
        // 说明是节点
        const DANodeMimeData* nodemime = qobject_cast< const DANodeMimeData* >(event->mimeData());
        if (nullptr == nodemime) {
            break;
        }
        DAPyNodeMetaData nodemeta = nodemime->getNodeMetaData();
        auto scenePos             = mapToScene(viewPos);
        // TODO: DAPyNodeGraphicsItem does not have acceptDragOn() method.
        // The old DAAbstractNodeGraphicsItem supported drag-on acceptance for node-to-node drops.
        // For now, skip node-to-node drag acceptance and always accept at view level.
        // Reimplement when DAPyNodeGraphicsItem gains drag-drop support or handle in scene.
        Q_UNUSED(nodemeta);
        Q_UNUSED(scenePos);
        break;
    } while (false);
    if (d_ptr->mDragInNodeItem) {
        d_ptr->mDragInNodeItem = nullptr;
    }
    event->acceptProposedAction();
}

void DAPyWorkFlowGraphicsView::dragLeaveEvent(QDragLeaveEvent* event)
{
    if (d_ptr->mDragInNodeItem) {
        d_ptr->mDragInNodeItem = nullptr;
    }
    DAGraphicsView::dragLeaveEvent(event);
}

void DAPyWorkFlowGraphicsView::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
        // 说明有节点的meta数据拖入
        const DANodeMimeData* nodemime = qobject_cast< const DANodeMimeData* >(event->mimeData());
        if (nullptr == nodemime) {
            return;
        }
        DAPyNodeMetaData nodemeta = nodemime->getNodeMetaData();
        // 两种情况，一种是节点托入到另外一个节点上面放下，这个会触发节点的drag函数
        auto evpos = DA::compat::eventPos(event);
        if (d_ptr->mDragInNodeItem) {
            // TODO: DAPyNodeGraphicsItem does not have drop() method.
            // The old DAAbstractNodeGraphicsItem supported node-to-node drops to merge/connect nodes.
            // For now, treat all drops as normal node creation (no node-to-node merge).
            // Reimplement when DAPyNodeGraphicsItem gains drop support or handle in scene.
            d_ptr->mDragInNodeItem = nullptr;
        } else {
            // 正常的拖入操作
            clearSelection();
            createNode_(nodemeta, evpos);
        }
    }
    DAGraphicsView::dropEvent(event);
}

void DAPyWorkFlowGraphicsView::resetCursor()
{
    if (cursor().shape() != Qt::ArrowCursor) {
        setCursor(Qt::ArrowCursor);
    }
}

}  // end DA
