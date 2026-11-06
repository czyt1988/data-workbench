#ifndef DASIGNALFORMSPECBUILDER_H
#define DASIGNALFORMSPECBUILDER_H
#include <QJsonObject>
#include "Analysis/DASignalAnalysisParams.h"

namespace DA
{
/**
 * @brief 用 DAFormSpec 构造分析参数表单与联动规则（方案 §8.6 / D20）
 *
 * 分组用可折叠分组而非页签；联动用框架的 visible_when 表达（不手写信号槽）：
 * - 峰值模式选「简单」时，高级三个字段隐藏；
 * - 窗函数只暴露一个字段（频谱与 STFT 共用口径，避免两个真相）。
 *
 * 字段名与 DASignalAnalysisParams 的 JSON 键名一一对应。
 */
class DASignalFormSpecBuilder
{
public:
    // 构造完整表单 JSON：current 作为字段默认值（记住上次值）；derivedResolutionHz 用于最小间距默认值 = 5 倍分辨率
    static QJsonObject build(const DASignalAnalysisParams& current, double derivedResolutionHz = 0.0);
};
}  // namespace DA

#endif  // DASIGNALFORMSPECBUILDER_H
