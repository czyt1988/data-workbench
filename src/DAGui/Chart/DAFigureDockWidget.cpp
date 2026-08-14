#include "DAFigureDockWidget.h"
// Qt
#include <QVBoxLayout>
// DAFigure
#include "DAFigureWidget.h"
namespace DA
{
//==============================================================
// DAFigureDockWidgetPrivate
//==============================================================
class DAFigureDockWidgetPrivate
{
    DA_IMPL_PUBLIC(DAFigureDockWidget)
public:
    DAFigureDockWidgetPrivate(DAFigureDockWidget* p);

public:
    DAFigureWidget* mFigureWidget { nullptr };  ///< 被封装的绘图窗口
};
DAFigureDockWidgetPrivate::DAFigureDockWidgetPrivate(DAFigureDockWidget* p) : q_ptr(p)
{
}

//===================================================
// DAFigureDockWidget
//===================================================
/**
 * @brief 构造
 *
 * 把传入的 DAFigureWidget 通过布局接管其父级（reparent 到本部件），
 * 供后续 ads::CDockWidget 包装
 * @param fig 被封装的绘图窗口，允许为空（此时不放置任何内容）
 * @param parent 父部件
 */
DAFigureDockWidget::DAFigureDockWidget(DAFigureWidget* fig, QWidget* parent)
    : QWidget(parent), d_ptr(new DAFigureDockWidgetPrivate(this))
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    if (fig) {
        d_ptr->mFigureWidget = fig;
        lay->addWidget(fig);  // reparent fig to this widget
    }
}

DAFigureDockWidget::~DAFigureDockWidget()
{
}

/**
 * @brief 获取封装的绘图窗口
 * @return
 */
DAFigureWidget* DAFigureDockWidget::getFigureWidget() const
{
    return d_ptr->mFigureWidget;
}

}  // namespace DA
