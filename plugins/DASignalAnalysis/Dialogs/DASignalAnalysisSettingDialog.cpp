#include "DASignalAnalysisSettingDialog.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include "Common/Settings/DAPropertyFormWidget.h"
#include "DAFormSchemaIO.h"
#include "DAFormSpec.h"
#include "Form/DASignalFormSpecBuilder.h"
#include "Signal/DASignalImporter.h"

namespace DA
{
DASignalAnalysisSettingDialog::DASignalAnalysisSettingDialog(const DASignalAnalysisParams& params,
                                                             double sampleRate,
                                                             QWidget* parent)
    : QDialog(parent), mSampleRate(sampleRate)
{
    setModal(true);
    setMinimumSize(520, 620);
    buildUI();
    retranslateUi();
    // 上次值作为字段默认值（「记住上次值」）；派生分辨率按真实段长算（评审 U7：不能传 fs/100）
    double derivedResolution = 0.0;
    if (sampleRate > 0.0 && params.resolutionHz > 0.0) {
        const int segLen = qMax(8, int(qRound(sampleRate / params.resolutionHz)));
        derivedResolution = sampleRate / double(segLen);
    }
    const QJsonObject specJson = DASignalFormSpecBuilder::build(params, derivedResolution);
    DAFormSpec spec;
    QString err;
    if (DAFormSchemaIO::fromJsonObject(specJson, spec, &err)) {
        mForm->setFormSpec(spec);
    } else {
        qWarning() << "DASignalAnalysisSettingDialog: invalid form spec:" << err;
    }
    mForm->setValues(params.toVariantMap());
    // 记录最小间距是否处于「自动跟随分辨率」状态（D17）：用户保存的是 0/负值即未手改
    mAutoSpacing = (params.peakMinSpacingHz > 0.0 || derivedResolution <= 0.0) ? -1.0 : 5.0 * derivedResolution;
    refreshDerivedInfo();
}

DASignalAnalysisSettingDialog::~DASignalAnalysisSettingDialog()
{
}

/**
 * @brief 弹出参数对话框（静态便捷入口）
 */
bool DASignalAnalysisSettingDialog::edit(DASignalAnalysisParams* params, double sampleRate, QWidget* parent)
{
    if (!params) {
        return false;
    }
    DASignalAnalysisSettingDialog dlg(*params, sampleRate, parent);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    *params = dlg.params();
    return true;
}

/**
 * @brief 结果参数
 */
DASignalAnalysisParams DASignalAnalysisSettingDialog::params() const
{
    if (!mForm) {
        return DASignalAnalysisParams();
    }
    return DASignalAnalysisParams::fromVariantMap(mForm->values());
}

/**
 * @brief 构建界面：表单 + 派生信息 + 恢复默认 + 确定/取消
 */
void DASignalAnalysisSettingDialog::buildUI()
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    mForm            = new DAPropertyFormWidget(this);
    lay->addWidget(mForm, 1);

    mDerivedLabel = new QLabel(this);
    mDerivedLabel->setWordWrap(true);
    mDerivedLabel->setStyleSheet(QStringLiteral("color:#727272;"));  // 中性次要文字
    lay->addWidget(mDerivedLabel);

    // FFmpeg 路径（方案 D7：视频/压缩音频依赖它；未装时相关入口置灰并在此可指定）
    QHBoxLayout* ffmpegRow = new QHBoxLayout;
    mFFmpegLabel           = new QLabel(this);
    mFFmpegEdit            = new QLineEdit(this);
    mFFmpegEdit->setText(DASignalImporter::ffmpegPath());
    mFFmpegBrowse = new QPushButton(this);
    ffmpegRow->addWidget(mFFmpegLabel);
    ffmpegRow->addWidget(mFFmpegEdit, 1);
    ffmpegRow->addWidget(mFFmpegBrowse);
    lay->addLayout(ffmpegRow);
    mFFmpegStatus = new QLabel(this);
    mFFmpegStatus->setWordWrap(true);
    lay->addWidget(mFFmpegStatus);
    connect(mFFmpegBrowse, &QPushButton::clicked, this, &DASignalAnalysisSettingDialog::onBrowseFFmpeg);
    connect(mFFmpegEdit, &QLineEdit::textChanged, this, &DASignalAnalysisSettingDialog::onFFmpegPathEdited);

