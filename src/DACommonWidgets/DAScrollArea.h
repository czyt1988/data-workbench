#ifndef DASCROLLAREA_H
#define DASCROLLAREA_H
#include <QScrollArea>
#include "DACommonWidgetsAPI.h"

namespace DA{
/**
 * @brief QScrollArea特化
 */
class DACOMMONWIDGETS_API DAScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    DAScrollArea(QWidget *parent = nullptr);
    ~DAScrollArea();
};
}//end DA
#endif // DASCROLLAREA_H
