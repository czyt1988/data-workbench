#ifndef DACHARTERRORBARITEMSETTINGWIDGET_H
#define DACHARTERRORBARITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include "DAAbstractChartItemSettingWidget.h"

// Qt
class QAbstractButton;
// qwt
class QwtPlotItem;
class QwtPlotIntervalCurve;
class QwtIntervalSymbol;

namespace Ui
{
class DAChartErrorBarItemSettingWidget;
}

namespace DA
{
class DAChartPlotItemSettingWidget;
/**
 * @brief 曲线设置窗口
 *
 * @note 注意此窗口不保存item
 */
class DAGUI_API DAChartErrorBarItemSettingWidget : public DAAbstractChartItemSettingWidget
{
    Q_OBJECT

public:
	explicit DAChartErrorBarItemSettingWidget(QWidget* parent = nullptr);
	~DAChartErrorBarItemSettingWidget();
	// item设置了
	virtual void plotItemSet(QwtPlotItem* item) override;
	// 根据QwtPlotCurve更新ui
	void updateUI(const QwtPlotIntervalCurve* item);
	// 根据ui更新plotitem
    void updateItemFromUI(QwtPlotIntervalCurve* item);
    void updateSymbolFromUI(QwtPlotIntervalCurve* item);
	// 标题
	void setTitle(const QString& t);
	QString getTitle() const;
	// maker编辑
	void enableErrorBarEdit(bool on = true);
	bool isEnableErrorBarEdit() const;
	// fill编辑
	void enableFillEdit(bool on = true);
	bool isEnableFillEdit() const;
	// 画笔
	QPen getCurvePen() const;
	// 填充
	QBrush getFillBrush() const;
	// 方向
	void setOrientation(Qt::Orientation v);
	Qt::Orientation getOrientation() const;
	// 清空界面
	void resetUI();
	// 获取itemplot widget
	DAChartPlotItemSettingWidget* getItemSettingWidget() const;
	// 重ui设置创建QwtIntervalSymbol
	QwtIntervalSymbol* createIntervalSymbolFromUI();
public slots:
	// 画笔
	void setCurvePen(const QPen& v);
	// 填充
	void setFillBrush(const QBrush& v);

private slots:
	void onGroupBoxErrorBarEnable(bool checked);
	void onGroupBoxFillEnable(bool checked);
	void onGroupBoxPenEnable(bool checked);
    void onErrorBarPenWidthChanged(int v);
	void onBrushChanged(const QBrush& b);
	void onButtonGroupOrientationClicked(QAbstractButton* b);
	void onCurvePenChanged(const QPen& p);
    void onPenEditWidgetToErrorBarChanged(const QPen& p);
    void onBrushEditWidgetToErrorBarChanged(const QBrush& b);
    void onBarStyleButtonClicked(QAbstractButton* btn);
protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);

private:
	Ui::DAChartErrorBarItemSettingWidget* ui;
};
}

#endif  // DACHARTERRORBARITEMSETTINGWIDGET_H
