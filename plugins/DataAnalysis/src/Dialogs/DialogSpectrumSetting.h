#ifndef DIALOGSPECTRUMSETTING_H
#define DIALOGSPECTRUMSETTING_H

#include <QDialog>
#include <QVariantMap>
#include "DAData.h"
namespace Ui
{
class DialogSpectrumSetting;
}

namespace DA
{
class DADataManager;
class DAPySeriesTableModule;
}
class DialogSpectrumSetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSpectrumSetting(QWidget* parent = nullptr);
    ~DialogSpectrumSetting();
    //
    void setDataManager(DA::DADataManager* mgr);
    // 设置当前的series
    void setCurrentSeries(const DA::DAPySeries& s);
    DA::DAPySeries getCurrentSeries() const;
    // 获取设置参数
    QVariantMap getSpectrumSetting();
    // 获取采样率
    double getSamplingRate() const;
private slots:
    void onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onAccepted();

private:
    Ui::DialogSpectrumSetting* ui;
    DA::DAPySeriesTableModule* mModuel { nullptr };
};

#endif  // DIALOGSPECTRUMSETTING_H
