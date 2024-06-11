#include "DAGraphicsStandardTextItem.h"
#include <QFont>
#include <QDebug>
#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QDomDocument>
#include <QDomElement>
#include <QTextStream>
#include "DAGraphicsItemFactory.h"
#include "DAGraphicsScene.h"
#include "DACommandsForGraphics.h"
namespace DA
{

//===================================================
// DAStandardGraphicsTextItem
//===================================================
DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(QGraphicsItem* parent) : QGraphicsTextItem(parent)
{
    initItem();
}

DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
	initItem();
	setPlainText(str);
	setFont(f);
}

DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
	initItem();
	setFont(f);
}

DAGraphicsStandardTextItem::~DAGraphicsStandardTextItem()
{
}

void DAGraphicsStandardTextItem::initItem()
{
	union Combine__ {
		uint32_t a;
		void* b;
	};
	Combine__ tmp;
	tmp.b = this;
	mID   = DAGraphicsItemFactory::generateID(tmp.a);
	setDefaultTextColor(Qt::black);  // 设置字体颜色
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
	QTextDocument* doc = document();
	if (doc) {
		doc->clearUndoRedoStacks(QTextDocument::UndoAndRedoStacks);
		doc->setUndoRedoEnabled(false);
	}
}

/**
 * @brief 设置编辑模式
 * @param on
 */
void DAGraphicsStandardTextItem::setEditable(bool on)
{
    setTextInteractionFlags(on ? Qt::TextEditorInteraction : Qt::NoTextInteraction);
}

/**
 * @brief 是否可编辑
 * @return
 */
bool DAGraphicsStandardTextItem::isEditable() const
{
    return textInteractionFlags().testFlag(Qt::TextEditorInteraction);
}

bool DAGraphicsStandardTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	QDomElement textItemEle = doc->createElement("text-info");
	QPointF scPos           = scenePos();
	textItemEle.setAttribute("id", getItemID());
	textItemEle.setAttribute("x", scPos.x());
	textItemEle.setAttribute("y", scPos.y());
	QTextDocument* textDoc = document();
	if (textDoc) {
		QString html = textDoc->toHtml();
		QDomDocument tempDoc;
		tempDoc.setContent(html);
		QDomElement innerHtmlElement = tempDoc.firstChildElement();  //<html>
		textItemEle.appendChild(innerHtmlElement);
	}
	parentElement->appendChild(textItemEle);
	return true;
}

bool DAGraphicsStandardTextItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	QDomElement textItemEle = itemElement->firstChildElement("text-info");
	if (textItemEle.isNull()) {
		return false;
	}
	QPointF pos;
	uint64_t id;
	if (getStringRealValue(textItemEle.attribute("x"), pos.rx())
		&& getStringRealValue(textItemEle.attribute("y"), pos.ry())) {
		setScenePos(pos);
	}
	if (getStringULongLongValue(textItemEle.attribute("id"), id)) {
		setItemID(id);
	}
	QDomElement htmlEle = textItemEle.firstChildElement("html");
	if (!htmlEle.isNull()) {
		QString html;
		QTextStream ss(&html);
		htmlEle.save(ss, 0);
		// 此函数会触发redo/undo
		setHtml(html);
	}
	return true;
}

void DAGraphicsStandardTextItem::setScenePos(const QPointF& p)
{
	setPos(mapToParent(mapFromScene(p)));
}

void DAGraphicsStandardTextItem::setScenePos(qreal x, qreal y)
{
	setScenePos(QPointF(x, y));
}

uint64_t DAGraphicsStandardTextItem::getItemID() const
{
	return mID;
}

void DAGraphicsStandardTextItem::setItemID(uint64_t id)
{
	mID = id;
}

/**
 * @brief 设置选中文本字体，如果没选中，将设置全部
 * @param v
 */
void DAGraphicsStandardTextItem::setSelectTextFamily(const QString& v)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	QTextCharFormat format;
	// 设置文本颜色
	format.setFontFamily(v);
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 获取选中文本字体字体名称，如果无法获取，返回QString()
 * @return
 */
QString DAGraphicsStandardTextItem::getSelectTextFamily() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	QTextCharFormat format = cursor.charFormat();
	if (format.isValid() || format.isEmpty()) {
		return QString();
	}
	return format.fontFamily();
}

/**
 * @brief 设置选中的颜色，如果没有选中对象，尝试全选
 * @param v
 */
void DAGraphicsStandardTextItem::setSelectTextColor(const QColor& v)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	QTextCharFormat format;
	// 设置文本颜色
	format.setForeground(QBrush(v));
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 获取选中的颜色，如果没有选中对象，尝试全选
 * @return
 */
QColor DAGraphicsStandardTextItem::getSelectTextColor() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	QTextCharFormat format = cursor.charFormat();
	if (format.isValid() || format.isEmpty()) {
		return QColor();
	}
	return format.foreground().color();
}

/**
 * @brief 设置选中文本字体，如果没选中，将设置全部
 * @param v
 */
void DAGraphicsStandardTextItem::setSelectTextFont(const QFont& v)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 说明有选中文本
	QTextCharFormat format;
	format.setFont(v);
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 选中文本字体，如果没选中，将设置全部
 * @return
 */
QFont DAGraphicsStandardTextItem::getSelectTextFont() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 获取该位置的字符格式
	QTextCharFormat format = cursor.charFormat();
	if (format.isValid() || format.isEmpty()) {
		return QFont();
	}
	return format.font();
}

/**
 * @brief 设置选中文本的字体大小
 * @param v
 */
