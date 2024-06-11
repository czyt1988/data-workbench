#include "DAWorkFlowEditWidget.h"
#include "ui_DAWorkFlowEditWidget.h"
// qt
#include <QUrl>
#include <QImage>
#include <QDebug>
#include <QUndoStack>
#include <QColor>
#include <QList>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include "DAGraphicsTextItem.h"
// workflow
#include "DAWorkFlowGraphicsView.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsPixmapItem.h"
//
#include "Commands/DACommandsForWorkFlow.h"
#include "DAXmlHelper.h"
namespace DA
{

DAWorkFlowEditWidget::DAWorkFlowEditWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAWorkFlowEditWidget), mScene(nullptr)
{
	ui->setupUi(this);
	createScene();
}

DAWorkFlowEditWidget::~DAWorkFlowEditWidget()
{
	qDebug() << "destroy DAWorkFlowEditWidget";
	delete ui;
}

/**
 * @brief 获取工作流
 * @return
 */
DAWorkFlow* DAWorkFlowEditWidget::getWorkflow() const
{
    return ui->workflowGraphicsView->getWorkflow();
}

/**
 * @brief DAWorkFlowEditWidget::setWorkFlow
 * @param w
 */
void DAWorkFlowEditWidget::setWorkFlow(DAWorkFlow* w)
{
	ui->workflowGraphicsView->setWorkFlow(w);
	connect(w, &DAWorkFlow::startExecute, this, &DAWorkFlowEditWidget::startExecute);
	connect(w, &DAWorkFlow::nodeExecuteFinished, this, &DAWorkFlowEditWidget::nodeExecuteFinished);
	connect(w, &DAWorkFlow::finished, this, &DAWorkFlowEditWidget::finished);
}

DAWorkFlowGraphicsView* DAWorkFlowEditWidget::getWorkFlowGraphicsView() const
{
    return ui->workflowGraphicsView;
}

/**
 * @brief 获取场景
 *
 * 每个DAWorkFlowEditWidget必定有一个场景，但可能有多个view
 * @return
 */
DAWorkFlowGraphicsScene* DAWorkFlowEditWidget::getWorkFlowGraphicsScene() const
{
	return mScene;
}

void DAWorkFlowEditWidget::setUndoStackActive()
{
	getWorkFlowGraphicsView()->setUndoStackActive();
}

void DAWorkFlowEditWidget::setEnableShowGrid(bool on)
{
	DAWorkFlowGraphicsScene* scene = getWorkFlowGraphicsScene();
	if (scene) {
		scene->showGridLine(on);
		scene->update();
	}
}

QUndoStack* DAWorkFlowEditWidget::getUndoStack()
{
	return getWorkFlowGraphicsView()->getUndoStack();
}

void DAWorkFlowEditWidget::runWorkFlow()
{
	DAWorkFlow* wf = ui->workflowGraphicsView->getWorkflow();
	if (nullptr == wf) {
		qCritical() << tr("no workflow set");
		return;
	}
	wf->exec();
}

void DAWorkFlowEditWidget::setMouseActionFlag(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continous)
{
	auto sc = getWorkFlowGraphicsScene();
	if (sc) {
		sc->setMouseAction(mf, continous);
	}
}

void DAWorkFlowEditWidget::addBackgroundPixmap(const QString& pixmapPath)
{
	auto sc = getWorkFlowGraphicsScene();
	if (!sc) {
		return;
	}

	QImage img(pixmapPath);
	QPixmap px;
	px.convertFromImage(img);
	DAGraphicsPixmapItem* item = sc->setBackgroundPixmap(px);
	item->setSelectable(true);
	item->setMoveable(true);
	// connect(item, &DAGraphicsPixmapItem::itemPosChange, this, &DAWorkFlowOperateWidget::onItemPosChange);
}

void DAWorkFlowEditWidget::setBackgroundPixmapLock(bool on)
{
	auto sc = getWorkFlowGraphicsScene();
	if (!sc) {
		return;
	}
	DAGraphicsPixmapItem* item = sc->getBackgroundPixmapItem();
	if (nullptr == item) {
		return;
	}
	item->setSelectable(!on);
	item->setMoveable(!on);
}

