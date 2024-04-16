#ifndef DAALIGMENTEDITWIDGET_H
#define DAALIGMENTEDITWIDGET_H

#include <QWidget>
#include "DACommonWidgetsAPI.h"
class QAbstractButton;
namespace Ui
{
class DAAligmentEditWidget;
}
namespace DA
{
/**
 * @brief 对齐编辑窗口
 */
class DACOMMONWIDGETS_API DAAligmentEditWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DAAligmentEditWidget(QWidget* parent = nullptr);
	~DAAligmentEditWidget();
	void setCurrentAlignment(Qt::Alignment al);
signals:
	/**
	 * @brief 对齐发生变化
	 * @param al
	 */
	void alignmentChanged(Qt::Alignment al);

protected:
	void changeEvent(QEvent* e);
protected slots:
	void onButtonGroupClicked(int id);

private:
	Ui::DAAligmentEditWidget* ui;
};
}  // end da
#endif  // DAALIGMENTEDITWIDGET_H