    QHBoxLayout* bottom = new QHBoxLayout;
    mRestoreButton      = new QPushButton(this);
    bottom->addWidget(mRestoreButton);
    bottom->addStretch();
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bottom->addWidget(mButtonBox);
    lay->addLayout(bottom);

    connect(mRestoreButton, &QPushButton::clicked, this, &DASignalAnalysisSettingDialog::onRestoreDefaults);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(mForm, &DAPropertyFormWidget::fieldValueChanged, this, &DASignalAnalysisSettingDialog::onFieldValueChanged);
}

/**
 * @brief 字段值变化：只刷新派生信息（不触发重算，重算在对话框确定后做一次）
 */
void DASignalAnalysisSettingDialog::onFieldValueChanged(const QString& fieldName, const QVariant& value)
{
    Q_UNUSED(value)
    if (fieldName == QLatin1String("resolutionHz") || fieldName == QLatin1String("stftWindowMs")) {
        refreshDerivedInfo();
    }
    if (fieldName == QLatin1String("resolutionHz")) {
        // 用户没手改过最小间距时，跟随实际分辨率联动（D17）
        syncPeakSpacingWithResolution();
    } else if (fieldName == QLatin1String("peakMinSpacingHz")) {
        mAutoSpacing = -1.0;  // 用户手改后不再跟随分辨率
    }
}

/**
 * @brief 当前生效的实际频率分辨率（由分辨率参数反推段长后回算）
 */
double DASignalAnalysisSettingDialog::currentDerivedResolution() const
{
    if (mSampleRate <= 0.0 || !mForm) {
        return 0.0;
    }
    const DASignalAnalysisParams p = params();
    const int segLen               = qMax(8, int(qRound(mSampleRate / qMax(p.resolutionHz, 1e-9))));
    return mSampleRate / double(segLen);
}

/**
 * @brief 最小间距跟随分辨率联动（仅在用户未手改时）
 */
void DASignalAnalysisSettingDialog::syncPeakSpacingWithResolution()
{
    if (!mForm || mAutoSpacing < 0.0) {
        return;
    }
    const double derived = currentDerivedResolution();
    if (derived <= 0.0) {
        return;
    }
    mAutoSpacing = 5.0 * derived;
    mForm->setValue(QStringLiteral("peakMinSpacingHz"), mAutoSpacing);  // 程序化写入不触发 fieldValueChanged
}

/**
 * @brief 恢复默认参数
 */
void DASignalAnalysisSettingDialog::onRestoreDefaults()
{
    const DASignalAnalysisParams def = DASignalAnalysisParams::defaults();
    mForm->setValues(def.toVariantMap());
    refreshDerivedInfo();
}

/**
 * @brief 刷新派生信息反显（段长 / 毫秒数 / 实际分辨率）
 *
 * 方案 §6.3：界面必须同步反显「段长 = … 点（… 毫秒）、实际分辨率 = … 赫兹」。
 */
void DASignalAnalysisSettingDialog::refreshDerivedInfo()
{
    if (!mDerivedLabel) {
        return;
    }
    const DASignalAnalysisParams p = params();
    if (mSampleRate <= 0.0) {
        mDerivedLabel->setText(tr("Segment length: unknown (sample rate is invalid)"));
        // cn:段长：未知（采样率无效）
        return;
    }
    const double res     = qMax(p.resolutionHz, 1e-9);
    const int segLen     = qMax(8, int(qRound(mSampleRate / res)));
    const double realRes = mSampleRate / double(segLen);
    const double segMs   = double(segLen) / mSampleRate * 1000.0;
    // STFT 窗口/跳步反显（与 stft.py 的推导一致）
    const int stftSeg = qMax(2, int(qRound(mSampleRate * qMax(p.stftWindowMs, 1.0) / 1000.0)));
    const double stftMs = double(stftSeg) / mSampleRate * 1000.0;
    const int stftHop  = qMax(1, int(qRound(stftSeg * (1.0 - qBound(0.0, p.stftOverlap, 99.0) / 100.0))));
    mDerivedLabel->setText(tr("Spectrum: segment = %1 samples (%2 ms), actual resolution = %3 Hz\nSTFT: window = %4 samples (%5 ms), hop = %6 samples")
                               .arg(segLen)
                               .arg(segMs, 0, 'f', 2)
                               .arg(realRes, 0, 'f', 4)
                               .arg(stftSeg)
                               .arg(stftMs, 0, 'f', 2)
                               .arg(stftHop));
    // cn:频谱：段长 = %1 点（%2 毫秒）、实际分辨率 = %3 赫兹；时频：窗长 = %4 点（%5 毫秒）、跳步 = %6 点
}

