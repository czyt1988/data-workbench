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
    : QDialog(parent), mSampleRate(sampleRate)
{
    setModal(true);
    setMinimumSize(520, 620);
    buildUI();
    retranslateUi();
    // 上次值作为字段默认值（「记住上次值」）
    const QJsonObject specJson = DASignalFormSpecBuilder::build(params, sampleRate > 0 ? sampleRate / 100.0 : 0.0);
    DAFormSpec spec;
    QString err;
    if (DAFormSchemaIO::fromJsonObject(specJson, spec, &err)) {
        mForm->setFormSpec(spec);
    } else {
        qWarning() << "DASignalAnalysisSettingDialog: invalid form spec:" << err;
    }
    mForm->setValues(params.toVariantMap());
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
    mDerivedLabel->setText(tr("Segment length = %1 samples (%2 ms), actual resolution = %3 Hz")
                               .arg(segLen)
                               .arg(segMs, 0, 'f', 2)
                               .arg(realRes, 0, 'f', 4));
    // cn:段长 = %1 点（%2 毫秒）、实际分辨率 = %3 赫兹
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
    if (mRestoreButton) {
        mRestoreButton->setText(tr("Restore Defaults"));  // cn:恢复默认
    }
    if (mButtonBox) {
        mButtonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));          // cn:确定
        mButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    }
}
}  // namespace DA
