#ifndef DADIALOGDATAFRAMEQUERYDATAS_H
#define DADIALOGDATAFRAMEQUERYDATAS_H

#include <QDialog>
#include "DAGuiAPI.h"
namespace Ui
{
class DADialogDataFrameQueryDatas;
}
namespace DA
{

/**
 * @brief querydatas参数设置
 */
class DAGUI_API DADialogDataFrameQueryDatas : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameQueryDatas(QWidget* parent = nullptr);
	~DADialogDataFrameQueryDatas();
	// 获取输入的条件
    QString getExpr() const;

private:
	Ui::DADialogDataFrameQueryDatas* ui;
};
}

#endif  // DADIALOGDATAFRAMEQUERYDATAS_H
