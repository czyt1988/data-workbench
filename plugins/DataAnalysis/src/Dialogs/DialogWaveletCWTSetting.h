#ifndef DIALOGWAVELETCWTSETTING_H
#define DIALOGWAVELETCWTSETTING_H

#include <QDialog>
#include <QVariantMap>
#include "DAData.h"

namespace Ui
{
class DialogWaveletCWTSetting;
}

namespace DA
{
class DADataManager;
class DAPySeriesTableModel;
}

class DialogWaveletCWTSetting : public QDialog
{
	Q_OBJECT

public:
	explicit DialogWaveletCWTSetting(QWidget* parent = nullptr);
	~DialogWaveletCWTSetting();

	//
	void setDataManager(DA::DADataManager* mgr);
	// 设置当前的series
	void setCurrentSeries(const DA::DAPySeries& s);
	DA::DAPySeries getCurrentSeries() const;
	//获取scales
	DA::DAPySeries getScalesSeries() const;
	// 获取设置参数
	QVariantMap getWaveletCWTSetting();
	// 获取采样率
	double getSamplingRate() const;

private slots:
	void onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onAccepted();

private:
	Ui::DialogWaveletCWTSetting* ui;
	DA::DAPySeriesTableModel* mModuel { nullptr };
};

#endif  // DIALOGWAVELETCWTSETTING_H
