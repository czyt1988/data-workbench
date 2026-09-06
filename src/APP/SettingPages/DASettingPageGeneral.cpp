#include "DASettingPageGeneral.h"
#include <QSignalBlocker>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <QSet>
#include "ui_DASettingPageGeneral.h"
#include "AppMainWindow.h"
#include "SARibbonBar.h"
#include "SARibbonGlobal.h"
#include "SARibbonMainWindow.h"
#include "DAAppConfig.h"
#include "DATranslatorManeger.h"
#include "DALogCategory.h"
namespace DA
{

DASettingPageGeneral::DASettingPageGeneral(QWidget* parent)
    : DAAbstractSettingPage(parent)
    , ui(new Ui::DASettingPageGeneral)
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&mButtonGroupRibbonStyle, &QButtonGroup::idClicked, this, &DASettingPageGeneral::onButtonGroupRibbonStyleClicked);
#else
    connect(&mButtonGroupRibbonStyle,
            QOverload< int >::of(&QButtonGroup::buttonClicked),
            this,
            &DASettingPageGeneral::onButtonGroupRibbonStyleClicked);
#endif
    connect(ui->comboBoxRibbonTheme,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASettingPageGeneral::onComboBoxRibbonThemeCurrentIndexChanged);
    connect(ui->comboBoxLanguage,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASettingPageGeneral::onComboBoxLanguageCurrentIndexChanged);
    connect(ui->comboBoxDockTabPosition,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASettingPageGeneral::onComboBoxDockTabPositionCurrentIndexChanged);
    connect(ui->fontComboBoxAppFont,
            &QFontComboBox::currentFontChanged,
            this,
            &DASettingPageGeneral::onFontComboBoxAppFontCurrentFontChanged);
    connect(ui->spinBoxFontSize,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageGeneral::onSpinBoxFontSizeValueChanged);
    connect(ui->checkBoxSaveUIState, &QCheckBox::stateChanged, this, &DASettingPageGeneral::onCheckBoxSaveUIStateChanged);
    connect(ui->toolButtonClearSaveState, &QToolButton::clicked, this, &DASettingPageGeneral::onToolButtonClearSaveStateClicked);
    // 填充下拉
    fillRibbonThemeCombo();
    fillLanguageCombo();
    fillDockTabPositionCombo();
    // 字号默认
    ui->spinBoxFontSize->setRange(6, 72);
    ui->spinBoxFontSize->setValue(QApplication::font().pointSize() > 0 ? QApplication::font().pointSize() : 9);
}

DASettingPageGeneral::~DASettingPageGeneral()
{
    delete ui;
}

void DASettingPageGeneral::fillRibbonThemeCombo()
{
    ui->comboBoxRibbonTheme->blockSignals(true);
    ui->comboBoxRibbonTheme->clear();
    ui->comboBoxRibbonTheme->addItem(tr("Windows 7"), static_cast< int >(SARibbonTheme::RibbonThemeWindows7));  // cn:Windows 7
    ui->comboBoxRibbonTheme->addItem(tr("Office 2013"), static_cast< int >(SARibbonTheme::RibbonThemeOffice2013));  // cn:Office 2013
    ui->comboBoxRibbonTheme->addItem(tr("Office 2016 Blue"), static_cast< int >(SARibbonTheme::RibbonThemeOffice2016Blue));  // cn:Office 2016 蓝色
    ui->comboBoxRibbonTheme->addItem(tr("Office 2016 Green"), static_cast< int >(SARibbonTheme::RibbonThemeOffice2016Green));  // cn:Office 2016 绿色
    ui->comboBoxRibbonTheme->addItem(tr("Office 2016 Dark"), static_cast< int >(SARibbonTheme::RibbonThemeOffice2016Dark));  // cn:Office 2016 深色
    ui->comboBoxRibbonTheme->addItem(tr("Office 2021 Blue"), static_cast< int >(SARibbonTheme::RibbonThemeOffice2021Blue));  // cn:Office 2021 蓝色
    ui->comboBoxRibbonTheme->addItem(tr("Office 2021 Green"), static_cast< int >(SARibbonTheme::RibbonThemeOffice2021Green));  // cn:Office 2021 绿色
    ui->comboBoxRibbonTheme->addItem(tr("Office 2021 Dark"), static_cast< int >(SARibbonTheme::RibbonThemeOffice2021Dark));  // cn:Office 2021 深色
    ui->comboBoxRibbonTheme->addItem(tr("Dark"), static_cast< int >(SARibbonTheme::RibbonThemeDark));  // cn:深色
    ui->comboBoxRibbonTheme->addItem(tr("Dark 2"), static_cast< int >(SARibbonTheme::RibbonThemeDark2));  // cn:深色2
    ui->comboBoxRibbonTheme->blockSignals(false);
}

