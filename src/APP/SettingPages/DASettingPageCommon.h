#ifndef DASETTINGPAGECOMMON_H
#define DASETTINGPAGECOMMON_H
#include "DAAbstractSettingPage.h"
#include <QWidget>
#include "DAGuiAPI.h"
#include <QButtonGroup>
#include "SARibbonBar.h"
namespace Ui
{
class DASettingPageCommon;
}

namespace DA
{
class DAAppConfig;
class AppMainWindow;

class DASettingPageCommon : public DAAbstractSettingPage
{
    Q_OBJECT

public:
    explicit DASettingPageCommon(QWidget* parent = nullptr);
    ~DASettingPageCommon();
    // 应用设置
    virtual void apply() override;
    // 设置页的标题，此函数影响DASettingWidget的listwidget的显示
    virtual QString getSettingPageTitle() const override;
    // 设置页的图标,此函数影响DASettingWidget的listwidget的显示
    virtual QIcon getSettingPageIcon() const override;
    // 设置配置
    bool setAppConfig(DAAppConfig* p);
private slots:
    void onButtonGroupRibbonStyleClicked(int id);
    // 日志数量改变
    void onSpinBoxDisplayLogsNumValueChanged(int v);
    // 程序在退出时是否保存ui的状态
    void onCheckBoxSaveUIStateStateChanged(int state);
    // 清除状态按钮点击
    void onToolButtonClearSaveStateClicked();

private:
    Ui::DASettingPageCommon* ui;
    DAAppConfig* mAppConfig { nullptr };
    QPixmap mPixmapRibbonStandard;
    QPixmap mPixmapRibbonStandard2Row;
    QPixmap mPixmapRibbonLite;
    QPixmap mPixmapRibbonLite2Row;
    QButtonGroup mButtonGroupRibbonStyle;
    SARibbonBar::RibbonStyles mOldRibbonStyle { SARibbonBar::RibbonStyleCompactTwoRow };
    SARibbonBar::RibbonStyles mNewRibbonStyle { SARibbonBar::RibbonStyleCompactTwoRow };
};
}
#endif  // DASETTINGPAGECOMMON_H
