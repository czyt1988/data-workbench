#include "DACommandsForWorkFlow.h"
// qt
#include <QObject>
// workflow
#include "DAPyWorkFlowGraphicsScene.h"
#include "DAGraphicsPixmapItem.h"
#include "DAPyWorkFlow.h"
#include "DAGraphicsItem.h"
#include "DAGraphicsTextItem.h"
#include "DAGraphicsStandardTextItem.h"
namespace DA
{
//===================================================
// DACommandWorkFlowSceneAddBackgroundPixmap
//===================================================

/**
 * @brief 构造函数
 * @param scene 工作流场景
 * @param pixmap 背景图片
 * @param parent 父命令
 */
DACommandWorkFlowSceneAddBackgroundPixmap::DACommandWorkFlowSceneAddBackgroundPixmap(DAPyWorkFlowGraphicsScene* scene,
                                                                                     const QPixmap& pixmap,
                                                                                     QUndoCommand* parent)
    : QUndoCommand(parent), mScene(scene), mNeedDeleteOldItem(false), mNeedDeleteNewItem(false)
{
	mOldItem = mScene->getBackgroundPixmapItem();
	if (mOldItem) {
		mNeedDeleteOldItem = true;
	}
	mNewItem = new DAGraphicsPixmapItem();
	mNewItem->setPixmap(pixmap);
	mNeedDeleteNewItem = false;
}

/**
 * @brief 析构函数
 */
DACommandWorkFlowSceneAddBackgroundPixmap::~DACommandWorkFlowSceneAddBackgroundPixmap()
{
	if (mNeedDeleteOldItem) {
		if (mOldItem) {
			if (mScene) {
				mScene->removeItem(mOldItem);
			}
			delete mOldItem;
		}
	}
	if (mNeedDeleteNewItem) {
		if (mNewItem) {
			if (mScene) {
				mScene->removeItem(mNewItem);
			}
			delete mNewItem;
		}
	}
}

/**
 * @brief 重做：设置新的背景图
 */
void DACommandWorkFlowSceneAddBackgroundPixmap::redo()
{
	if (mOldItem) {
		mNeedDeleteOldItem = true;
		mScene->removeBackgroundPixmapItem();
	}
	mScene->setBackgroundPixmapItem(mNewItem);
	mNeedDeleteNewItem = false;
}

/**
 * @brief 撤销：恢复旧的背景图
 */
void DACommandWorkFlowSceneAddBackgroundPixmap::undo()
{
	mScene->removeBackgroundPixmapItem();
	mNeedDeleteNewItem = true;
	if (mOldItem) {
		mNeedDeleteOldItem = false;
		mScene->setBackgroundPixmapItem(mOldItem);
	}
}

//==============================================================
// DACommandGraphicsShapeBorderPenChange
//==============================================================

/**
 * @brief 构造函数
 * @param items 图形项列表
 * @param p 新的画笔
 * @param parent 父命令
 */
DACommandGraphicsShapeBorderPenChange::DACommandGraphicsShapeBorderPenChange(const QList< DAGraphicsItem* >& items,
                                                                             const QPen& p,
                                                                             QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mNewPen(p)
{
	mNewIsShow = p.style() != Qt::NoPen;
	for (const DAGraphicsItem* i : items) {
		mOldIsShow.append(i->isShowBorder());
		mOldPens.append(i->getBorderPen());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsShapeBorderPenChange::~DACommandGraphicsShapeBorderPenChange()
{
}

/**
 * @brief 重做：应用新的边框画笔
 */
void DACommandGraphicsShapeBorderPenChange::redo()
{
	for (DAGraphicsItem* i : std::as_const(mItems)) {
		i->setShowBorder(mNewIsShow);
		i->setBorderPen(mNewPen);
		i->update();
	}
}

/**
 * @brief 撤销：恢复旧的边框画笔
 */
void DACommandGraphicsShapeBorderPenChange::undo()
{
	for (int i = 0; i < mItems.size(); ++i) {
		mItems[ i ]->setShowBorder(mOldIsShow[ i ]);
		mItems[ i ]->setBorderPen(mOldPens[ i ]);
		mItems[ i ]->update();
	}
}
//==============================================================
// DACommandGraphicsShapeBackgroundBrushChange
//==============================================================

/**
 * @brief 构造函数
 * @param items 图形项列表
 * @param v 新的画刷
 * @param parent 父命令
 */
DACommandGraphicsShapeBackgroundBrushChange::DACommandGraphicsShapeBackgroundBrushChange(const QList< DAGraphicsItem* >& items,
                                                                                         const QBrush& v,
                                                                                         QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mNewBrush(v)
{
	mNewIsShow = v.style() != Qt::NoBrush;
	for (const DAGraphicsItem* i : items) {
		mOldIsShow.append(i->isShowBackground());
		mOldBrushs.append(i->getBackgroundBrush());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsShapeBackgroundBrushChange::~DACommandGraphicsShapeBackgroundBrushChange()
{
}

/**
 * @brief 重做：应用新的背景画刷
 */
void DACommandGraphicsShapeBackgroundBrushChange::redo()
{
	for (DAGraphicsItem* i : std::as_const(mItems)) {
		i->enableShowBackground(mNewIsShow);
		i->setBackgroundBrush(mNewBrush);
		i->update();
	}
}

/**
 * @brief 撤销：恢复旧的背景画刷
 */
void DACommandGraphicsShapeBackgroundBrushChange::undo()
{
	for (int i = 0; i < mItems.size(); ++i) {
		mItems[ i ]->enableShowBackground(mOldIsShow[ i ]);
		mItems[ i ]->setBackgroundBrush(mOldBrushs[ i ]);
		mItems[ i ]->update();
	}
}
//===================================================
// DACommandGraphicsTextItemsChangeFont
//===================================================

/**
 * @brief 构造函数
 * @param items 文本项列表
 * @param newfonts 新字体列表
 * @param parent 父命令
 */
DACommandGraphicsStandardTextItemsChangeFont::DACommandGraphicsStandardTextItemsChangeFont(
    const QList< DAGraphicsStandardTextItem* >& items,
    const QList< QFont >& newfonts,
    QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mNewFonts(newfonts)
{
	for (const DAGraphicsStandardTextItem* i : items) {
		mOldFonts.append(i->font());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsStandardTextItemsChangeFont::~DACommandGraphicsStandardTextItemsChangeFont()
{
}

/**
 * @brief 重做：应用新字体
 */
void DACommandGraphicsStandardTextItemsChangeFont::redo()
{
	QUndoCommand::redo();  // 此函数会执行子内容的redo/undo
	int size = qMin(mItems.size(), mNewFonts.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setFont(mNewFonts[ i ]);
	}
}

/**
 * @brief 撤销：恢复旧字体
 */
void DACommandGraphicsStandardTextItemsChangeFont::undo()
{
	QUndoCommand::undo();
	int size = qMin(mItems.size(), mOldFonts.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setFont(mOldFonts[ i ]);
	}
}

//===============================================================
// DACommandGraphicsTextItemsChangeFont
//===============================================================

/**
 * @brief 构造函数
 * @param items 文本项列表
 * @param newfonts 新字体列表
 * @param parent 父命令
 */
DACommandGraphicsTextItemsChangeFont::DACommandGraphicsTextItemsChangeFont(const QList< DAGraphicsTextItem* >& items,
                                                                           const QList< QFont >& newfonts,
                                                                           QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mNewFonts(newfonts)
{
	for (const DAGraphicsTextItem* i : items) {
		mOldFonts.append(i->getSelectTextFont());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsTextItemsChangeFont::~DACommandGraphicsTextItemsChangeFont()
{
}

/**
 * @brief 重做：应用新字体
 */
void DACommandGraphicsTextItemsChangeFont::redo()
{
	QUndoCommand::redo();  // 此函数会执行子内容的redo/undo
	int size = qMin(mItems.size(), mNewFonts.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setSelectTextFont(mNewFonts[ i ]);
	}
}

/**
 * @brief 撤销：恢复旧字体
 */
void DACommandGraphicsTextItemsChangeFont::undo()
{
	QUndoCommand::undo();
	int size = qMin(mItems.size(), mOldFonts.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setSelectTextFont(mOldFonts[ i ]);
	}
}
//===================================================
// DACommandGraphicsStandardTextItemsChangeColor
//===================================================

/**
 * @brief 构造函数
 * @param items 文本项列表
 * @param newcolors 新颜色列表
 * @param parent 父命令
 */
DACommandGraphicsStandardTextItemsChangeColor::DACommandGraphicsStandardTextItemsChangeColor(
    const QList< DAGraphicsStandardTextItem* >& items,
    const QList< QColor >& newcolors,
    QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mNewColors(newcolors)
{
	for (const DAGraphicsStandardTextItem* i : items) {
		mOldColors.append(i->defaultTextColor());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsStandardTextItemsChangeColor::~DACommandGraphicsStandardTextItemsChangeColor()
{
}

/**
 * @brief 重做：应用新颜色
 */
void DACommandGraphicsStandardTextItemsChangeColor::redo()
{
	QUndoCommand::redo();
	int size = qMin(mItems.size(), mNewColors.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setDefaultTextColor(mNewColors[ i ]);
	}
}

/**
 * @brief 撤销：恢复旧颜色
 */
void DACommandGraphicsStandardTextItemsChangeColor::undo()
{
	QUndoCommand::undo();
	int size = qMin(mItems.size(), mOldColors.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setDefaultTextColor(mOldColors[ i ]);
	}
}
//===============================================================
// DACommandGraphicsTextItemsChangeColor
//===============================================================

/**
 * @brief 构造函数
 * @param items 文本项列表
 * @param newcolors 新颜色列表
 * @param parent 父命令
 */
DACommandGraphicsTextItemsChangeColor::DACommandGraphicsTextItemsChangeColor(const QList< DAGraphicsTextItem* >& items,
                                                                             const QList< QColor >& newcolors,
                                                                             QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mNewColors(newcolors)
{
	for (const DAGraphicsTextItem* i : items) {
		mOldColors.append(i->getSelectTextColor());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsTextItemsChangeColor::~DACommandGraphicsTextItemsChangeColor()
{
}

/**
 * @brief 重做：应用新颜色
 */
void DACommandGraphicsTextItemsChangeColor::redo()
{
	QUndoCommand::redo();
	int size = qMin(mItems.size(), mNewColors.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setSelectTextColor(mNewColors[ i ]);
	}
}

/**
 * @brief 撤销：恢复旧颜色
 */
void DACommandGraphicsTextItemsChangeColor::undo()
{
	QUndoCommand::undo();
	int size = qMin(mItems.size(), mOldColors.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setSelectTextColor(mOldColors[ i ]);
	}
}
//===============================================================
// DACommandGraphicsStandardTextItemsChangeHtml
//===============================================================

/**
 * @brief 构造函数
 * @param items 文本项列表
 * @param oldhtml 旧HTML内容列表
 * @param skipFirstRedo 是否跳过首次redo
 * @param parent 父命令
 */
DACommandGraphicsStandardTextItemsChangeHtml::DACommandGraphicsStandardTextItemsChangeHtml(
    const QList< DAGraphicsStandardTextItem* >& items,
    const QList< QString >& oldhtml,
    bool skipFirstRedo,
    QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mOldHtml(oldhtml), mSkipFirstRedo(skipFirstRedo)
{
	for (const DAGraphicsStandardTextItem* i : items) {
		mNewHtml.append(i->toHtml());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsStandardTextItemsChangeHtml::~DACommandGraphicsStandardTextItemsChangeHtml()
{
}

/**
 * @brief 重做：应用新HTML内容
 */
void DACommandGraphicsStandardTextItemsChangeHtml::redo()
{
	if (mSkipFirstRedo) {
		mSkipFirstRedo = false;
		return;
	}
	QUndoCommand::redo();
	int size = qMin(mItems.size(), mNewHtml.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setHtml(mNewHtml[ i ]);
	}
}

/**
 * @brief 撤销：恢复旧HTML内容
 */
void DACommandGraphicsStandardTextItemsChangeHtml::undo()
{
	QUndoCommand::undo();
	int size = qMin(mItems.size(), mOldHtml.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setHtml(mOldHtml[ i ]);
	}
}
//===============================================================
// DACommandGraphicsTextItemsChangeHtml
//===============================================================

/**
 * @brief 构造函数
 * @param items 文本项列表
 * @param oldhtml 旧HTML内容列表
 * @param skipFirstRedo 是否跳过首次redo
 * @param parent 父命令
 */
DACommandGraphicsTextItemsChangeHtml::DACommandGraphicsTextItemsChangeHtml(const QList< DAGraphicsTextItem* >& items,
                                                                           const QList< QString >& oldhtml,
                                                                           bool skipFirstRedo,
                                                                           QUndoCommand* parent)
    : QUndoCommand(parent), mItems(items), mOldHtml(oldhtml), mSkipFirstRedo(skipFirstRedo)
{
	for (const DAGraphicsTextItem* i : items) {
		mNewHtml.append(i->toHtml());
	}
}

/**
 * @brief 析构函数
 */
DACommandGraphicsTextItemsChangeHtml::~DACommandGraphicsTextItemsChangeHtml()
{
}

/**
 * @brief 重做：应用新HTML内容
 */
void DACommandGraphicsTextItemsChangeHtml::redo()
{
	if (mSkipFirstRedo) {
		mSkipFirstRedo = false;
		return;
	}
	QUndoCommand::redo();
	int size = qMin(mItems.size(), mNewHtml.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setHtml(mNewHtml[ i ]);
	}
}

/**
 * @brief 撤销：恢复旧HTML内容
 */
void DACommandGraphicsTextItemsChangeHtml::undo()
{
	QUndoCommand::undo();
	int size = qMin(mItems.size(), mOldHtml.size());
	for (int i = 0; i < size; ++i) {
		mItems[ i ]->setHtml(mOldHtml[ i ]);
	}
}

}  // end DA
