#ifndef DIALOGPEAKANALYSISSETTING_H
#define DIALOGPEAKANALYSISSETTING_H

#include <QDialog>
#include <QVariantMap>
#include "DAData.h"
#include <QList>

namespace Ui
{
class DialogPeakAnalysisSetting;
}

namespace DA
{
class DADataManager;
class DAPySeriesTableModel;
}

class DialogPeakAnalysisSetting : public QDialog
{
	Q_OBJECT

public:
	explicit DialogPeakAnalysisSetting(QWidget* parent = nullptr);
	~DialogPeakAnalysisSetting();

	void setDataManager(DA::DADataManager* mgr);
	// 设置当前的series
	void setCurrentSeries(const DA::DAPySeries& s);
	// 获取采样率
	double getSamplingRate() const;
	DA::DAPySeries getCurrentSeries() const;
	// 获取峰值分析设置参数
	QVariantMap getPeakAnalysisSetting();

private slots:
	void onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
	void onAccepted();

	void on_checkBoxAuto_toggled(bool checked);

private:
	Ui::DialogPeakAnalysisSetting* ui;
	DA::DAPySeriesTableModel* mModuel { nullptr };
	QList< double > mCount {};
};

#endif  // DIALOGPEAKANALYSISSETTING_H
