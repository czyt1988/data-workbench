﻿#ifndef DAAPPFIGUREWIDGET_H
#define DAAPPFIGUREWIDGET_H
#include "DAFigureWidget.h"
#include "DAGuiAPI.h"
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;

namespace DA
{
class DADataManager;

DA_IMPL_FORWARD_DECL(DAAppFigureWidget)
/**
 * @brief DAFigureWidget的特例化，加入了拖曳等功能
 */
class DAAppFigureWidget : public DAFigureWidget
{
    DA_IMPL(DAAppFigureWidget)
public:
    DAAppFigureWidget(QWidget* parent = 0);
    ~DAAppFigureWidget();
    void setDataManager(DADataManager* mgr);

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;
};
}
#endif  // DAAPPFIGUREWIDGET_H
