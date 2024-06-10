#ifndef DACOMMANDSFORWORKFLOW_H
#define DACOMMANDSFORWORKFLOW_H
#include "DAGuiAPI.h"
// qt
#include <QUndoCommand>
#include <QPen>
#include <QBrush>
#include <QList>
#include <QTextDocument>
#include <QPointer>
// workflow
#include "DANodeMetaData.h"
#include "DANodeLinkPoint.h"
#include "DAAbstractNode.h"

namespace DA
{
class DAGraphicsPixmapItem;
class DAWorkFlowGraphicsScene;
class DAGraphicsItem;
class DAGraphicsStandardTextItem;
class DAGraphicsTextItem;

/**
 * @brief 添加背景
 */
class DAGUI_API DACommandWorkFlowSceneAddBackgroundPixmap : public QUndoCommand
{
public:
	DACommandWorkFlowSceneAddBackgroundPixmap(DAWorkFlowGraphicsScene* scene,
											  const QPixmap& pixmap,
											  QUndoCommand* parent = nullptr);
	~DACommandWorkFlowSceneAddBackgroundPixmap();
	void redo();
	void undo();

private:
	DAWorkFlowGraphicsScene* _scene;
	DAGraphicsPixmapItem* _oldItem;
	DAGraphicsPixmapItem* _newItem;
	bool _needDeleteOldItem;
	bool _needDeleteNewItem;
};

/**
 * @brief 改变border的pen
 */
class DAGUI_API DACommandGraphicsShapeBorderPenChange : public QUndoCommand
{
public:
	DACommandGraphicsShapeBorderPenChange(const QList< DAGraphicsItem* >& items, const QPen& p, QUndoCommand* parent = nullptr);
	~DACommandGraphicsShapeBorderPenChange();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsItem* > m_items;
	QList< QPen > m_oldPens;
	QPen m_newPen;
	QList< bool > m_oldIsShow;
	bool m_newIsShow;
};
/**
 * @brief 改变shape的bk brush
 */
class DAGUI_API DACommandGraphicsShapeBackgroundBrushChange : public QUndoCommand
{
public:
	DACommandGraphicsShapeBackgroundBrushChange(const QList< DAGraphicsItem* >& items,
												const QBrush& v,
												QUndoCommand* parent = nullptr);
	~DACommandGraphicsShapeBackgroundBrushChange();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsItem* > m_items;
	QList< QBrush > m_oldBrushs;
	QBrush m_newBrush;
	QList< bool > m_oldIsShow;
	bool m_newIsShow;
};

/**
 * @brief 改变字体
 */
class DAGUI_API DACommandGraphicsStandardTextItemsChangeFont : public QUndoCommand
{
public:
	DACommandGraphicsStandardTextItemsChangeFont(const QList< DAGraphicsStandardTextItem* >& items,
												 const QList< QFont >& newfonts,
												 QUndoCommand* parent = nullptr);
	~DACommandGraphicsStandardTextItemsChangeFont();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsStandardTextItem* > m_items;
	QList< QFont > m_oldFonts;
	QList< QFont > m_newFonts;
};

/**
 * @brief 改变字体
 */
class DAGUI_API DACommandGraphicsTextItemsChangeFont : public QUndoCommand
{
public:
	DACommandGraphicsTextItemsChangeFont(const QList< DAGraphicsTextItem* >& items,
										 const QList< QFont >& newfonts,
										 QUndoCommand* parent = nullptr);
	~DACommandGraphicsTextItemsChangeFont();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsTextItem* > m_items;
	QList< QFont > m_oldFonts;
	QList< QFont > m_newFonts;
};

/**
 * @brief 改变颜色
 */
class DAGUI_API DACommandGraphicsStandardTextItemsChangeColor : public QUndoCommand
{
public:
	DACommandGraphicsStandardTextItemsChangeColor(const QList< DAGraphicsStandardTextItem* >& items,
												  const QList< QColor >& newcolors,
												  QUndoCommand* parent = nullptr);
	~DACommandGraphicsStandardTextItemsChangeColor();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsStandardTextItem* > m_items;
	QList< QColor > m_oldColors;
	QList< QColor > m_newColors;
};

/**
 * @brief 改变颜色
 */
class DAGUI_API DACommandGraphicsTextItemsChangeColor : public QUndoCommand
{
public:
	DACommandGraphicsTextItemsChangeColor(const QList< DAGraphicsTextItem* >& items,
										  const QList< QColor >& newcolors,
										  QUndoCommand* parent = nullptr);
	~DACommandGraphicsTextItemsChangeColor();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsTextItem* > m_items;
	QList< QColor > m_oldColors;
	QList< QColor > m_newColors;
};

/**
 * @brief 改变GraphicsTextItem内容
 */
class DAGUI_API DACommandGraphicsStandardTextItemsChangeHtml : public QUndoCommand
{
public:
	DACommandGraphicsStandardTextItemsChangeHtml(const QList< DAGraphicsStandardTextItem* >& items,
												 const QList< QString >& oldhtml,
												 bool skipFirstRedo   = true,
												 QUndoCommand* parent = nullptr);
	~DACommandGraphicsStandardTextItemsChangeHtml();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsStandardTextItem* > m_items;
	QList< QString > m_oldHtml;
	QList< QString > m_newHtml;
	bool mSkipFirstRedo;
};

/**
 * @brief 改变GraphicsTextItem内容
 */
class DAGUI_API DACommandGraphicsTextItemsChangeHtml : public QUndoCommand
{
public:
	DACommandGraphicsTextItemsChangeHtml(const QList< DAGraphicsTextItem* >& items,
										 const QList< QString >& oldhtml,
										 bool skipFirstRedo   = true,
										 QUndoCommand* parent = nullptr);
	~DACommandGraphicsTextItemsChangeHtml();

	virtual void redo() override;
	virtual void undo() override;

private:
	QList< DAGraphicsTextItem* > m_items;
	QList< QString > m_oldHtml;
	QList< QString > m_newHtml;
	bool mSkipFirstRedo;
};

}

#endif  // DACOMMANDSFORWORKFLOWNODEGRAPHICS_H
