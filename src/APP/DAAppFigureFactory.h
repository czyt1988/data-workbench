#ifndef DAAPPFIGUREFACTORY_H
#define DAAPPFIGUREFACTORY_H
#include "DAFigureFactory.h"
namespace DA
{
class DADataManager;
/**
 * @brief APP下特化绘图相关的类
 */
class DAAppFigureFactory : public DAFigureFactory
{
public:
    DAAppFigureFactory();
    virtual ~DAAppFigureFactory();
    virtual DAFigureWidget* createFigure(QWidget* par = nullptr) override;
};
}

#endif  // DAAPPFIGUREFACTORY_H
