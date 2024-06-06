#include "DAWorkFlowGraphicsView.h"
#include <QDragEnterEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QDomDocument>
#include <QDomElement>
#include "DAAbstractNode.h"
#include "DANodeMetaData.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAWorkFlow.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAGraphicsItemGroup.h"
#include "DAGraphicsPixmapItem.h"
#include "DAXmlHelper.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAWorkFlowGraphicsView
//===================================================
DAWorkFlowGraphicsView::DAWorkFlowGraphicsView(QWidget* parent) : DAGraphicsView(parent)
{
	setAcceptDrops(true);
	centerOn(0, 0);
	setRenderHint(QPainter::Antialiasing, true);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

DAWorkFlowGraphicsView::DAWorkFlowGraphicsView(QGraphicsScene* scene, QWidget* parent) : DAGraphicsView(scene, parent)
{
	setAcceptDrops(true);
	centerOn(0, 0);
	setRenderHint(QPainter::Antialiasing, true);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

DAWorkFlowGraphicsView::~DAWorkFlowGraphicsView()
{
	//    DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	//    if (sc) {
	//        sc->clear();
	//    }
	//    qDebug() << "destory DAWorkFlowGraphicsView";
}

/**
 * @brief DAWorkFlowGraphicsView不负责workflow的所有权
 * @param wf
 */
void DAWorkFlowGraphicsView::setWorkFlow(DAWorkFlow* wf)
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		return sc->setWorkFlow(wf);
	}
}

/**
 * @brief DAWorkFlowGraphicsView::workflow
 * @return
 */
DAWorkFlow* DAWorkFlowGraphicsView::getWorkflow()
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		return (sc->getWorkflow());
	}
	return nullptr;
}

void DAWorkFlowGraphicsView::setUndoStackActive()
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		sc->setUndoStackActive();
	}
}

QUndoStack* DAWorkFlowGraphicsView::getUndoStack()
{
	DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {

		return &(sc->undoStack());
	}
	return nullptr;
}

/**
 * @brief 获取DAWorkFlowGraphicsScene
 * @return
 */
DAWorkFlowGraphicsScene* DAWorkFlowGraphicsView::getWorkFlowGraphicsScene()
{
    return qobject_cast< DAWorkFlowGraphicsScene* >(scene());
}

/**
 * @brief 获取当前view视图下的scene中心
 * @return
 */
QPointF DAWorkFlowGraphicsView::getViewCenterMapToScene() const
{
    auto r = viewport()->rect().center();
    return mapToScene(r);
}

/**
 * @brief 把item移动到屏幕中心
 * @param item
 */
void DAWorkFlowGraphicsView::moveItemToViewCenter(QGraphicsItem* item)
{
    QPointF c = getViewCenterMapToScene();
    auto br   = item->boundingRect();
    c.rx() -= (br.width() / 2);
    c.ry() -= (br.height() / 2);
    item->setPos(c);
}

void DAWorkFlowGraphicsView::copySelectItems()
{
    QList< DAGraphicsItem* > its = selectedDAItems();
    copyItems(its, true);
}

void DAWorkFlowGraphicsView::copyItems(const QList< DAGraphicsItem* >& its, bool isCopy)
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
        qDebug().noquote() << "copy items,da-xml:" << doc.toString();
    }
}

/**
 * @brief 剪切
 */
void DAWorkFlowGraphicsView::cutSelectItems()
{
    qDebug() << "DAWorkFlowEditWidget::cutSelectItems";
    QList< DAGraphicsItem* > its = selectedDAItems();
    copyItems(its, false);
    // 复制完成后要删除
    auto scene = getWorkFlowGraphicsScene();
    getUndoStack()->beginMacro(tr("cut"));
    for (DAGraphicsItem* i : qAsConst(its)) {
        scene->removeItem_(i);
    }
    getUndoStack()->endMacro();
}

void DAWorkFlowGraphicsView::paste(DAWorkFlowGraphicsView::PasteMode mode)
{
    switch (mode) {
    case PaseteRangeCenterToViewCenter:
        pasteToViewCenter();
        break;
    case PaseteRangeCenterToCursor:
        break;
    default:
        break;
    }
}

/**
 * @brief 设置item的选中状态
 * @param items
 * @param isSelect
 */
void DAWorkFlowGraphicsView::setSelectionState(const QList< QGraphicsItem* >& items, bool isSelect)
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
void DAWorkFlowGraphicsView::clearSelection()
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
void DAWorkFlowGraphicsView::selectAll()
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
QRectF DAWorkFlowGraphicsView::calcItemsSceneRange(const QList< QGraphicsItem* >& its)
{
    if (its.empty()) {
        return QRectF();
    }
    QRectF range = its.first()->sceneBoundingRect();
    for (int i = 1; i < its.size(); ++i) {
        range.united(its[ i ]->sceneBoundingRect());
    }
    return range;
}

