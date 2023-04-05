#include "DAFigureTreeView.h"
#include "DAFigureTreeModel.h"
#include "DAFigureWidget.h"
namespace DA
{
//==============================================================
// DAFigureTreeViewPrivate
//==============================================================

class DAFigureTreeViewPrivate
{
public:
    DA_IMPL_PUBLIC(DAFigureTreeView)
    DAFigureTreeViewPrivate(DAFigureTreeView* p);
};

DAFigureTreeViewPrivate::DAFigureTreeViewPrivate(DAFigureTreeView* p) : q_ptr(p)
{
}
//==============================================================
// DAFigureTreeView
//==============================================================

DAFigureTreeView::DAFigureTreeView(QWidget* parent) : QTreeView(parent), d_ptr(new DAFigureTreeViewPrivate(this))
{
    DAFigureTreeModel* m = new DAFigureTreeModel(this);
    setModel(m);
}

DAFigureTreeView::~DAFigureTreeView()
{
}

/**
 * @brief 设置fig
 * @param fig
 */
void DAFigureTreeView::setFigure(DA::DAFigureWidget* fig)
{
    DAFigureTreeModel* m = static_cast< DAFigureTreeModel* >(model());
    m->setFigure(fig);
}

/**
 * @brief 获取管理的窗口
 * @return
 */
DAFigureWidget* DAFigureTreeView::getFigure() const
{
    DAFigureTreeModel* m = static_cast< DAFigureTreeModel* >(model());
    return m->getFigure();
}
}
