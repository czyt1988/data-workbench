#include "DASettingPageCommon.h"
#include <QSignalBlocker>
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
    mButtonGroupRibbonStyle.addButton(ui->radioButtonStandardStyle,
                                      static_cast< int >(SARibbonBar::RibbonStyleLooseThreeRow));
    mButtonGroupRibbonStyle.addButton(ui->radioButtonStandardStyle2Row,
                                      static_cast< int >(SARibbonBar::RibbonStyleLooseTwoRow));
    mButtonGroupRibbonStyle.addButton(ui->radioButtonLiteStyle, static_cast< int >(SARibbonBar::RibbonStyleCompactThreeRow));
    mButtonGroupRibbonStyle.addButton(ui->radioButtonLiteStyle2Row,
                                      static_cast< int >(SARibbonBar::RibbonStyleCompactTwoRow));
#if QT_VERSION_MAJOR >= 6
    connect(&mButtonGroupRibbonStyle, &QButtonGroup::idClicked, this, &DASettingPageCommon::onButtonGroupRibbonStyleClicked);
#else
    connect(&mButtonGroupRibbonStyle,
            QOverload< int >::of(&QButtonGroup::buttonClicked),
            this,
            &DASettingPageCommon::onButtonGroupRibbonStyleClicked);
#endif
    connect(ui->spinBoxDisplayLogsNum,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageCommon::onSpinBoxDisplayLogsNumValueChanged);
    connect(ui->checkBoxSaveUIState, &QCheckBox::stateChanged, this, &DASettingPageCommon::onCheckBoxSaveUIStateStateChanged);
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
    DAAppConfig& cfg = *mAppConfig;
    // 记录旧值
    mOldRibbonStyle = static_cast< SARibbonBar::RibbonStyles >(cfg[ DA_CONFIG_KEY_RIBBON_STYLE ].toInt());
    // 更新
    cfg[ DA_CONFIG_KEY_RIBBON_STYLE ]           = static_cast< int >(mNewRibbonStyle);
    cfg[ DA_CONFIG_KEY_SHOW_LOG_NUM ]           = ui->spinBoxDisplayLogsNum->value();
    cfg[ DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE ] = ui->checkBoxSaveUIState->isChecked();
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
    QSignalBlocker blocker(this);
    // 名字不符合，跳过
    mAppConfig       = p;
    DAAppConfig& cfg = *p;
    // ribbon style
    SARibbonBar::RibbonStyles ribbonStyle = static_cast< SARibbonBar::RibbonStyles >(
        cfg[ DA_CONFIG_KEY_RIBBON_STYLE ].toInt());
    mOldRibbonStyle = ribbonStyle;
    switch (ribbonStyle) {
    case SARibbonBar::RibbonStyleCompactThreeRow:
        ui->radioButtonLiteStyle->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonLite);
        break;
    case SARibbonBar::RibbonStyleCompactTwoRow:
        ui->radioButtonLiteStyle2Row->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonLite2Row);
        break;
    case SARibbonBar::RibbonStyleLooseThreeRow:
        ui->radioButtonStandardStyle->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonStandard);
        break;
    case SARibbonBar::RibbonStyleLooseTwoRow:
        ui->radioButtonStandardStyle2Row->setChecked(true);
        ui->labelmage->setPixmap(mPixmapRibbonStandard2Row);
        break;
    default:
        break;
    }
    // 是否记录ui
    bool isSaveUIState = cfg[ DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE ].toBool();
    ui->checkBoxSaveUIState->setChecked(isSaveUIState);
    // 日志
    bool isOK = false;
    int c     = cfg[ DA_CONFIG_KEY_SHOW_LOG_NUM ].toInt(&isOK);
    if (isOK) {
        ui->spinBoxDisplayLogsNum->setValue(c);
    }

    return true;
}

void DASettingPageCommon::onButtonGroupRibbonStyleClicked(int id)
{
    SARibbonBar::RibbonStyles ribbonStyle = static_cast< SARibbonBar::RibbonStyles >(id);
    switch (ribbonStyle) {
    case SARibbonBar::RibbonStyleCompactThreeRow:
        ui->labelmage->setPixmap(mPixmapRibbonLite);
        break;
    case SARibbonBar::RibbonStyleCompactTwoRow:
        ui->labelmage->setPixmap(mPixmapRibbonLite2Row);
        break;
    case SARibbonBar::RibbonStyleLooseThreeRow:
        ui->labelmage->setPixmap(mPixmapRibbonStandard);
        break;
    case SARibbonBar::RibbonStyleLooseTwoRow:
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

void DASettingPageCommon::onCheckBoxSaveUIStateStateChanged(int state)
{
    Q_UNUSED(state);
    emit settingChanged();
}

}  // end DA