QList< QGraphicsItem* > DAWorkFlowGraphicsView::cast(const QList< DAGraphicsItem* >& its)
{
    QList< QGraphicsItem* > res;
    res.reserve(its.size());
    for (DAGraphicsItem* i : its) {
        res.append(static_cast< QGraphicsItem* >(i));
    }
    return res;
}

void DAWorkFlowGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    DAGraphicsView::mouseMoveEvent(event);
}

void DAWorkFlowGraphicsView::mousePressEvent(QMouseEvent* event)
{
    DAGraphicsView::mousePressEvent(event);
}

void DAWorkFlowGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    DAGraphicsView::mouseReleaseEvent(event);
}

void DAWorkFlowGraphicsView::keyPressEvent(QKeyEvent* event)
{
    DAGraphicsView::keyPressEvent(event);
}

void DAWorkFlowGraphicsView::keyReleaseEvent(QKeyEvent* event)
{
    DAGraphicsView::keyReleaseEvent(event);
}

void DAWorkFlowGraphicsView::pasteToViewCenter()
{
    DAWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
    //! 首先获取剪切板信息
    QPointF viewCenterPoint = getViewCenterMapToScene();
    QClipboard* clipboard   = QApplication::clipboard();
    if (!clipboard) {
        qDebug() << "can not get clipboard";
        return;
    }
    QDomDocument doc;
    const QMimeData* mimeData = clipboard->mimeData();

    // 优先判断是否有text/da-xml
    QByteArray daxmlData = mimeData->data(QStringLiteral("text/da-xml"));
    if (daxmlData.size() > 0) {
        qDebug() << "clipboard paste text/da-xml";
        if (!doc.setContent(daxmlData)) {
            qInfo() << tr("Unrecognized mime formats:%1,paste failed").arg(mimeData->formats().join(","));  // cn:无法识别的mime类型:%1,粘贴失败
            return;
        }
        QDomElement rootEle = doc.firstChildElement(QStringLiteral("da-clip"));
        if (rootEle.isNull()) {
            qInfo() << tr("Unsupported pasted content");  // cn:不支持的粘贴内容
            return;
        }
        DAXmlHelper xml;
        if (!xml.loadClipBoardElement(&rootEle, sc)) {
            qInfo() << tr("An exception occurred during the process of parsing and pasting content");  // cn:解析粘贴内容过程出现异常
            return;
        }
        //!把原来选中的取消选中，把粘贴的选中
        clearSelection();
        const auto loadedItems = xml.getAllDealItems();
        QRectF range           = calcItemsSceneRange(loadedItems);
        // 计算偏移
        QPointF offset = range.center() - viewCenterPoint;
        for (QGraphicsItem* i : loadedItems) {
            if (DAGraphicsLinkItem* link = dynamic_cast< DAGraphicsLinkItem* >(i)) {
                // 连接线不进行偏移，自动调整
                continue;
            }
            if (i) {
                i->setPos(i->pos() - offset);
            }
        }
        // 进行偏移让所有item回到视图中心
        setSelectionState(loadedItems, true);
    } else if (mimeData->hasImage()) {
        // 粘贴图片
        qDebug() << "clipboard paste Image";
        QImage image = qvariant_cast< QImage >(mimeData->imageData());
        if (image.isNull()) {
            return;
        }
        if (DAGraphicsPixmapItem* pixmapItem = sc->addPixmapItem_(image)) {
            moveItemToViewCenter(pixmapItem);
        }
    } else if (mimeData->hasText()) {
        // 粘贴文本
        QString textData = mimeData->text();
        qDebug() << "clipboard paste Text:" << textData;
        // 有可能选中了多个文件,多个文件会用/n分割，这里不处理
        QUrl url(textData);
        if (url.isValid() && url.scheme() == QStringLiteral("file")) {
            // 转换为本地文件路径
            QString filePath = url.toLocalFile();
            qDebug() << "clipboard paste local file:" << filePath;
            // QFileInfo fi(filePath);
            //! 1.首先判断是否是project工程，如果是工程的话，直接把工程复制进来
            // TODO

            //! 2.如果不是工程，判断是否是图片
            if (DAGraphicsPixmapItem* pixmapItem = sc->addPixmapItem_(QImage(filePath))) {
                moveItemToViewCenter(pixmapItem);
            }

        } else {
            // 单纯复制文本，直接生成一个文本框
        }
    }
}

}  // end DA
