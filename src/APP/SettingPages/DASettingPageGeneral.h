#ifndef DASETTINGPAGEGENERAL_H
#define DASETTINGPAGEGENERAL_H
#include "DAAbstractSettingPage.h"
#include <QWidget>
#include "DAGuiAPI.h"
#include <QButtonGroup>
#include <QFont>
#include "SARibbonBar.h"
namespace Ui
{
class DASettingPageGeneral;
}

namespace DA
{
class DAAppConfig;

/**
 * @brief 通用设置页：语言、Ribbon 样式/主题、应用字体、退出保存UI状态
 */
class DASettingPageGeneral : public DAAbstractSettingPage
{
    Q_OBJECT

public:
    explicit DASettingPageGeneral(QWidget* parent = nullptr);
    virtual ~DASettingPageGeneral() override;
    // 应用设置
    virtual void apply() override;
    // 设置页的标题
    virtual QString getSettingPageTitle() const override;
    // 设置页的图标
    virtual QIcon getSettingPageIcon() const override;
    // 设置配置
    bool setAppConfig(DAAppConfig* p);
private Q_SLOTS:
    void onButtonGroupRibbonStyleClicked(int id);
    void onComboBoxRibbonThemeCurrentIndexChanged(int index);
    void onComboBoxLanguageCurrentIndexChanged(int index);
    void onComboBoxDockTabPositionCurrentIndexChanged(int index);
    void onFontComboBoxAppFontCurrentFontChanged(const QFont& font);
    void onSpinBoxFontSizeValueChanged(int v);
    void onCheckBoxSaveUIStateChanged(int state);
    void onToolButtonClearSaveStateClicked();

private:
    // 填充 Ribbon 主题下拉
    void fillRibbonThemeCombo();
    // 填充语言下拉
    void fillLanguageCombo();
    // 填充 dock 标签页方位下拉
    void fillDockTabPositionCombo();

private:
    Ui::DASettingPageGeneral* ui;
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
#endif  // DASETTINGPAGEGENERAL_H
