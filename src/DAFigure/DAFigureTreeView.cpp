#include "DAFigureTreeView.h"
#include "DAFigureTreeModel.h"
#include "DAFigureWidget.h"
namespace DA
{
//==============================================================
// DAFigureTreeViewPrivate
//==============================================================

class DAFigureTreeView::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAFigureTreeView)
    PrivateData(DAFigureTreeView* p);
};

DAFigureTreeView::PrivateData::PrivateData(DAFigureTreeView* p) : q_ptr(p)
{
}
//==============================================================
// DAFigureTreeView
//==============================================================

DAFigureTreeView::DAFigureTreeView(QWidget* parent) : QTreeView(parent), DA_PIMPL_CONSTRUCT
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
