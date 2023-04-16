#ifndef DAFIGURETREEVIEW_H
#define DAFIGURETREEVIEW_H
#include "DAFigureAPI.h"
#include <QTreeView>
namespace DA
{
class DAFigureWidget;

/**
 * @brief 绘图树
 */
class DAFIGURE_API DAFigureTreeView : public QTreeView
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureTreeView)
public:
    DAFigureTreeView(QWidget* parent = nullptr);
    ~DAFigureTreeView();
    //设置figure
    void setFigure(DAFigureWidget* fig);
    //获取管理的fig
    DAFigureWidget* getFigure() const;
};
}

#endif  // DAFIGURETREEVIEW_H
