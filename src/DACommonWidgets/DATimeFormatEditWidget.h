#ifndef DATIMEFORMATEDITWIDGET_H
#define DATIMEFORMATEDITWIDGET_H
#include "DACommonWidgetsAPI.h"
#include <QWidget>
#include <QDateTime>
class QComboBox;
namespace Ui
{
class DATimeFormatEditWidget;
}

namespace DA
{

///
/// \brief 时间设置窗口 用于设置时间的格式
///
class DACOMMONWIDGETS_API DATimeFormatEditWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DATimeFormatEditWidget(QWidget* parent = 0);
	~DATimeFormatEditWidget();
	QString getTimeFormat() const;
	void setTimeFormatText(const QString& str);
private slots:
	void onCurrentTextChanged(const QString& text);

private:
	void setComboxTimeFormat(QComboBox* comb);

private:
	Ui::DATimeFormatEditWidget* ui;
	QDateTime m_currentDateTime;
};
}  // end DA
#endif  // DATIMEFORMATEDITWIDGET_H
