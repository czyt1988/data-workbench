#include "DASignalAnalysisSettingDialog.h"
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include "Common/Settings/DAPropertyFormWidget.h"
#include "DAFormSchemaIO.h"
#include "DAFormSpec.h"
#include "Form/DASignalFormSpecBuilder.h"

namespace DA
{
DASignalAnalysisSettingDialog::DASignalAnalysisSettingDialog(const DASignalAnalysisParams& params,
                                                             double sampleRate,
                                                             QWidget* parent)
    : QDialog(parent), mSampleRate(sampleRate), mBaseParams(params)
{
    setModal(true);
    setMinimumSize(520, 480);
    buildUI();
    retranslateUi();
    // 上次值作为字段默认值（「记住上次值」）；派生分辨率按真实段长算（评审 U7：不能传 fs/100）
    const double derivedResolution = derivedResolutionFor(params);
    const QJsonObject specJson     = DASignalFormSpecBuilder::build(params, derivedResolution);
    DAFormSpec spec;
    QString err;
    if (DAFormSchemaIO::fromJsonObject(specJson, spec, &err)) {
        mForm->setFormSpec(spec);
    } else {
        qWarning() << "DASignalAnalysisSettingDialog: invalid form spec:" << err;
    }
    // 回显副本：自动哨兵（-1/0）换算成 5×实际分辨率再写入，避免 QDoubleSpinBox 的 min=0 把它夹成 0
    // （评审 N-U6；直接 setValues(params.toVariantMap()) 会让「高级」里显示 0）
    mForm->setValues(makeEchoParams(params, derivedResolution).toVariantMap());
    // 记录最小间距是否处于「自动跟随分辨率」状态（D17）：参数为 <=0 即用户未手改
    mPeakSpacingAuto = (params.peakMinSpacingHz <= 0.0);
    mFormReady       = true;
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
 *
 * 最小间距仍处于自动状态时写回 -1 哨兵（Python 侧对 <=0 按 5 倍实际分辨率兜底），
 * 这样下次打开对话框仍然「未手改」，改分辨率继续跟随（评审 N-C5）。
 *
 * 常用参数（预处理 / 窗函数 / 幅值口径 / 分辨率 / STFT 窗长与跳步 / 峰值模式、标记数量、
 * 相对门限）已搬上丝带，表单里没有这些字段：fromVariantMap() 会把缺失键填成默认值，
 * 这里从打开对话框时的基线参数回填，否则「确定」会把丝带上的选择静默重置。
 * dB 参考不再有独立入口，返回前按幅值口径派生（D7）。
 */
DASignalAnalysisParams DASignalAnalysisSettingDialog::params() const
{
    if (!mForm) {
        return DASignalAnalysisParams();
    }
    DASignalAnalysisParams p = DASignalAnalysisParams::fromVariantMap(mForm->values());
    p.removeDc               = mBaseParams.removeDc;
    p.detrend                = mBaseParams.detrend;
    p.window                 = mBaseParams.window;
    p.amplitudeMode          = mBaseParams.amplitudeMode;
    p.resolutionHz           = mBaseParams.resolutionHz;
    p.stftWindowMs           = mBaseParams.stftWindowMs;
    p.stftOverlap            = mBaseParams.stftOverlap;
    p.peakMode               = mBaseParams.peakMode;
    p.peakTopN               = mBaseParams.peakTopN;
    p.peakRelDb              = mBaseParams.peakRelDb;
    if (mPeakSpacingAuto) {
        p.peakMinSpacingHz = -1.0;
    }
    p.normalizeDbReference();
    return p;
}

/**
 * @brief 构造表单回显用的参数副本
 *
 * peakMinSpacingHz 的 -1/0 是「自动」哨兵，而表单字段 min=0 会把它夹成 0，
 * 导致高级模式看不到「5×实际分辨率」派生默认值（评审 N-U6）。
 * 回显前统一换算成 signalEffectivePeakMinSpacingHz() 的有效值。
 */
DASignalAnalysisParams DASignalAnalysisSettingDialog::makeEchoParams(const DASignalAnalysisParams& params,
                                                                     double derivedResolution) const
{
    DASignalAnalysisParams echo = params;
    if (echo.peakMinSpacingHz <= 0.0) {
        echo.peakMinSpacingHz = signalEffectivePeakMinSpacingHz(params, derivedResolution);
    }
    return echo;
}

/**
 * @brief 构建界面：表单 + 派生信息 + 恢复默认 + 确定/取消
 *
 * 全局设置（FFmpeg 路径等）不在这里：它们在 主页 → 设置 → 信号分析 页里，
 * 这样没导入数据时也能配置。
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
 * @brief 字段值变化：只刷新派生信息与联动（不触发重算，重算在对话框确定后做一次）
 *
 * 分辨率 / STFT 窗长的双向联动已随字段迁到丝带（在 DASignalAnalysisUI 里做），
 * 本对话框只剩峰值高级项，因此这里只需要处理「最小间距是否还处于自动跟随」。
 */
void DASignalAnalysisSettingDialog::onFieldValueChanged(const QString& fieldName, const QVariant& value)
{
    Q_UNUSED(value)
    if (fieldName == QLatin1String("peakMinSpacingHz")) {
        // 用户手改后不再跟随分辨率（写回参数时保留显式值，不再写 -1 哨兵）
        mPeakSpacingAuto = false;
    }
    refreshDerivedInfo();
}

/**
 * @brief 按给定参数反推实际频率分辨率（按真实段长回算，评审 U7）
 */
double DASignalAnalysisSettingDialog::derivedResolutionFor(const DASignalAnalysisParams& params) const
{
    if (mSampleRate <= 0.0 || !(params.resolutionHz > 0.0)) {
        return 0.0;
    }
    const int segLen = qMax(8, int(qRound(mSampleRate / params.resolutionHz)));
    return mSampleRate / double(segLen);
}

/**
 * @brief 恢复默认参数
 *
 * 默认 peakMinSpacingHz=-1 表示自动，回显同样经 makeEchoParams() 换算成 5×实际分辨率，
 * 否则会被表单 min=0 夹成 0（评审 N-U6）。
 */
void DASignalAnalysisSettingDialog::onRestoreDefaults()
{
    if (!mForm) {
        return;
    }
    const DASignalAnalysisParams def = DASignalAnalysisParams::defaults();
    mPeakSpacingAuto                 = true;
    mForm->setValues(makeEchoParams(def, derivedResolutionFor(def)).toVariantMap());
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

void DASignalAnalysisSettingDialog::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(e);
}

void DASignalAnalysisSettingDialog::retranslateUi()
{
    setWindowTitle(tr("Advanced Analysis Settings"));  // cn:高级分析设置
    if (mRestoreButton) {
        mRestoreButton->setText(tr("Restore Defaults"));  // cn:恢复默认
    }
    if (mButtonBox) {
        mButtonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));          // cn:确定
        mButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    }
    // 表单字段标签由 DAFormSpec 生成：语言切换后必须重建规格才能重译（评审 I7）
    rebuildFormForLanguage();
    if (mFormReady) {
        refreshDerivedInfo();
    }
}

/**
 * @brief 语言切换时重建表单规格以重译字段标签
 *
 * DAFormSpec 的 label 在 build() 时经 QCoreApplication::translate 求值，构造后才改语言
 * 不会自动刷新；这里先记下当前表单值，重建规格后原样写回，避免丢用户输入。
 */
void DASignalAnalysisSettingDialog::rebuildFormForLanguage()
{
    if (!mForm || !mFormReady) {
        return;
    }
    const QVariantMap current      = mForm->values();
    const DASignalAnalysisParams p = DASignalAnalysisParams::fromVariantMap(current);
    const QJsonObject specJson     = DASignalFormSpecBuilder::build(p, derivedResolutionFor(p));
    DAFormSpec spec;
    QString err;
    if (!DAFormSchemaIO::fromJsonObject(specJson, spec, &err)) {
        qWarning() << "DASignalAnalysisSettingDialog: invalid form spec on retranslate:" << err;
        return;
    }
    mForm->setFormSpec(spec);
    mForm->setValues(current);
}
}  // namespace DA
