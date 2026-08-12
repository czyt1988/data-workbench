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

/**
 * @brief 构造函数
 * @param parent 父项
 */
DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(QGraphicsItem* parent) : QGraphicsTextItem(parent)
{
    initItem();
}

/**
 * @brief 构造函数，指定文本内容、字体和父项
 * @param str 文本内容
 * @param f 字体
 * @param parent 父项
 */
DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(const QString& str, const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
	initItem();
	setPlainText(str);
	setFont(f);
}

/**
 * @brief 构造函数，指定字体和父项
 * @param f 字体
 * @param parent 父项
 */
DAGraphicsStandardTextItem::DAGraphicsStandardTextItem(const QFont& f, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
	initItem();
	setFont(f);
}

/**
 * @brief 析构函数
 */
DAGraphicsStandardTextItem::~DAGraphicsStandardTextItem()
{
}

/**
 * @brief 初始化文本项，设置默认属性
 */
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

/**
 * @brief 将文本项信息保存到XML
 * @param doc XML文档
 * @param parentElement 父XML元素
 * @param ver 版本号
 * @return 保存成功返回true
 */
bool DAGraphicsStandardTextItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	QDomElement textItemEle = doc->createElement("text-info");
	QPointF p               = pos();
	textItemEle.setAttribute("flags", static_cast< int >(flags()));
	textItemEle.setAttribute("id", static_cast< qulonglong >(getItemID()));
	textItemEle.setAttribute("x", p.x());
	textItemEle.setAttribute("y", p.y());
	textItemEle.setAttribute("z", zValue());
	textItemEle.setAttribute("rotation", rotation());
	textItemEle.setAttribute("scale", scale());
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

/**
 * @brief 从XML加载文本项信息
 * @param itemElement XML元素
 * @param ver 版本号
 * @return 加载成功返回true
 */
bool DAGraphicsStandardTextItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	QDomElement textItemEle = itemElement->firstChildElement("text-info");
	if (textItemEle.isNull()) {
		return false;
	}
	QPointF pos;
	qulonglong id;
	qreal realValue;
	QString strflags = textItemEle.attribute("flags");
	if (!strflags.isEmpty()) {
		bool ok       = false;
		int flagvalue = strflags.toInt(&ok);
		if (ok) {
			GraphicsItemFlags f = static_cast< GraphicsItemFlags >(flagvalue);
			setFlags(f);
		}
	}
	if (getStringRealValue(textItemEle.attribute("x"), pos.rx())
		&& getStringRealValue(textItemEle.attribute("y"), pos.ry())) {
		setPos(pos);
	}
	if (getStringULongLongValue(textItemEle.attribute("id"), id)) {
		setItemID(id);
	}
	if (getStringRealValue(textItemEle.attribute("z", ""), realValue)) {
		setZValue(realValue);
	}
	if (getStringRealValue(textItemEle.attribute("rotation", ""), realValue)) {
		setRotation(realValue);
	}
	if (getStringRealValue(textItemEle.attribute("scale", ""), realValue)) {
		setScale(realValue);
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

/**
 * @brief 设置文本项在场景中的位置
 * @param p 场景坐标
 */
void DAGraphicsStandardTextItem::setScenePos(const QPointF& p)
{
	setPos(mapToParent(mapFromScene(p)));
}

/**
 * @brief 设置文本项在场景中的位置
 * @param x x坐标
 * @param y y坐标
 */
void DAGraphicsStandardTextItem::setScenePos(qreal x, qreal y)
{
	setScenePos(QPointF(x, y));
}

/**
 * @brief 获取文本项的唯一ID
 * @return 文本项ID
 * @sa setItemID
 */
uint64_t DAGraphicsStandardTextItem::getItemID() const
{
	return mID;
}

/**
 * @brief 设置文本项的唯一ID
 * @param id 文本项ID
 * @sa getItemID
 */
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
	if (!format.isValid() || format.isEmpty()) {
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
	if (!format.isValid() || format.isEmpty()) {
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
	if (!format.isValid() || format.isEmpty()) {
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
	if (!format.isValid() || format.isEmpty()) {
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
	if (!format.isValid() || format.isEmpty()) {
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
	if (!format.isValid() || format.isEmpty()) {
		return false;
	}
	auto w = format.fontWeight();
	return w == QFont::Bold;
}

/**
 * @brief 焦点离开事件，退出编辑模式
 * @param focusEvent 焦点事件
 */
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

/**
 * @brief 鼠标双击事件，左键双击进入编辑模式
 * @param event 鼠标事件
 */
void DAGraphicsStandardTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		// czy:setTextInteractionFlags必须在setFocus之前，否则会出现异常
		setTextInteractionFlags(Qt::TextEditorInteraction);
		setFocus();
	}
	QGraphicsTextItem::mouseDoubleClickEvent(event);
}

/**
 * @brief 项变化事件，当场景改变时绑定文本内容变化到DAGraphicsScene的undo/redo
 * @param change 变化类型
 * @param value 变化值
 * @return 返回变化后的值
 */
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
						QString hnew = doc->toHtml("utf-8");
#else
						QString hnew = doc->toHtml();
#endif
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
