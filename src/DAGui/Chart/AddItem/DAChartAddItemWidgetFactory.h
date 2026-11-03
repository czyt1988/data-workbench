#ifndef DACHARTADDITEMWIDGETFACTORY_H
#define DACHARTADDITEMWIDGETFACTORY_H
#include "DAGuiAPI.h"
#include "DAFigureAPI.h"
#include <QIcon>
#include <QList>
#include <QString>
#include <functional>

class QwtPlotItem;

namespace DA
{
class DAAbstractChartAddWidgetBase;

/**
 * @brief 图表添加页所属的窗口家族
 */
enum class DAChartAddWidgetFamily
{
    Curve2D,    ///< 二维绘图引导窗口
    Surface3D,  ///< 三维绘图引导窗口
    Stats       ///< 统计绘图引导窗口
};

/**
 * @brief 一种图表类型的完整描述
 *
 * 把显示文案、图标、所属窗口、页面创建方式与 item 后置修饰集中在一处。
 * 引导窗口只遍历注册表建列表和路由，不再维护多份同构的 switch。
 * initPlotItem 用来表达「同一种页面、不同图表类型」的样式差异，
 * 例如散点图复用曲线页面，但把线条改为点状。
 */
struct DAChartAddItemWidgetDescriptor
{
    DA::DAChartTypes type { DA::DAChartTypes::Unknow };  ///< 图表类型
    DAChartAddWidgetFamily family { DAChartAddWidgetFamily::Curve2D };  ///< 所属窗口家族
    const char* textContext { nullptr };  ///< 翻译上下文
    const char* textSource { nullptr };   ///< 未翻译源串（QT_TRANSLATE_NOOP 标记）
    const char* iconFile { nullptr };     ///< 图标文件名（位于 :/DAGui/ChartType/icon/chart-type/）
    std::function< DAAbstractChartAddWidgetBase*() > creator;  ///< 页面创建函数
    std::function< void(QwtPlotItem*) > initPlotItem;         ///< 可选的 item 后置修饰
    // 可选：与其它类型共用同一个页面（如散点图复用曲线页面）。
    // 未设置时每个类型各自一个页面。
    DA::DAChartTypes pageType { DA::DAChartTypes::Unknow };

    // 按当前语言翻译后的显示文本
    QString text() const;
    // 列表图标
    QIcon icon() const;
};

/**
 * @brief 图表添加页注册表（单例）
 *
 * 集中登记每种图表类型的元数据，二维/三维/统计三个引导窗口都从它取列表与页面。
 * 新增一种图表只需新增页面类并注册一条描述，不再改动引导窗口的路由逻辑；
 * 插件也可以通过 registerType() 注册自定义图表类型。
 *
 * @code
 * auto& f = DAChartAddItemWidgetFactory::instance();
 * const auto* desc = f.descriptor(DAChartTypes::Curve);
 * if (desc && desc->creator) {
 *     QWidget* page = desc->creator();
 * }
 * @endcode
 */
class DAGUI_API DAChartAddItemWidgetFactory
{
public:
    // 获取注册表单例（首次调用时注册内置类型）
    static DAChartAddItemWidgetFactory& instance();
    // 注册一种图表类型；重复注册同一类型时后者覆盖前者
    void registerType(const DAChartAddItemWidgetDescriptor& desc);
    // 是否已注册
    bool isRegistered(DA::DAChartTypes type) const;
    // 查找描述，未注册返回 nullptr
    const DAChartAddItemWidgetDescriptor* descriptor(DA::DAChartTypes type) const;
    // 按注册顺序返回某一家族的全部描述
    QList< DAChartAddItemWidgetDescriptor > descriptors(DAChartAddWidgetFamily family) const;
    // 按注册顺序返回全部描述
    QList< DAChartAddItemWidgetDescriptor > allDescriptors() const;
    // 是否为三维图表类型
    static bool is3DType(DA::DAChartTypes type);

private:
    DAChartAddItemWidgetFactory();
    ~DAChartAddItemWidgetFactory() = default;
    DAChartAddItemWidgetFactory(const DAChartAddItemWidgetFactory&) = delete;
    DAChartAddItemWidgetFactory& operator=(const DAChartAddItemWidgetFactory&) = delete;
    // 注册内置的二维、三维与统计图表类型
    void registerBuiltinTypes();

private:
    QList< DAChartAddItemWidgetDescriptor > mDescriptors;  ///< 注册顺序即界面显示顺序
};

}  // end DA

#endif  // DACHARTADDITEMWIDGETFACTORY_H
