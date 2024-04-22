#ifndef DAALIGMENTPOSITIONEDITWIDGET_H
#define DAALIGMENTPOSITIONEDITWIDGET_H

#include <QWidget>
#include "DACommonWidgetsAPI.h"
namespace Ui
{
class DAAligmentPositionEditWidget;
}
namespace DA
{
/**
 * @brief The DAAligmentPositionEditWidget class
 */
class DACOMMONWIDGETS_API DAAligmentPositionEditWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DAAligmentPositionEditWidget(QWidget* parent = nullptr);
	~DAAligmentPositionEditWidget();
	// 设置aligment
	Qt::Alignment getAligmentPosition() const;
    // 把输入的al进行过滤，得到本控件支持的组合
    static Qt::Alignment acceptAligment(Qt::Alignment al);
public slots:
	void setAligmentPosition(Qt::Alignment al);
signals:
	void aligmentPositionChanged(Qt::Alignment al);

protected:
	void changeEvent(QEvent* e);
private slots:
    void onButtonClicked(int id);

private:
	Ui::DAAligmentPositionEditWidget* ui;
};
}  // end DA
#endif  // DAALIGMENTPOSITIONEDITWIDGET_H
