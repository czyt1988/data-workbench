#ifndef DATOOLBOX_H
#define DATOOLBOX_H

#include <QScrollArea>
#include <QMap>
#include "DAPyNodeFactory.h"
#include "DAGuiAPI.h"
class QToolBox;
namespace DA
{
class DANodeListWidget;

/**
 * @brief 针对workflow节点显示的ToolBox
 */
class DAGUI_API DAToolBox : public QScrollArea
{
    Q_OBJECT
public:
    DAToolBox(QWidget* parent = nullptr);
    void addItems(const QMap< QString, QList< DAPyNodeMetaData > >& datas);
    void addItems(const QList< DAPyNodeMetaData >& datas);
    //清空所有分组页（收藏页保留），用于插件热插拔后重建节点列表
    void clear();
    //获取收藏list，如果没有就返回nullptr
    DANodeListWidget* getFavoriteList();
    //创建收藏列
    DANodeListWidget* createFavoriteList();
    //添加到收藏
    void addToFavorite(const DAPyNodeMetaData& md);
    void removeFavorite(const DAPyNodeMetaData& md);
    //
    DAPyNodeMetaData getNodeMetaData(const QPoint& p) const;
    //自适应item的最小高度，如果发现item的高度小于传入设置的最小值，
    //则会根据最小值自动计算QToolBox的最小高度，保证item能达到最小值
    void adjustMinItemHight(int minHeight);
    // QToolBox的代理
    int addItem(QWidget* w, const QIcon& iconSet, const QString& text);
    int addItem(QWidget* w, const QString& text);
    int count() const;
    int currentIndex() const;
    QWidget* currentWidget() const;

private:
    QToolBox* mToolBox;
    DANodeListWidget* mFavoriteList;
};
}  // namespace DA
#endif  // FCTOOLBOX_H
