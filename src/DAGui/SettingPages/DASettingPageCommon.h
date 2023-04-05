#ifndef DASETTINGPAGECOMMON_H
#define DASETTINGPAGECOMMON_H
#include "DAAbstractSettingPage.h"
#include <QWidget>
#include "DAGuiAPI.h"
namespace Ui
{
class DASettingPageCommon;
}

namespace DA
{

class DAGUI_API DASettingPageCommon : public DAAbstractSettingPage
{
    Q_OBJECT

public:
    explicit DASettingPageCommon(QWidget* parent = nullptr);
    ~DASettingPageCommon();
    //应用设置
    virtual void apply() override;

private:
    Ui::DASettingPageCommon* ui;
};
}
#endif  // DASETTINGPAGECOMMON_H
