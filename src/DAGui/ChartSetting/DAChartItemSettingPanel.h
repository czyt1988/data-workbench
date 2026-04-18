#ifndef DACHARTITEMSETTINGPANEL_H
#define DACHARTITEMSETTINGPANEL_H
#include "DAGuiAPI.h"
#include "DAAbstractChartItemSettingWidget.h"
// Qwt（基类已引入qwt_plot.h，此处引入完整类型定义）
#include "qwt_plot_curve.h"
#include "qwt_axis.h"
#include "qwt_symbol.h"
#include "DAChartSymbolEditWidget.h"
// Qt forward declarations
class QComboBox;
class QRadioButton;
class QButtonGroup;
class QAbstractButton;

namespace DA
{
class DAPropertyPanelWidget;

/**
 * @brief Qwt专有图表项属性设置面板基类
 *
 * 继承DAAbstractChartItemSettingWidget，内部持有DAPropertyPanelWidget，
 * 在通用属性便捷方法之上叠加Qwt专有属性创建方法和值读写方法。
 *
 * 使用方式：
 * 1. 继承此类，实现buildPropertyPanel()
 * 2. 在buildPropertyPanel()中调用addXxxProperty方法构建面板
 * 3. 通过propertyPanel()调用DAPropertyPanelWidget通用便捷方法
 * 4. 使用get/setXxxValue进行Qwt专有类型的值读写
 *
 * @code
 * class MyCurveSettingPanel : public DAChartItemSettingPanel
 * {
 *     Q_OBJECT
 * public:
 *     void buildPropertyPanel() override
 *     {
 *         addCurveStyleProperty(1, tr("曲线样式"));
 *         addSymbolProperty(2, tr("标记"));
 *     }
 * };
 * @endcode
 *
 * @see DAAbstractChartItemSettingWidget
 * @see DAPropertyPanelWidget
 */
class DAGUI_API DAChartItemSettingPanel : public DAAbstractChartItemSettingWidget
{
	Q_OBJECT
public:
	explicit DAChartItemSettingPanel(QWidget* parent = nullptr);
	~DAChartItemSettingPanel() override;

	// 获取通用的属性面板，子类可通过此指针调用DAPropertyPanelWidget的通用便捷方法
	DAPropertyPanelWidget* propertyPanel() const;

	// === Qwt专有属性创建方法 ===

	// 添加曲线样式属性（QComboBox，填充QwtPlotCurve::CurveStyle枚举：Lines=1,Sticks=2,Steps=3,Dots=4,NoCurve=5）
	void addCurveStyleProperty(int id, const QString& name);

	// 添加方向属性（Horizontal/Vertical两个QRadioButton + QButtonGroup）
	void addOrientationProperty(int id, const QString& name);

	// 添加坐标轴属性（QComboBox，根据isYAxis填充Y轴(YLeft=0,YRight=1)或X轴(XBottom=2,XTop=3)）
	void addAxisProperty(int id, const QString& name, bool isYAxis = true);

	// 添加标记属性（DAChartSymbolEditWidget编辑器，BelowLayout）
	void addSymbolProperty(int id, const QString& name);

	// 添加刻度样式属性（Normal/DateTime两个QRadioButton + QButtonGroup）
	void addScaleStyleProperty(int id, const QString& name);

	// === Qwt专有值读写方法 ===

	// 获取/设置曲线样式值（通过ComboBox的currentData）
	QwtPlotCurve::CurveStyle getCurveStyleValue(int id) const;
	void setCurveStyleValue(int id, QwtPlotCurve::CurveStyle style);

	// 获取/设置方向值（通过QButtonGroup）
	Qt::Orientation getOrientationValue(int id) const;
	void setOrientationValue(int id, Qt::Orientation orientation);

	// 获取/设置坐标轴值（通过ComboBox的currentData）
	QwtAxis::Position getAxisValue(int id) const;
	void setAxisValue(int id, QwtAxis::Position axisId);

	// 获取标记编辑器Widget
	DAChartSymbolEditWidget* getSymbolWidget(int id) const;

	// 获取/设置刻度样式值（通过QButtonGroup）
	int getScaleStyleValue(int id) const;
	void setScaleStyleValue(int id, int scaleStyle);

Q_SIGNALS:
	/**
	 * @brief Qwt专有属性值变化信号
	 * @param propertyId 属性ID
	 * @note 此信号转发自DAPropertyPanelWidget::propertyValueChanged
	 */
	void propertyValueChanged(int propertyId);

protected:
	// 纯虚函数，子类在此构建面板布局（子类构造函数末尾自行调用）
	virtual void buildPropertyPanel() = 0;

protected Q_SLOTS:
	// 转发DAPropertyPanelWidget::propertyValueChanged到propertyValueChanged信号
	void onPanelPropertyValueChanged(int propertyId);

private:
	DAPropertyPanelWidget* mPanel;
	QMap<int, QButtonGroup*> mButtonGroupMap;     // 方向属性QButtonGroup
	QMap<int, QButtonGroup*> mScaleStyleGroupMap; // 刻度样式属性QButtonGroup
};
}  // end namespace DA

#endif  // DACHARTITEMSETTINGPANEL_H
