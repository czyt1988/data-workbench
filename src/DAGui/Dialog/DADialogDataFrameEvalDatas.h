#ifndef DADIALOGDATAFRAMEEVALDATAS_H
#define DADIALOGDATAFRAMEEVALDATAS_H

#include <QDialog>
#include "DAGuiAPI.h"
namespace Ui
{
class DADialogDataFrameEvalDatas;
}
namespace DA
{

/**
 * @brief evaldatas参数设置
 */
class DAGUI_API DADialogDataFrameEvalDatas : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameEvalDatas(QWidget* parent = nullptr);
	~DADialogDataFrameEvalDatas();
	// 获取输入的条件
    QString getExpr() const;

private:
	Ui::DADialogDataFrameEvalDatas* ui;
};
}

#endif  // DADIALOGDATAFRAMEEVALDATAS_H