void DASettingPageGeneral::fillLanguageCombo()
{
    ui->comboBoxLanguage->blockSignals(true);
    ui->comboBoxLanguage->clear();
    // 0: 跟随系统
    ui->comboBoxLanguage->addItem(tr("System"), QString());  // cn:跟随系统
    // 扫描内置翻译文件，提取可用语言代码
    const QList< QString > paths = DA::DATranslatorManeger::getDefaultTranslatorFilePath();
    QSet< QString > langs;
    for (const QString& p : paths) {
        QString base = QFileInfo(p).baseName();  // 如 da_en_US
        if (base.startsWith("da_")) {
            QString lang = base.mid(3);  // en_US
            if (!langs.contains(lang)) {
                langs.insert(lang);
                ui->comboBoxLanguage->addItem(lang, lang);
            }
        }
    }
    ui->comboBoxLanguage->blockSignals(false);
}

void DASettingPageGeneral::fillDockTabPositionCombo()
{
    ui->comboBoxDockTabPosition->blockSignals(true);
    ui->comboBoxDockTabPosition->clear();
    ui->comboBoxDockTabPosition->addItem(tr("Top"), QStringLiteral("top"));       // cn:上方
    ui->comboBoxDockTabPosition->addItem(tr("Bottom"), QStringLiteral("bottom"));  // cn:下方
    ui->comboBoxDockTabPosition->blockSignals(false);
}

void DASettingPageGeneral::apply()
{
    if (nullptr == mAppConfig) {
        return;
    }
    DAAppConfig& cfg = *mAppConfig;
    // 记录旧值
    mOldRibbonStyle = static_cast< SARibbonBar::RibbonStyles >(cfg[ DA_CONFIG_KEY_RIBBON_STYLE ].toInt());
    // 更新
    cfg[ DA_CONFIG_KEY_RIBBON_STYLE ]           = static_cast< int >(mNewRibbonStyle);
    cfg[ DA_CONFIG_KEY_RIBBON_THEME ]            = ui->comboBoxRibbonTheme->currentData().toInt();
    cfg[ DA_CONFIG_KEY_LANGUAGE ]                = ui->comboBoxLanguage->currentData().toString();
    cfg[ DA_CONFIG_KEY_DOCK_TAB_POSITION ]       = ui->comboBoxDockTabPosition->currentData().toString();
    QFont f                                       = ui->fontComboBoxAppFont->currentFont();
    cfg[ DA_CONFIG_KEY_APP_FONT_FAMILY ]        = f.family();
    cfg[ DA_CONFIG_KEY_APP_FONT_POINT_SIZE ]    = ui->spinBoxFontSize->value();
    cfg[ DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE ] = ui->checkBoxSaveUIState->isChecked();
    cfg.apply();
    emit settingApplyed();
}

QString DASettingPageGeneral::getSettingPageTitle() const
{
    return tr("General");  // cn:通用
}

QIcon DASettingPageGeneral::getSettingPageIcon() const
{
    return QIcon(":/DAGui/icon/setting-common.svg");
}

