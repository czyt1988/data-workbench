#ifndef DIALOGWAVELETDWTSETTING_H
#define DIALOGWAVELETDWTSETTING_H

#include <QDialog>
#include <QVariantMap>
#include "DAData.h"

namespace Ui
{
class DialogWaveletDWTSetting;
}

namespace DA
{
class DADataManager;
class DAPySeriesTableModule;
}

class DialogWaveletDWTSetting : public QDialog
{
	Q_OBJECT

public:
	explicit DialogWaveletDWTSetting(QWidget* parent = nullptr);
	~DialogWaveletDWTSetting();

	//
	void setDataManager(DA::DADataManager* mgr);
	// 设置当前的series
	void setCurrentSeries(const DA::DAPySeries& s);
	DA::DAPySeries getCurrentSeries() const;
	//获取scales
	DA::DAPySeries getScalesSeries() const;
	// 获取设置参数
	QVariantMap getWaveletDWTSetting();
	// 获取采样率
	double getSamplingRate() const;

private slots:
	void onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onAccepted();

private:
	Ui::DialogWaveletDWTSetting* ui;
	DA::DAPySeriesTableModule* mModuel { nullptr };
};

#endif  // DIALOGWAVELETDWTSETTING_H
