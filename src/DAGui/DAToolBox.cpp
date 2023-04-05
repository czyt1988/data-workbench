#include "DAToolBox.h"
#include "DANodeListWidget.h"
#include <QToolBox>
#include <QDebug>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAToolBox
//===================================================
DAToolBox::DAToolBox(QWidget* parent) : QScrollArea(parent), _favoriteList(nullptr)
{
    _toolBox = new QToolBox(this);
    setWidget(_toolBox);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}
/**
 * @brief 添加items
 * @param datas
 * @note 注意已淘汰
 */
void DAToolBox::addItems(const QMap< QString, QList< DANodeMetaData > >& datas)
{
    for (auto i = datas.begin(); i != datas.end(); ++i) {
        DANodeListWidget* nlw = new DANodeListWidget(this);
        nlw->addItems(i.value());
        _toolBox->addItem(nlw, i.key());
    }
    adjustMinItemHight(300);
}

/**
 * @brief 添加items
 * @param datas
 */
void DAToolBox::addItems(const QList< DANodeMetaData >& datas)
{
    //先提取分组，确认分组都建立
    QList< QString > orderGroup;
    QHash< QString, QList< DANodeMetaData > > groupOrderNodes;
    for (const DANodeMetaData& md : qAsConst(datas)) {
        // 每个md的分组按顺序去重归集
        if (!orderGroup.contains(md.getGroup())) {
            orderGroup.append(md.getGroup());
        }
        groupOrderNodes[ md.getGroup() ].append(md);
    }
    //创建分组的topitem
    for (const QString& g : qAsConst(orderGroup)) {
        DANodeListWidget* gitem = new DANodeListWidget(this);
        _toolBox->addItem(gitem, g);
        gitem->addItems(groupOrderNodes[ g ]);
    }
    adjustMinItemHight(300);
}

/**
 * @brief 获取收藏list，如果没有就创建
 * @return
 */
DANodeListWidget* DAToolBox::getFavoriteList()
{
    if (nullptr == _favoriteList) {
        return createFavoriteList();
    }
    return _favoriteList;
}

/**
 * @brief 创建一个收藏列表
 *
 * 收藏列表只允许有一个，重复调用不会重复创建
 * @return
 */
DANodeListWidget* DAToolBox::createFavoriteList()
{
    if (_favoriteList) {
        return _favoriteList;
    }
    _favoriteList = new DANodeListWidget(this);
    _favoriteList->setProperty("isFavoriteListWidget", true);
    _toolBox->addItem(_favoriteList, QIcon(":/icon/icon/favorite.svg"), tr("Favorite"));
    return _favoriteList;
}

/**
 * @brief 添加到收藏
 * @param md
 */
void DAToolBox::addToFavorite(const DANodeMetaData& md)
{
    DANodeListWidget* fav = getFavoriteList();
    fav->addItem(md);
}

/**
 * @brief 移除收藏
 * @param md
 */
void DAToolBox::removeFavorite(const DANodeMetaData& md)
{
    DANodeListWidget* fav = getFavoriteList();
    int c                 = fav->count();
    QList< QListWidgetItem* > needRemove;
    for (int i = 0; i < c; ++i) {
        QListWidgetItem* item = fav->item(i);
        if (item->type() == DANodeListWidgetItem::ThisItemType) {
            DANodeListWidgetItem* nitem = static_cast< DANodeListWidgetItem* >(item);
            if (md == nitem->getNodeMetaData()) {
                needRemove.append(item);
            }
        }
    }
    for (QListWidgetItem* i : needRemove) {
        delete i;
    }
}

/**
 * @brief 通过位置获取对应的md
 * @param p
 * @return
 */
DANodeMetaData DAToolBox::getNodeMetaData(const QPoint& p) const
{
    DANodeListWidget* nl = qobject_cast< DANodeListWidget* >(_toolBox->currentWidget());
    if (!nl) {
        return DANodeMetaData();
    }
    return nl->getNodeMetaData(nl->mapFromGlobal(mapToGlobal(p)));
}

/**
 * @brief 自适应item的最小高度
 *
 * 如果发现item的高度小于传入设置的最小值，则会根据最小值自动计算QToolBox的最小高度，保证item能达到最小值
 * @param minHeight
 */
void DAToolBox::adjustMinItemHight(int minHeight)
{
    QWidget* w = _toolBox->currentWidget();
    if (w == nullptr) {
        //没有设置窗口返回
        return;
    }
    int curheight = w->height();
    qDebug() << "curheight=" << curheight;
    if (curheight >= minHeight) {
        return;
    }
    //由于QToolBox没有获取item标签高度的接口，因此只能估算标签高度为文字高度的1.5倍,或者icon的1.1
    int itemTitleHeight = _toolBox->fontMetrics().lineSpacing();
    QIcon icon          = _toolBox->itemIcon(_toolBox->currentIndex());
    if (!icon.isNull()) {
        QList< QSize > avas = icon.availableSizes();
        for (const QSize& s : qAsConst(avas)) {
            if (s.height() * 1.1 > itemTitleHeight) {
                itemTitleHeight = s.height() * 1.1;
            }
        }
    }
    //计算最大的高度
    int willSetMinHeight = _toolBox->count() * itemTitleHeight;
    willSetMinHeight += minHeight;
    //
    _toolBox->setMinimumHeight(willSetMinHeight);
    qDebug() << "setMinimumHeight=" << willSetMinHeight;
}

int DAToolBox::addItem(QWidget* w, const QIcon& iconSet, const QString& text)
{
    return _toolBox->addItem(w, iconSet, text);
}

int DAToolBox::addItem(QWidget* w, const QString& text)
{
    return _toolBox->addItem(w, text);
}

int DAToolBox::count() const
{
    return _toolBox->count();
}

int DAToolBox::currentIndex() const
{
    return _toolBox->currentIndex();
}

QWidget* DAToolBox::currentWidget() const
{
    return _toolBox->currentWidget();
}

}
