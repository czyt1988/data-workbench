#ifndef DAFIGUREFACTORY_H
#define DAFIGUREFACTORY_H
#include "DAGuiAPI.h"
#include "DAChartFactory.h"
#include "DAFigureWidget.h"
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
    virtual DAFigureWidget* createFigure(QWidget* par = nullptr);

protected:
    /**
     * @brief 创建DAChartFactory，DAChartFactory将会设置到DAFigureWidget中
     * @note 调用createFigure会调用createChartFactory，并把factory设置到figure中
     * @return 默认返回nullptr,代表无需额外设置chart factory
     * @note 此函数正常外部不应该调用,而是createFigure内部调用，因此，设置为protected
     */
    virtual DAChartFactory* createChartFactory();
};
}  // end DA
#endif  // DAFIGUREFACTORY_H
