#include "DASettingPageCommon.h"
#include "ui_DASettingPageCommon.h"
#include "AppMainWindow.h"
#include "SARibbonBar.h"
#include "DAAppConfig.h"
namespace DA
{

DASettingPageCommon::DASettingPageCommon(QWidget* parent)
    : DAAbstractSettingPage(parent)
    , ui(new Ui::DASettingPageCommon)
    , mPixmapRibbonStandard(":/app/settingpages/Icon/settingpages/ribbon-style-standard-3r.png")
    , mPixmapRibbonStandard2Row(":/app/settingpages/Icon/settingpages/ribbon-style-standard-2r.png")
    , mPixmapRibbonLite(":/app/settingpages/Icon/settingpages/ribbon-style-lite-3r.png")
    , mPixmapRibbonLite2Row(":/app/settingpages/Icon/settingpages/ribbon-style-lite-2r.png")
{
    ui->setupUi(this);
    mButtonGroupRibbonStyle.setExclusive(true);
    mButtonGroupRibbonStyle.addButton(ui->radioButtonStandardStyle, static_cast< int >(SARibbonBar::OfficeStyle));
    mButtonGroupRibbonStyle.addButton(ui->radioButtonStandardStyle2Row, static_cast< int >(SARibbonBar::OfficeStyleTwoRow));
    mButtonGroupRibbonStyle.addButton(ui->radioButtonLiteStyle, static_cast< int >(SARibbonBar::WpsLiteStyle));
    mButtonGroupRibbonStyle.addButton(ui->radioButtonLiteStyle2Row, static_cast< int >(SARibbonBar::WpsLiteStyleTwoRow));
    connect(&mButtonGroupRibbonStyle, QOverload< int >::of(&QButtonGroup::buttonClicked), this, &DASettingPageCommon::onButtonGroupRibbonStyleClicked);
    connect(ui->spinBoxDisplayLogsNum, QOverload< int >::of(&QSpinBox::valueChanged), this, &DASettingPageCommon::onSpinBoxDisplayLogsNumValueChanged);
}

DASettingPageCommon::~DASettingPageCommon()
{
    delete ui;
}

void DASettingPageCommon::apply()
{
    if (nullptr == mAppConfig) {
        return;
    }
    DAAppConfig& cfg                  = *mAppConfig;
    cfg[ DA_CONFIG_KEY_RIBBON_STYLE ] = static_cast< int >(mNewRibbonStyle);
    cfg[ DA_CONFIG_KEY_SHOW_LOG_NUM ] = ui->spinBoxDisplayLogsNum->value();
    cfg.apply();
    emit settingApplyed();
}

QString DASettingPageCommon::getSettingPageTitle() const
{
    return tr("common");
}

QIcon DASettingPageCommon::getSettingPageIcon() const
{
    return QIcon(":/icon/icon/setting-common.svg");
}

bool DASettingPageCommon::setAppConfig(DAAppConfig* p)
{
    if (nullptr == p) {
        return false;
    }
    //名字不符合，跳过
    mAppConfig       = p;
    DAAppConfig& cfg = *p;
    // ribbon style
    SARibbonBar::RibbonStyle ribbonStyle = static_cast< SARibbonBar::RibbonStyle >(cfg[ DA_CONFIG_KEY_RIBBON_STYLE ].toInt());
    mOldRibbonStyle                      = ribbonStyle;
    switch (ribbonStyle) {
    case SARibbonBar::WpsLiteStyle:
        ui->radioButtonLiteStyle->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonLite);
        break;
    case SARibbonBar::WpsLiteStyleTwoRow:
        ui->radioButtonLiteStyle2Row->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonLite2Row);
        break;
    case SARibbonBar::OfficeStyle:
        ui->radioButtonStandardStyle->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonStandard);
        break;
    case SARibbonBar::OfficeStyleTwoRow:
        ui->radioButtonStandardStyle2Row->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonStandard2Row);
        break;
    default:
        break;
    }
    //日志
    bool isOK = false;
    int c     = cfg[ DA_CONFIG_KEY_SHOW_LOG_NUM ].toInt(&isOK);
    if (isOK) {
        ui->spinBoxDisplayLogsNum->setValue(c);
    }
    return true;
}

void DASettingPageCommon::onButtonGroupRibbonStyleClicked(int id)
{
    SARibbonBar::RibbonStyle ribbonStyle = static_cast< SARibbonBar::RibbonStyle >(id);
    switch (ribbonStyle) {
    case SARibbonBar::WpsLiteStyle:
        ui->labelmage->setPixmap(mPixmapRibbonLite);
        break;
    case SARibbonBar::WpsLiteStyleTwoRow:
        ui->labelmage->setPixmap(mPixmapRibbonLite2Row);
        break;
    case SARibbonBar::OfficeStyle:
        ui->labelmage->setPixmap(mPixmapRibbonStandard);
        break;
    case SARibbonBar::OfficeStyleTwoRow:
        ui->labelmage->setPixmap(mPixmapRibbonStandard2Row);
        break;
    default:
        break;
    }
    mNewRibbonStyle = ribbonStyle;
    if (mOldRibbonStyle != mNewRibbonStyle) {
        emit settingChanged();
    }
}

void DASettingPageCommon::onSpinBoxDisplayLogsNumValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

}  // end DA
