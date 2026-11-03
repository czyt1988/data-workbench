#ifndef DASTATSPLOTREQUEST_H
#define DASTATSPLOTREQUEST_H
#include "DAGuiAPI.h"
#include "DAFigureAPI.h"
#include "DAData.h"
#include <QString>
#include <QVariantMap>

class DAChartPlotRenderer;
class DAStatsPlotCoordinator;

namespace DA
{
class DAFigureWidget;
class DAChartWidget;
class DAPyDataFrame;

/**
 * @brief 统计绘图请求基类（纯数据消息对象，非 QObject）
 *
 * 替代旧的「plotRequested(QJsonObject params, ...)」字符串键 JSON 契约：
 * 参数以类型化字段携带，字符串键收缩到 toPythonParams() 单点；渲染编排经
 * dispatchTo 双分派衔接 DAStatsPlotCoordinator，分派穷尽性由继承保证。
 */
class DAGUI_API DAStatsPlotRequest
{
public:
    DAStatsPlotRequest() = default;
    virtual ~DAStatsPlotRequest();

    // 图表类型（替代旧 __plot_type__ 魔法键）
    virtual DAChartTypes type() const = 0;
    // 请求是否有效（含 figure/chart 非空、data 为 DataFrame、本类型必需字段非空）
    virtual bool isValid() const = 0;
    // 无效时的用户可读错误信息（国际化）
    virtual QString errorString() const = 0;
    // 喂给 DAPyScriptsStatistics 位置参数 params 的键值（唯一字符串键点，净化输出）
    virtual QVariantMap toPythonParams() const = 0;
    // 双分派到 coordinator 对应的 plotXxx（一行实现）
    virtual void dispatchTo(DAStatsPlotCoordinator& coordinator,
                            DAChartPlotRenderer& renderer,
                            const DAPyDataFrame& df) const = 0;

    // 公共上下文（随请求一并携带）
    DAData data;                       ///< 数据源
    DAFigureWidget* figure { nullptr };  ///< 目标 Figure 窗口
    DAChartWidget* chart { nullptr };    ///< 目标 Chart 窗口

protected:
    // 基类级校验：figure/chart 非空且 data 为 DataFrame
    bool isValidBase() const;
    // 基类级错误信息（全部有效时返回空串，供子类 errorString 复用）
    QString baseErrorString() const;
};

}  // namespace DA

#endif  // DASTATSPLOTREQUEST_H
