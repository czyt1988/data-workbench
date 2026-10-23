#include "DASettingPageGeneral.h"
#include <QSignalBlocker>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <QPixmap>
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
    , mIconRibbonStandard(":/app/settingpages/Icon/settingpages/ribbon-style-standard-3r.svg")
    , mIconRibbonStandard2Row(":/app/settingpages/Icon/settingpages/ribbon-style-standard-2r.svg")
    , mIconRibbonLite(":/app/settingpages/Icon/settingpages/ribbon-style-lite-3r.svg")
    , mIconRibbonLite2Row(":/app/settingpages/Icon/settingpages/ribbon-style-lite-2r.svg")
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

void DASettingPageGeneral::setRibbonStylePreview(const QIcon& icon)
{
    // 预览为 SVG，按当前设备像素比渲染物理尺寸，避免高分屏拉伸发虚
    const qreal dpr = devicePixelRatioF();
    const QSize physicalSize(qRound(240 * dpr), qRound(96 * dpr));
    QPixmap pm = icon.pixmap(physicalSize);
    pm.setDevicePixelRatio(dpr);
    ui->labelImage->setPixmap(pm);
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
    // 记录旧值
    mOldRibbonStyle = static_cast< SARibbonBar::RibbonStyles >(mAppConfig->ui().ribbonStyle);
    // 更新
    DAUISettings s        = mAppConfig->ui();
    s.ribbonStyle         = static_cast< int >(mNewRibbonStyle);
    s.ribbonTheme         = ui->comboBoxRibbonTheme->currentData().toInt();
    s.language            = ui->comboBoxLanguage->currentData().toString();
    s.dockTabPosition     = (ui->comboBoxDockTabPosition->currentData().toString() == QLatin1String("top"))
                                ? DADockTabPosition::Top
                                : DADockTabPosition::Bottom;
    QFont f               = ui->fontComboBoxAppFont->currentFont();
    s.fontFamily          = f.family();
    s.fontPointSize       = ui->spinBoxFontSize->value();
    s.saveUiStateOnClose  = ui->checkBoxSaveUIState->isChecked();
    // setUI 会发出 sectionChanged，由主窗口负责热应用；保存由设置对话框触发
    mAppConfig->setUI(s);
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
    mAppConfig                 = p;
    const DAUISettings& cfgUI  = p->ui();
    // ribbon style
    SARibbonBar::RibbonStyles ribbonStyle = static_cast< SARibbonBar::RibbonStyles >(cfgUI.ribbonStyle);
    mOldRibbonStyle = ribbonStyle;
    mNewRibbonStyle = ribbonStyle;
    switch (ribbonStyle) {
    case SARibbonBar::RibbonStyleCompactThreeRow:
        ui->radioButtonLiteStyle->setChecked(true);
        setRibbonStylePreview(mIconRibbonLite);
        break;
    case SARibbonBar::RibbonStyleCompactTwoRow:
        ui->radioButtonLiteStyle2Row->setChecked(true);
        setRibbonStylePreview(mIconRibbonLite2Row);
        break;
    case SARibbonBar::RibbonStyleLooseThreeRow:
        ui->radioButtonStandardStyle->setChecked(true);
        setRibbonStylePreview(mIconRibbonStandard);
        break;
    case SARibbonBar::RibbonStyleLooseTwoRow:
        ui->radioButtonStandardStyle2Row->setChecked(true);
        setRibbonStylePreview(mIconRibbonStandard2Row);
        break;
    default:
        break;
    }
    // ribbon theme
    int theme = cfgUI.ribbonTheme;
    int tidx  = ui->comboBoxRibbonTheme->findData(theme);
    if (tidx >= 0) {
        ui->comboBoxRibbonTheme->setCurrentIndex(tidx);
    }
    // language
    QString lang = cfgUI.language;
    int lidx    = ui->comboBoxLanguage->findData(lang);
    ui->comboBoxLanguage->setCurrentIndex(lidx >= 0 ? lidx : 0);
    // 操作窗口标签页方位（取值已在加载时规范化）
    const QString tabPos = (cfgUI.dockTabPosition == DADockTabPosition::Top) ? QStringLiteral("top")
                                                                            : QStringLiteral("bottom");
    int pidx             = ui->comboBoxDockTabPosition->findData(tabPos);
    ui->comboBoxDockTabPosition->setCurrentIndex(pidx >= 0 ? pidx : 0);
    // font
    QFont f;
    if (!cfgUI.fontFamily.isEmpty()) {
        f.setFamily(cfgUI.fontFamily);
    }
    ui->fontComboBoxAppFont->setCurrentFont(f);
    double sz = cfgUI.fontPointSize;
    ui->spinBoxFontSize->setValue(sz > 0 ? static_cast< int >(sz)
                                         : (QApplication::font().pointSize() > 0 ? QApplication::font().pointSize() : 9));
    // 是否记录ui
    ui->checkBoxSaveUIState->setChecked(cfgUI.saveUiStateOnClose);

    return true;
}

void DASettingPageGeneral::onButtonGroupRibbonStyleClicked(int id)
{
    SARibbonBar::RibbonStyles ribbonStyle = static_cast< SARibbonBar::RibbonStyles >(id);
    switch (ribbonStyle) {
    case SARibbonBar::RibbonStyleCompactThreeRow:
        setRibbonStylePreview(mIconRibbonLite);
        break;
    case SARibbonBar::RibbonStyleCompactTwoRow:
        setRibbonStylePreview(mIconRibbonLite2Row);
        break;
    case SARibbonBar::RibbonStyleLooseThreeRow:
        setRibbonStylePreview(mIconRibbonStandard);
        break;
    case SARibbonBar::RibbonStyleLooseTwoRow:
        setRibbonStylePreview(mIconRibbonStandard2Row);
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
