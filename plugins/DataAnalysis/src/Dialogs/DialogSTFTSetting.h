#ifndef DIALOGSTFTSETTING_H
#define DIALOGSTFTSETTING_H
#include <QDialog>
#include <QVariantMap>
#include "DAData.h"
namespace Ui
{
class DialogSTFTSetting;
}

namespace DA
{
class DADataManager;
class DAPySeriesTableModel;
}
class DialogSTFTSetting : public QDialog
{
	Q_OBJECT

public:
	explicit DialogSTFTSetting(QWidget* parent = nullptr);
	~DialogSTFTSetting();
	//
	void setDataManager(DA::DADataManager* mgr);
	// 设置当前的series
	void setCurrentSeries(const DA::DAPySeries& s);
	DA::DAPySeries getCurrentSeries() const;
	// 获取设置参数
	QVariantMap getSTFTSetting();
	// 获取采样率
	double getSamplingRate() const;
private slots:
	void onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onAccepted();

private:
	Ui::DialogSTFTSetting* ui;
	DA::DAPySeriesTableModel* mModuel{ nullptr };
};

#endif  // DIALOGSTFTSETTING_H