void DAWorkFlowEditWidget::setSelectTextToBold(bool on)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextBold(on);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextBold(on);
	}
}

void DAWorkFlowEditWidget::setSelectTextToItalic(bool on)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextItalic(on);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextItalic(on);
	}
}

void DAWorkFlowEditWidget::setSelectTextColor(const QColor& color)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextColor(color);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextColor(color);
	}
}

void DAWorkFlowEditWidget::setSelectTextFamily(const QString& family)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextFamily(family);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextFamily(family);
	}
}

void DAWorkFlowEditWidget::setSelectTextPointSize(const int size)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextPointSize(size);
	}
	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextPointSize(size);
	}
}

/**
 * @brief 设置选中的textitem的字体
 *
 * @note 此操作自带redo/undo，DAGraphicsStandardTextItem会自动把命令放入scene的undo stack中
 * @param f
 */
void DAWorkFlowEditWidget::setSelectTextItemFont(const QFont& f)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextFont(f);
	}
	auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextFont(f);
	}
}

/**
 * @brief 设置当前选中图元的背景
 *
 * @note 支持redo/undo
 * @param b
 */
void DAWorkFlowEditWidget::setSelectShapeBackgroundBrush(const QBrush& b)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	QList< DAGraphicsItem* > items = getSelectDAItems();
	if (items.isEmpty()) {
		return;
	}
	auto cmd = new DA::DACommandGraphicsShapeBackgroundBrushChange(items, b);
	secen->push(cmd);
}
/**
 * @brief 设置当前选中图元的边框
 * @param v
 */
void DAWorkFlowEditWidget::setSelectShapeBorderPen(const QPen& v)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	QList< DAGraphicsItem* > items = getSelectDAItems();
	if (items.isEmpty()) {
		return;
	}
	DA::DACommandGraphicsShapeBorderPenChange* cmd = new DA::DACommandGraphicsShapeBorderPenChange(items, v);
	secen->push(cmd);
}

/**
 * @brief 全选
 */
void DAWorkFlowEditWidget::selectAll()
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	secen->selectAll();
}

/**
 * @brief 取消选中
 */
void DAWorkFlowEditWidget::clearSelection()
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	secen->clearSelection();
}

/**
 * @brief 复制当前选中的items
 */
void DAWorkFlowEditWidget::copySelectItems()
{
	qDebug() << "DAWorkFlowEditWidget::copySelectItems";
	QList< DAGraphicsItem* > its = ui->workflowGraphicsView->selectedDAItems();
	copyItems(its, true);
}

/**
 * @brief 复制条目
 * @param its
 * @param isCopy 为true代表是复制，为false代表是剪切
 */
void DAWorkFlowEditWidget::copyItems(QList< DAGraphicsItem* > its, bool isCopy)
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
void DAWorkFlowEditWidget::cutSelectItems()
{
	qDebug() << "DAWorkFlowEditWidget::cutSelectItems";
	QList< DAGraphicsItem* > its = ui->workflowGraphicsView->selectedDAItems();
	copyItems(its, false);
	// 复制完成后要删除
	auto scene = getWorkFlowGraphicsScene();
	getUndoStack()->beginMacro(tr("cut"));
	for (DAGraphicsItem* i : qAsConst(its)) {
		scene->removeItem_(i);
	}
	getUndoStack()->endMacro();
}

/**
 * @brief 粘贴动作，把目标粘贴到view中心区域
 */
void DAWorkFlowEditWidget::paste(PasteMode mode)
{
	//! 首先获取剪切板信息
	QClipboard* clipboard = QApplication::clipboard();
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
		if (!xml.loadClipBoardElement(&rootEle, this)) {
			qInfo() << tr("An exception occurred during the process of parsing and pasting content");  // cn:解析粘贴内容过程出现异常
			return;
		}
		//!把原来选中的取消选中，把粘贴的选中
		clearSelection();
		QList< QGraphicsItem* > loadedItems = xml.getAllDealItems();
		setSelectionState(loadedItems, true);
	} else if (mimeData->hasImage()) {
		// 粘贴图片
		qDebug() << "clipboard paste Image";
		QImage image = qvariant_cast< QImage >(mimeData->imageData());
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
		} else {
			// 单纯复制文本，直接生成一个文本框
		}
	}
}