void DAGraphicsStandardTextItem::setSelectTextPointSize(int v)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 说明有选中文本
	QTextCharFormat format;
	format.setFontPointSize(v);
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 选中文本的字体大小
 * @return 如果无法获取选中的尺寸，返回-1
 */
int DAGraphicsStandardTextItem::getSelectTextPointSize() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 获取该位置的字符格式
	QTextCharFormat format = cursor.charFormat();
	if (format.isValid() || format.isEmpty()) {
		return -1;
	}
	return format.fontPointSize();
}

/**
 * @brief 文字斜体
 * @param on
 */
void DAGraphicsStandardTextItem::setSelectTextItalic(bool on)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 说明有选中文本
	QTextCharFormat format;
	format.setFontItalic(on);
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 文字斜体
 * @return
 */
bool DAGraphicsStandardTextItem::getSelectTextItalic() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 获取该位置的字符格式
	QTextCharFormat format = cursor.charFormat();
	if (format.isValid() || format.isEmpty()) {
		return false;
	}
	return format.fontItalic();
}

/**
 * @brief 文字粗体
 * @param on
 */
void DAGraphicsStandardTextItem::setSelectTextBold(bool on)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 说明有选中文本
	QTextCharFormat format;
	format.setFontWeight(on ? QFont::Bold : QFont::Normal);
	// 应用格式到选中文本
	cursor.setCharFormat(format);
}

/**
 * @brief 文字粗体
 * @return
 */
bool DAGraphicsStandardTextItem::getSelectTextBold() const
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection()) {
		cursor.select(QTextCursor::Document);
	}
	// 获取该位置的字符格式
	QTextCharFormat format = cursor.charFormat();
	if (format.isValid() || format.isEmpty()) {
		return false;
	}
	auto w = format.fontWeight();
	return w == QFont::Bold;
}

void DAGraphicsStandardTextItem::focusOutEvent(QFocusEvent* focusEvent)
{
	//! 这里不能执行下面这些语句，尤其把选中内容取消，这样会导致一些控件无法选中文本进行修改，
	//! 例如颜色改变按钮，是个弹出的menu，如果失去焦点就情况选中，会把选中擦去，就无法改变局部颜色
	// QTextCursor cursor = textCursor();
	// cursor.clearSelection();
	// cursor.setPosition(QTextCursor::Start);
	// setTextCursor(cursor);
	setTextInteractionFlags(Qt::NoTextInteraction);
	QGraphicsTextItem::focusOutEvent(focusEvent);
}

void DAGraphicsStandardTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		// czy:setTextInteractionFlags必须在setFocus之前，否则会出现异常
		setTextInteractionFlags(Qt::TextEditorInteraction);
		setFocus();
	}
	QGraphicsTextItem::mouseDoubleClickEvent(event);
}

QVariant DAGraphicsStandardTextItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
	if (change == QGraphicsItem::ItemSceneChange) {
		if (QGraphicsScene* newScene = value.value< QGraphicsScene* >()) {
			if (DAGraphicsScene* daScene = qobject_cast< DAGraphicsScene* >(newScene)) {
				QTextDocument* doc = document();
				if (doc) {

					//! 此方法有问题，前面有2个不知名的command
					// connect(doc,
					// 		&QTextDocument::undoCommandAdded,
					// 		daScene,
					// 		std::bind(&DAGraphicsScene::textDocumentUndoCommandAdded, daScene, doc));

					//! 此方法不行，undoCommandAdded获取的html不是文本改变后的html
					//  connect(doc, &QTextDocument::undoCommandAdded, daScene, [ this, daScene ]() {
					//  	QString hnew = this->toHtml();
					//  	qDebug() << hnew;
					//  	if (mOldHtml != hnew) {
					//  		daScene->push(new DACommandTextItemHtmlContentChanged(this, mOldHtml, hnew));
					//  		mOldHtml = hnew;
					//  	}
					//  });

					//! 此方法可行，但是需要记录非常多的命令，每个字符串的变化都会记录，虽然可以做压缩，
					//! 通过记录变更日期和内容进行时间和内容的压缩，但还是会每敲一个字就触发一次记录
					//! 通过focusOutEvent，无法捕获到全局的变化，如全局的颜色设置等
					connect(doc, &QTextDocument::contentsChanged, this, [ this, doc, daScene ]() {
						QString hnew = doc->toHtml("utf-8");
						if (mOldHtml.isEmpty()) {
							mOldHtml = hnew;
							return;
						}
						if (this->mOldHtml != hnew) {
							daScene->push(new DACommandTextItemHtmlContentChanged(this, mOldHtml, hnew));
							mOldHtml = hnew;
						}
					});
				}
			}
		}
	}
	return QGraphicsTextItem::itemChange(change, value);
}

/**
 * @brief 是否自动绑定DAGraphicsScene的redo/undo
 * @return
 */
bool DAGraphicsStandardTextItem::getAutoBindRedoundoToScene() const
{
    return mAutoBindRedoundoToScene;
}

/**
 * @brief 设置自动绑定DAGraphicsScene的redo/undo
 *
 * 如果是，这样QDocumentText的redo/undo会自动被DAGraphicsScene的redo/undo捕获
 * @param v
 */
void DAGraphicsStandardTextItem::setAutoBindRedoundoToScene(bool v)
{
    mAutoBindRedoundoToScene = v;
}

/**
 * @brief 清除文字的选中
 */
void DAGraphicsStandardTextItem::clearTextSelection()
{
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	setTextCursor(cursor);
}
}
