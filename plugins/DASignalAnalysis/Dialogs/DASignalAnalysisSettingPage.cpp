#include "DASignalAnalysisSettingPage.h"
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QDebug>
#include "Common/Settings/DAPropertyFormWidget.h"
#include "DAFormSchemaIO.h"
#include "DAFormSpec.h"
#include "DALogCategory.h"
#include "DASignalAnalysisUI.h"
#include "Signal/DASignalImporter.h"

namespace DA
{
namespace
{
// 表单字段名 = 配置文件键名（与 DASignalImporter 的读写口径一致）
const char* const cFieldFFmpegPath = "ffmpegPath";
// 文字配色：中性次要文字 / 成功绿 / 危险橙红（DESIGN.md §2.3 官方色板）
const char* const cMutedStyle   = "color:#727272;";
const char* const cSuccessStyle = "color:#669E8B;";
const char* const cErrorStyle   = "color:#CE6043;";

/**
 * @brief 读取配置里用户显式指定的 FFmpeg 路径（空表示走 PATH 探测）
 *
 * 不回填自动探测到的路径：留空是「自动」语义，若把探测结果写进表单，
 * 用户下次保存就会把临时探测到的路径固化成配置。
 */
QString configuredFFmpegPath()
{
    return DASignalImporter::loadPluginConfig().value(QStringLiteral("ffmpegPath")).toString();
}
}  // namespace

DASignalAnalysisSettingPage::DASignalAnalysisSettingPage(QWidget* parent) : DAAbstractSettingPage(parent)
{
    buildUI();
    // 表单规格与页面文本统一由 retranslateUi 构建（语言切换时走同一路径）
    retranslateUi();
    mFormReady = true;
    loadConfig();
    refreshStatus();
}

DASignalAnalysisSettingPage::~DASignalAnalysisSettingPage()
{
}

/**
 * @brief 应用设置：保存 FFmpeg 路径并立即刷新插件入口
 *
 * 路径无效时不落盘（内联提示已经在编辑时给出），表单回退到已保存的路径，
 * 避免对话框常驻时下次打开仍显示未生效的无效文本；其余设置照常提交。
 */
void DASignalAnalysisSettingPage::apply()
{
    if (!applyFFmpegPath()) {
        daWarning << tr("The FFmpeg executable was not found, so the path was not saved.");
        // cn:未找到 FFmpeg 可执行文件，路径未保存。
        loadConfig();  // 表单回到已保存值
    }
    refreshStatus();
    if (mSignalUI) {
        // 让「导入视频」等入口立即按新状态启用/置灰，不用重启程序
        mSignalUI->onGlobalSettingsApplied();
    }
    Q_EMIT settingApplyed();
}

QString DASignalAnalysisSettingPage::getSettingPageTitle() const
{
    return tr("Signal Analysis");  // cn:信号分析
}

QIcon DASignalAnalysisSettingPage::getSettingPageIcon() const
{
    return QIcon(QStringLiteral(":/DASignalAnalysis/icon/analysisSetting.svg"));
}

/**
 * @brief 注入插件界面层（弱引用，插件卸载后自动失效）
 */
void DASignalAnalysisSettingPage::setSignalAnalysisUI(DASignalAnalysisUI* ui)
{
    mSignalUI = ui;
}

/**
 * @brief 构建界面：属性表单 + 内联状态 + 说明文字
 */
void DASignalAnalysisSettingPage::buildUI()
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    mForm            = new DAPropertyFormWidget(this);
    lay->addWidget(mForm);

    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(true);
    lay->addWidget(mStatusLabel);

    mHintLabel = new QLabel(this);
    mHintLabel->setWordWrap(true);
    mHintLabel->setStyleSheet(QString::fromLatin1(cMutedStyle));
    lay->addWidget(mHintLabel);
    lay->addStretch();

    connect(mForm, &DAPropertyFormWidget::fieldValueChanged, this, &DASignalAnalysisSettingPage::onFieldValueChanged);
}

/**
 * @brief 字段值变化：刷新内联状态并通知设置对话框本页已改动
 */
void DASignalAnalysisSettingPage::onFieldValueChanged(const QString& fieldName, const QVariant& value)
{
    Q_UNUSED(value)
    if (fieldName != QLatin1String(cFieldFFmpegPath)) {
        return;
    }
    refreshStatus();
    // 只有被标记为改动的页面才会在「应用/确定」时调用 apply()
    Q_EMIT settingChanged();
}

/**
 * @brief 用配置回填表单
 */
void DASignalAnalysisSettingPage::loadConfig()
{
    if (nullptr == mForm) {
        return;
    }
    mForm->setValue(QLatin1String(cFieldFFmpegPath), QDir::toNativeSeparators(configuredFFmpegPath()));
}

/**
 * @brief 刷新 FFmpeg 路径的内联状态
 *
 * 留空表示「使用系统 PATH 里的 ffmpeg」，这里直接显示 PATH 探测结果
 * （不能复用 DASignalImporter::ffmpegPath()：它优先读配置，清空后未应用前会误报旧配置路径）；
 * 填了路径则按文件是否存在且可执行给出有效/无效反馈（字段校验不用模态消息框）。
 */
