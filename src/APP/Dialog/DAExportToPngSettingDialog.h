#ifndef DAEXPORTTOPNGSETTINGDIALOG_H
#define DAEXPORTTOPNGSETTINGDIALOG_H

#include <QDialog>
class QAbstractButton;
namespace Ui
{
class DAExportToPngSettingDialog;
}

namespace DA
{

class DAExportToPngSettingDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DAExportToPngSettingDialog(QWidget* parent = nullptr);
	~DAExportToPngSettingDialog();
	// 获取保存的文件路径
	QString getSelectSaveFilePath() const;
	// 获取dpi
	int getDPI() const;
private slots:
	void onButtonGroupDPITypeButtonClicked(QAbstractButton* button);
	void onPushButtonExportClicked();

private:
	Ui::DAExportToPngSettingDialog* ui;
	QString mSaveFilePath;
};
}
#endif  // DAEXPORTTOPNGSETTINGDIALOG_H
