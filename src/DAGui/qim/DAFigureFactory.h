#ifndef DAFIGUREFACTORY_H
#define DAFIGUREFACTORY_H
#include "DAGuiAPI.h"
#include "DAFigureScrollArea.h"
namespace DA
{
/**
 * @brief 用于生成DAFigureWidget的工厂类，如果有继承的DAFigureWidget，则需要继承此工厂类提供DAFigureWidget*
 */
class DAGUI_API DAFigureFactory
{
public:
    DAFigureFactory();
    virtual ~DAFigureFactory();
    /**
     * @brief 创建figure接口，如果需要提供自定义的figure，可继承此方法
     *
     * 默认提供@ref DAFigureWidget
     * @param par
     * @return
     */
    virtual DAFigureScrollArea* createFigure(QWidget* par = nullptr);
};
}  // end DA
#endif  // DAFIGUREFACTORY_H