void DASignalAnalysisSettingPage::refreshStatus()
{
    if ((nullptr == mForm) || (nullptr == mStatusLabel)) {
        return;
    }
    const QString text = mForm->value(QLatin1String(cFieldFFmpegPath)).toString().trimmed();
    if (text.isEmpty()) {
        const QString detected = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
        if (detected.isEmpty()) {
            mStatusLabel->setStyleSheet(QString::fromLatin1(cErrorStyle));
            mStatusLabel->setText(tr("No FFmpeg was found in PATH. Specify an executable to enable video import."));
            // cn:系统 PATH 中未找到 FFmpeg；指定可执行文件后才能导入视频。
        } else {
            mStatusLabel->setStyleSheet(QString::fromLatin1(cMutedStyle));
            mStatusLabel->setText(tr("Empty: using the FFmpeg found in PATH: %1").arg(QDir::toNativeSeparators(detected)));
            // cn:留空：使用系统 PATH 中的 FFmpeg：%1
        }
        return;
    }
    const QFileInfo fi(text);
    if (fi.exists() && fi.isFile() && fi.isExecutable()) {
        mStatusLabel->setStyleSheet(QString::fromLatin1(cSuccessStyle));
        mStatusLabel->setText(tr("FFmpeg path is valid."));  // cn:FFmpeg 路径有效。
    } else {
        mStatusLabel->setStyleSheet(QString::fromLatin1(cErrorStyle));
        mStatusLabel->setText(tr("File does not exist."));  // cn:文件不存在。
    }
}

/**
 * @brief 应用 FFmpeg 路径
 *
 * 留空表示清除配置（回到 PATH 探测）；路径指向的文件不存在或不可执行时不写入配置，
 * 由编辑期的内联提示告知用户（字段校验不允许用模态消息框打断）。
 */
bool DASignalAnalysisSettingPage::applyFFmpegPath()
{
    if (nullptr == mForm) {
        return false;
    }
    const QString text = mForm->value(QLatin1String(cFieldFFmpegPath)).toString().trimmed();
    if (text.isEmpty()) {
        DASignalImporter::setFFmpegPath(QString());
        return true;
    }
    const QFileInfo fi(text);
    if (!fi.exists() || !fi.isFile() || !fi.isExecutable()) {
        return false;
    }
    DASignalImporter::setFFmpegPath(fi.absoluteFilePath());
    return true;
}

/**
 * @brief 语言切换：重建表单规格以重译字段标签/描述，保留当前输入
 */
void DASignalAnalysisSettingPage::retranslateUi()
{
    if (mHintLabel) {
        mHintLabel->setText(
            tr("Analysis parameters (frequency resolution, windows, peak thresholds and calibration) are configured "
               "per signal in Analysis Settings on the Signal Analysis tab."));
        // cn:频率分辨率、窗函数、峰值门限与标定等分析参数在「信号分析」标签页的「分析设置」里按信号单独配置。
    }
    rebuildFormForLanguage();
    refreshStatus();
}

/**
 * @brief 按当前语言重建表单规格
 *
 * DAFormSpec 的 label/description 在 build 时经 QCoreApplication::translate 求值，
 * 构造后才切换语言不会自动刷新，因此语言变更后先记下当前输入、重建规格再写回。
 */
void DASignalAnalysisSettingPage::rebuildFormForLanguage()
{
    if (nullptr == mForm) {
        return;
    }
    const QVariantMap current = mFormReady ? mForm->values() : QVariantMap();
    DAFormSpec spec;
    QString err;
    if (!DAFormSchemaIO::fromJsonObject(buildFormSpec(), spec, &err)) {
        qWarning() << "DASignalAnalysisSettingPage: invalid form spec:" << err;
        return;
    }
    mForm->setFormSpec(spec);
    if (!current.isEmpty()) {
        mForm->setValues(current);
    }
}

/**
 * @brief 全局设置表单规格
 *
 * 字段类型 file 由 DAFormEditorRegistry 渲染为「文本框 + 浏览按钮」（DAFilePathEditWidget）。
 */
QJsonObject DASignalAnalysisSettingPage::buildFormSpec()
{
    QJsonObject ffmpeg;
    ffmpeg[ "name" ]        = QString::fromLatin1(cFieldFFmpegPath);
    ffmpeg[ "type" ]        = QStringLiteral("file");
    ffmpeg[ "label" ]       = tr("FFmpeg executable");  // cn:FFmpeg 可执行文件
    ffmpeg[ "description" ] = tr(
        "Used for video import and compressed audio decoding. Leave empty to use the ffmpeg found in PATH.");
    // cn:用于视频导入与压缩音频解码；留空则使用系统 PATH 中的 ffmpeg。
    ffmpeg[ "filter" ]  = tr("Executable (*.exe);;All files (*)");  // cn:可执行文件 (*.exe);;所有文件 (*)
    ffmpeg[ "default" ] = QString();

    QJsonObject group;
    group[ "kind" ]  = QStringLiteral("group");
    group[ "name" ]  = QStringLiteral("externalTools");
    group[ "label" ] = tr("External Tools");  // cn:外部工具
    group[ "items" ] = QJsonArray { ffmpeg };

    QJsonObject spec;
    spec[ "version" ] = 2;
    spec[ "title" ]   = tr("Signal Analysis");  // cn:信号分析
    spec[ "items" ]   = QJsonArray { group };
    return spec;
}

void DASignalAnalysisSettingPage::changeEvent(QEvent* e)
{
    if (e && (e->type() == QEvent::LanguageChange)) {
        retranslateUi();
    }
    DAAbstractSettingPage::changeEvent(e);
}
}  // namespace DA