bool DASettingPageGeneral::setAppConfig(DAAppConfig* p)
{
    if (nullptr == p) {
        return false;
    }
    QSignalBlocker blocker(this);
    mAppConfig       = p;
    DAAppConfig& cfg = *p;
    // ribbon style
    SARibbonBar::RibbonStyles ribbonStyle =
        static_cast< SARibbonBar::RibbonStyles >(cfg[ DA_CONFIG_KEY_RIBBON_STYLE ].toInt());
    mOldRibbonStyle = ribbonStyle;
    mNewRibbonStyle = ribbonStyle;
    switch (ribbonStyle) {
    case SARibbonBar::RibbonStyleCompactThreeRow:
        ui->radioButtonLiteStyle->setChecked(true);
        ui->labelImage->setPixmap(mPixmapRibbonLite);
        break;
    case SARibbonBar::RibbonStyleCompactTwoRow:
        ui->radioButtonLiteStyle2Row->setChecked(true);
        ui->labelImage->setPixmap(mPixmapRibbonLite2Row);
        break;
    case SARibbonBar::RibbonStyleLooseThreeRow:
        ui->radioButtonStandardStyle->setChecked(true);
        ui->labelImage->setPixmap(mPixmapRibbonStandard);
        break;
    case SARibbonBar::RibbonStyleLooseTwoRow:
        ui->radioButtonStandardStyle2Row->setChecked(true);
        ui->labelImage->setPixmap(mPixmapRibbonStandard2Row);
        break;
    default:
        break;
    }
    // ribbon theme
    int theme = cfg[ DA_CONFIG_KEY_RIBBON_THEME ].toInt();
    int tidx  = ui->comboBoxRibbonTheme->findData(theme);
    if (tidx >= 0) {
        ui->comboBoxRibbonTheme->setCurrentIndex(tidx);
    }
    // language
    QString lang = cfg[ DA_CONFIG_KEY_LANGUAGE ].toString();
    int lidx    = ui->comboBoxLanguage->findData(lang);
    ui->comboBoxLanguage->setCurrentIndex(lidx >= 0 ? lidx : 0);
    // dock 标签页方位（空值/非法值回落到默认"下方"）
    QString tabPos = cfg[ DA_CONFIG_KEY_DOCK_TAB_POSITION ].toString();
    int pidx      = ui->comboBoxDockTabPosition->findData(tabPos);
    if (pidx < 0) {
        pidx = ui->comboBoxDockTabPosition->findData(QStringLiteral("bottom"));
    }
    ui->comboBoxDockTabPosition->setCurrentIndex(pidx);
    // font
    QFont f;
    QString fam = cfg[ DA_CONFIG_KEY_APP_FONT_FAMILY ].toString();
    if (!fam.isEmpty()) {
        f.setFamily(fam);
    }
    ui->fontComboBoxAppFont->setCurrentFont(f);
    double sz = cfg[ DA_CONFIG_KEY_APP_FONT_POINT_SIZE ].toDouble();
    ui->spinBoxFontSize->setValue(sz > 0 ? static_cast< int >(sz)
                                         : (QApplication::font().pointSize() > 0 ? QApplication::font().pointSize() : 9));
    // 是否记录ui
    bool isSaveUIState = cfg[ DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE ].toBool();
    ui->checkBoxSaveUIState->setChecked(isSaveUIState);

    return true;
}

void DASettingPageGeneral::onButtonGroupRibbonStyleClicked(int id)
{
    SARibbonBar::RibbonStyles ribbonStyle = static_cast< SARibbonBar::RibbonStyles >(id);
    switch (ribbonStyle) {
    case SARibbonBar::RibbonStyleCompactThreeRow:
        ui->labelImage->setPixmap(mPixmapRibbonLite);
        break;
    case SARibbonBar::RibbonStyleCompactTwoRow:
        ui->labelImage->setPixmap(mPixmapRibbonLite2Row);
        break;
    case SARibbonBar::RibbonStyleLooseThreeRow:
        ui->labelImage->setPixmap(mPixmapRibbonStandard);
        break;
    case SARibbonBar::RibbonStyleLooseTwoRow:
        ui->labelImage->setPixmap(mPixmapRibbonStandard2Row);
        break;
    default:
        break;
    }
    mNewRibbonStyle = ribbonStyle;
    if (mOldRibbonStyle != mNewRibbonStyle) {
        emit settingChanged();
    }
}

void DASettingPageGeneral::onComboBoxRibbonThemeCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    emit settingChanged();
}

void DASettingPageGeneral::onComboBoxLanguageCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    emit settingChanged();
}

void DASettingPageGeneral::onComboBoxDockTabPositionCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    emit settingChanged();
}

void DASettingPageGeneral::onFontComboBoxAppFontCurrentFontChanged(const QFont& font)
{
    Q_UNUSED(font);
    emit settingChanged();
}

void DASettingPageGeneral::onSpinBoxFontSizeValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

void DASettingPageGeneral::onCheckBoxSaveUIStateChanged(int state)
{
    Q_UNUSED(state);
    emit settingChanged();
}

/**
 * @brief 把保存文件删除
 */
void DASettingPageGeneral::onToolButtonClearSaveStateClicked()
{
    auto btn =
        QMessageBox::question(this,
                              tr("Question"),  // cn:疑问
                              tr("This operation will delete the file that records the window state information. After "
                                 "deleting the file, if the window state information recording is not enabled, the "
                                 "window will open in the default layout"));  // cn:此操作将删除记录窗口位置信息的文件，删除文件后，如果不开启窗口位置信息记录，窗口将以默认布局打开
    if (btn != QMessageBox::Yes) {
        return;
    }
    if (AppMainWindow::removeStateSettingFile()) {
        daInfo << tr("Successfully removed window state record file");  // cn:成功删除窗口状态记录文件
    }
}

}  // end DA
