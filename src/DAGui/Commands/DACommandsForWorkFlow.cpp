#include "DACommandsForWorkFlow.h"
// qt
#include <QObject>
// workflow
#include "DANodeGraphicsScene.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsPixmapItem.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAWorkFlow.h"
#include "DAGraphicsItem.h"
#include "DAGraphicsStandardTextItem.h"
namespace DA
{
//===================================================
// DACommandWorkFlowSceneAddBackgroundPixmap
//===================================================
DACommandWorkFlowSceneAddBackgroundPixmap::DACommandWorkFlowSceneAddBackgroundPixmap(DAWorkFlowGraphicsScene* scene,
                                                                                     const QPixmap& pixmap,
                                                                                     QUndoCommand* parent)
    : QUndoCommand(parent), _scene(scene), _needDeleteOldItem(false), _needDeleteNewItem(false)
{
    _oldItem = _scene->getBackgroundPixmapItem();
    if (_oldItem) {
        _needDeleteOldItem = true;
    }
    _newItem = new DAGraphicsPixmapItem();
    _newItem->setPixmap(pixmap);
    _needDeleteNewItem = false;
}

DACommandWorkFlowSceneAddBackgroundPixmap::~DACommandWorkFlowSceneAddBackgroundPixmap()
{
    if (_needDeleteOldItem) {
        if (_oldItem) {
            _scene->removeItem(_oldItem);
            delete _oldItem;
        }
    }
    if (_needDeleteNewItem) {
        if (_newItem) {
            _scene->removeItem(_newItem);
            delete _newItem;
        }
    }
}

void DACommandWorkFlowSceneAddBackgroundPixmap::redo()
{
    if (_oldItem) {
        _needDeleteOldItem = true;
        _scene->removeBackgroundPixmapItem();
    }
    _scene->setBackgroundPixmapItem(_newItem);
    _needDeleteNewItem = false;
}

void DACommandWorkFlowSceneAddBackgroundPixmap::undo()
{
    _scene->removeBackgroundPixmapItem();
    _needDeleteNewItem = true;
    if (_oldItem) {
        _needDeleteOldItem = false;
        _scene->setBackgroundPixmapItem(_oldItem);
    }
}

//==============================================================
// DACommandGraphicsShapeBorderPenChange
//==============================================================

DACommandGraphicsShapeBorderPenChange::DACommandGraphicsShapeBorderPenChange(const QList< DAGraphicsItem* >& items,
                                                                             const QPen& p,
                                                                             QUndoCommand* parent)
    : QUndoCommand(parent), m_items(items), m_newPen(p)
{
    m_newIsShow = p.style() != Qt::NoPen;
    for (const DAGraphicsItem* i : qAsConst(items)) {
        m_oldIsShow.append(i->isShowBorder());
        m_oldPens.append(i->getBorderPen());
    }
}

DACommandGraphicsShapeBorderPenChange::~DACommandGraphicsShapeBorderPenChange()
{
}

void DACommandGraphicsShapeBorderPenChange::redo()
{
    for (DAGraphicsItem* i : m_items) {
        i->setShowBorder(m_newIsShow);
        i->setBorderPen(m_newPen);
        i->update();
    }
}

void DACommandGraphicsShapeBorderPenChange::undo()
{
    for (int i = 0; i < m_items.size(); ++i) {
        m_items[ i ]->setShowBorder(m_oldIsShow[ i ]);
        m_items[ i ]->setBorderPen(m_oldPens[ i ]);
        m_items[ i ]->update();
    }
}
//==============================================================
// DACommandGraphicsShapeBackgroundBrushChange
//==============================================================
DACommandGraphicsShapeBackgroundBrushChange::DACommandGraphicsShapeBackgroundBrushChange(const QList< DAGraphicsItem* >& items,
                                                                                         const QBrush& v,
                                                                                         QUndoCommand* parent)
    : QUndoCommand(parent), m_items(items), m_newBrush(v)
{
    m_newIsShow = v.style() != Qt::NoBrush;
    for (const DAGraphicsItem* i : qAsConst(items)) {
        m_oldIsShow.append(i->isShowBackground());
        m_oldBrushs.append(i->getBackgroundBrush());
    }
}

DACommandGraphicsShapeBackgroundBrushChange::~DACommandGraphicsShapeBackgroundBrushChange()
{
}

void DACommandGraphicsShapeBackgroundBrushChange::redo()
{
    for (DAGraphicsItem* i : m_items) {
        i->setShowBackground(m_newIsShow);
        i->setBackgroundBrush(m_newBrush);
        i->update();
    }
}

void DACommandGraphicsShapeBackgroundBrushChange::undo()
{
    for (int i = 0; i < m_items.size(); ++i) {
        m_items[ i ]->setShowBackground(m_oldIsShow[ i ]);
        m_items[ i ]->setBackgroundBrush(m_oldBrushs[ i ]);
        m_items[ i ]->update();
    }
}
//===================================================
// DACommandGraphicsTextItemsChangeFont
//===================================================
DACommandGraphicsTextItemsChangeFont::DACommandGraphicsTextItemsChangeFont(const QList< DAGraphicsStandardTextItem* >& items,
                                                                           const QList< QFont >& newfonts,
                                                                           QUndoCommand* parent)
    : QUndoCommand(parent), m_items(items), m_newFonts(newfonts)
{
    for (const DAGraphicsStandardTextItem* i : qAsConst(items)) {
        m_oldFonts.append(i->font());
    }
}

DACommandGraphicsTextItemsChangeFont::~DACommandGraphicsTextItemsChangeFont()
{
}

void DACommandGraphicsTextItemsChangeFont::redo()
{
    QUndoCommand::redo();  // 此函数会执行子内容的redo/undo
    int size = qMin(m_items.size(), m_newFonts.size());
    for (int i = 0; i < size; ++i) {
        m_items[ i ]->setFont(m_newFonts[ i ]);
    }
}

void DACommandGraphicsTextItemsChangeFont::undo()
{
    QUndoCommand::undo();
    int size = qMin(m_items.size(), m_oldFonts.size());
    for (int i = 0; i < size; ++i) {
        m_items[ i ]->setFont(m_oldFonts[ i ]);
    }
}

//===================================================
// DACommandGraphicsTextItemsChangeColor
//===================================================
DACommandGraphicsTextItemsChangeColor::DACommandGraphicsTextItemsChangeColor(const QList< DAGraphicsStandardTextItem* >& items,
                                                                             const QList< QColor >& newcolors,
                                                                             QUndoCommand* parent)
    : QUndoCommand(parent), m_items(items), m_newColors(newcolors)
{
    for (const DAGraphicsStandardTextItem* i : qAsConst(items)) {
        m_oldColors.append(i->defaultTextColor());
    }
}

DACommandGraphicsTextItemsChangeColor::~DACommandGraphicsTextItemsChangeColor()
{
}

void DACommandGraphicsTextItemsChangeColor::redo()
{
    QUndoCommand::redo();
    int size = qMin(m_items.size(), m_newColors.size());
    for (int i = 0; i < size; ++i) {
        m_items[ i ]->setDefaultTextColor(m_newColors[ i ]);
    }
}

void DACommandGraphicsTextItemsChangeColor::undo()
{
    QUndoCommand::undo();
    int size = qMin(m_items.size(), m_oldColors.size());
    for (int i = 0; i < size; ++i) {
        m_items[ i ]->setDefaultTextColor(m_oldColors[ i ]);
    }
}

}  // end DA
