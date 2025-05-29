#ifndef DADIALOGDATAFRAMEDATASEARCH_H
#define DADIALOGDATAFRAMEDATASEARCH_H

#include <QDialog>

namespace Ui
{
class DADialogDataFrameDataSearch;
}

namespace DA
{
class DADialogDataFrameDataSearch : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameDataSearch(QWidget* parent = nullptr);
	~DADialogDataFrameDataSearch();
	// 获取输入的条件
	QString getExpr() const;

private:
	Ui::DADialogDataFrameDataSearch* ui;
};
}

#endif  // DADIALOGDATAFRAMEDATASEARCH_H