/**
 * @brief 移除选中的条目
 */
void DAWorkFlowEditWidget::removeSelectItems()
{
    getWorkFlowGraphicsScene()->removeSelectedItems_();
}

/**
 * @brief 执行取消动作
 */
void DAWorkFlowEditWidget::cancel()
{
	DANodeGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		if (sc->isStartLink()) {
			sc->cancelLink();
		} else {
			// 不在连线状态按下esc，就取消选择
			sc->clearSelection();
		}
	}
}

QFont DAWorkFlowEditWidget::getDefaultTextFont() const
{
	return getWorkFlowGraphicsScene()->getDefaultTextFont();
}

void DAWorkFlowEditWidget::setDefaultTextFont(const QFont& f)
{
	getWorkFlowGraphicsScene()->setDefaultTextFont(f);
}

QColor DAWorkFlowEditWidget::getDefaultTextColor() const
{
	return getWorkFlowGraphicsScene()->getDefaultTextColor();
}

void DAWorkFlowEditWidget::setDefaultTextColor(const QColor& c)
{
    getWorkFlowGraphicsScene()->setDefaultTextColor(c);
}

/**
 * @brief 获取选中的文本
 * @return
 */
QList< DAGraphicsStandardTextItem* > DAWorkFlowEditWidget::getSelectStandardTextItems()
{
	QList< DAGraphicsStandardTextItem* > res;
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return res;
	}
	QList< QGraphicsItem* > its = secen->selectedItems();
	if (its.size() == 0) {
		return res;
	}
	for (QGraphicsItem* item : qAsConst(its)) {
		if (DAGraphicsStandardTextItem* textItem = dynamic_cast< DAGraphicsStandardTextItem* >(item)) {
			res.append(textItem);
		}
	}
	return res;
}

/**
 * @brief getSelectTextItems
 * @return
 */
QList< DAGraphicsTextItem* > DAWorkFlowEditWidget::getSelectTextItems()
{
	QList< DAGraphicsTextItem* > res;
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return res;
	}
	const QList< QGraphicsItem* > its = secen->selectedItems();
	if (its.size() == 0) {
		return res;
	}
	for (QGraphicsItem* item : its) {
		if (DAGraphicsTextItem* textItem = dynamic_cast< DAGraphicsTextItem* >(item)) {
			res.append(textItem);
		}
	}
	return res;
}

/**
 * @brief 获取选中的基本图元
 * @return
 */
QList< DAGraphicsItem* > DAWorkFlowEditWidget::getSelectDAItems()
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return QList< DAGraphicsItem* >();
	}
	return secen->selectedDAItems();
}

void DAWorkFlowEditWidget::createScene()
{
	mScene = new DAWorkFlowGraphicsScene(this);
	ui->workflowGraphicsView->setScene(mScene);
	//    connect(_scene, &DAWorkFlowGraphicsScene::selectNodeItemChanged, this, [ this ](DAGraphicsItem* i) {
	//        if (DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
	//            emit selectNodeItemChanged(ni);
	//        }
	//    });

	connect(mScene, &DAWorkFlowGraphicsScene::selectNodeItemChanged, this, &DAWorkFlowEditWidget::selectNodeItemChanged);
	connect(mScene, &DAWorkFlowGraphicsScene::mouseActionFinished, this, &DAWorkFlowEditWidget::mouseActionFinished);
}

void DAWorkFlowEditWidget::setSelectionState(const QList< QGraphicsItem* >& items, bool isSelect)
{
	auto scene = getWorkFlowGraphicsScene();
	if (!scene) {
		return;
	}
	scene->setSelectionState(items, isSelect);
}

}  // end of DA