/**
 * @brief 选择 FFmpeg 可执行文件
 */
void DASignalAnalysisSettingDialog::onBrowseFFmpeg()
{
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Select FFmpeg Executable"), QString(), tr("Executable (*.exe);;All files (*)"));
    // cn:选择 FFmpeg 可执行文件
    if (path.isEmpty()) {
        return;
    }
    mFFmpegEdit->setText(QDir::toNativeSeparators(path));
}

/**
 * @brief 路径文本变化：内联校验（不用模态消息框做字段校验）
 */
void DASignalAnalysisSettingDialog::onFFmpegPathEdited()
{
    const QString text = mFFmpegEdit ? mFFmpegEdit->text().trimmed() : QString();
    if (text.isEmpty()) {
        mFFmpegStatus->setStyleSheet(QStringLiteral("color:#727272;"));
        mFFmpegStatus->setText(tr("Empty: fall back to the ffmpeg found in PATH."));
        // cn:留空则回退到系统 PATH 中的 ffmpeg。
        return;
    }
    const QFileInfo fi(text);
    if (fi.exists() && fi.isFile()) {
        mFFmpegStatus->setStyleSheet(QStringLiteral("color:#669E8B;"));  // 成功绿（官方色板）
        mFFmpegStatus->setText(tr("FFmpeg path is valid."));            // cn:FFmpeg 路径有效。
    } else {
        mFFmpegStatus->setStyleSheet(QStringLiteral("color:#CE6043;"));  // 危险橙红（官方色板）
        mFFmpegStatus->setText(tr("File does not exist."));              // cn:文件不存在。
    }
}

/**
 * @brief 应用并持久化 FFmpeg 路径；路径无效返回 false
 */
bool DASignalAnalysisSettingDialog::applyFFmpegPath()
{
    const QString text = mFFmpegEdit ? mFFmpegEdit->text().trimmed() : QString();
    if (text.isEmpty()) {
        DASignalImporter::setFFmpegPath(QString());
        return true;
    }
    const QFileInfo fi(text);
    if (!fi.exists() || !fi.isFile()) {
        onFFmpegPathEdited();  // 显示内联错误
        return false;
    }
    DASignalImporter::setFFmpegPath(fi.absoluteFilePath());
    return true;
}

/**
 * @brief 确定：先应用 FFmpeg 路径（无效时留在对话框内）
 */
void DASignalAnalysisSettingDialog::accept()
{
    if (!applyFFmpegPath()) {
        return;
    }
    QDialog::accept();
}

void DASignalAnalysisSettingDialog::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(e);
}

void DASignalAnalysisSettingDialog::retranslateUi()
{
    setWindowTitle(tr("Signal Analysis Settings"));  // cn:信号分析设置
    if (mFFmpegLabel) {
        mFFmpegLabel->setText(tr("FFmpeg path"));  // cn:FFmpeg 路径
    }
    if (mFFmpegBrowse) {
        mFFmpegBrowse->setText(tr("Browse..."));  // cn:浏览...
    }
    if (mFFmpegStatus) {
        onFFmpegPathEdited();
    }
    if (mRestoreButton) {
        mRestoreButton->setText(tr("Restore Defaults"));  // cn:恢复默认
    }
    if (mButtonBox) {
        mButtonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));          // cn:确定
        mButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    }
}
}  // namespace DA
