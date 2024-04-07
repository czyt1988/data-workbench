#ifndef DIALOGSPECTRUMSETTING_H
#define DIALOGSPECTRUMSETTING_H

#include <QDialog>

namespace Ui
{
class DialogSpectrumSetting;
}

class DialogSpectrumSetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSpectrumSetting(QWidget* parent = nullptr);
    ~DialogSpectrumSetting();

private:
    Ui::DialogSpectrumSetting* ui;
};

#endif  // DIALOGSPECTRUMSETTING_H
