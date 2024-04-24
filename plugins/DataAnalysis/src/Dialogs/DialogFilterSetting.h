#ifndef DIALOGFILTERSETTING_H
#define DIALOGFILTERSETTING_H

#include <QDialog>
#include <QVariantMap>
#include "DAData.h"

namespace Ui
{
class DialogFilterSetting;
}

namespace DA
{
class DADataManager;
class DAPySeriesTableModule;
}

class DialogFilterSetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFilterSetting(QWidget* parent = nullptr);
    ~DialogFilterSetting();

    void initFilterTypeSetting();

    void setDataManager(DA::DADataManager* mgr);
    // 设置当前的series
    void setCurrentSeries(const DA::DAPySeries& s);
    DA::DAPySeries getCurrentSeries() const;
    // 获取设置参数
    QVariantMap getFilterSetting();
    // 获取采样率
    double getSamplingRate() const;
    //获取滤波器阶数
    int getFilterorder() const;

private slots:
    void onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onAccepted();

private:
    Ui::DialogFilterSetting* ui;
    DA::DAPySeriesTableModule* mModuel { nullptr };
};

#endif  // DIALOGFILTERSETTING_H
