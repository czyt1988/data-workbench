#include "DAPeakMarkerDisplayDialog.h"
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "Page/DAPeakMarkerLabel.h"

namespace DA
{
namespace
{
/// 预览用的示例峰值（序号 1 / 频率 32.5 赫兹 / 幅值 0.85 或 -12.3 分贝）
constexpr int cPreviewIndex          = 1;
constexpr double cPreviewFrequency   = 32.5;
constexpr double cPreviewAmplitude   = 0.85;
constexpr double cPreviewAmplitudeDb = -12.3;
}  // namespace

DAPeakMarkerDisplayDialog::DAPeakMarkerDisplayDialog(const QString& textTemplate, bool dbMode, QWidget* parent)
    : QDialog(parent), mDbMode(dbMode)
{
    setModal(true);
    setMinimumSize(440, 220);
    buildUI();
    retranslateUi();
    if (mEdit) {
        mEdit->setText(textTemplate);
        mEdit->setFocus();
    }
    updatePreview();
}

DAPeakMarkerDisplayDialog::~DAPeakMarkerDisplayDialog()
{
}

/**
 * @brief 弹出峰值标记显示设置对话框（静态便捷入口）
 */
bool DAPeakMarkerDisplayDialog::edit(QString* textTemplate, bool dbMode, QWidget* parent)
{
    if (!textTemplate) {
        return false;
    }
    DAPeakMarkerDisplayDialog dlg(*textTemplate, dbMode, parent);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    *textTemplate = dlg.peakMarkerText();
    return true;
}

/**
 * @brief 结果模板
 */
QString DAPeakMarkerDisplayDialog::peakMarkerText() const
{
    return mEdit ? mEdit->text() : QString();
}

/**
 * @brief 构建界面：说明 + 占位符按钮行 + 编辑框 + 渲染预览（自上而下一列）
 */
void DAPeakMarkerDisplayDialog::buildUI()
{
    QVBoxLayout* lay = new QVBoxLayout(this);

    mHintLabel = new QLabel(this);
    mHintLabel->setWordWrap(true);
    mHintLabel->setStyleSheet(QStringLiteral("color:#727272;"));  // 中性次要文字
    lay->addWidget(mHintLabel);

    // 占位符按钮：点击即插入到编辑框光标处
    QHBoxLayout* buttonRow = new QHBoxLayout;
    mInsertIndexButton     = new QPushButton(this);
    mInsertFrequencyButton = new QPushButton(this);
    mInsertAmplitudeButton = new QPushButton(this);
    buttonRow->addWidget(mInsertIndexButton);
    buttonRow->addWidget(mInsertFrequencyButton);
    buttonRow->addWidget(mInsertAmplitudeButton);
    buttonRow->addStretch();
    lay->addLayout(buttonRow);

    mEdit = new QLineEdit(this);
    mEdit->setObjectName(QStringLiteral("da-peak-marker-edit"));  // 测试与样式查找用
    lay->addWidget(mEdit);

    // 渲染结果：与图上标记文本同一渲染函数，用户所见即所得
    mPreviewLabel = new QLabel(this);
    mPreviewLabel->setObjectName(QStringLiteral("da-peak-marker-preview"));
    mPreviewLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mPreviewLabel->setWordWrap(true);
    lay->addWidget(mPreviewLabel);
    lay->addStretch();

    QHBoxLayout* bottom = new QHBoxLayout;
    mRestoreButton      = new QPushButton(this);
    bottom->addWidget(mRestoreButton);
    bottom->addStretch();
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bottom->addWidget(mButtonBox);
    lay->addLayout(bottom);

    connect(mInsertIndexButton, &QPushButton::clicked, this, &DAPeakMarkerDisplayDialog::onInsertIndex);
    connect(mInsertFrequencyButton, &QPushButton::clicked, this, &DAPeakMarkerDisplayDialog::onInsertFrequency);
    connect(mInsertAmplitudeButton, &QPushButton::clicked, this, &DAPeakMarkerDisplayDialog::onInsertAmplitude);
    connect(mRestoreButton, &QPushButton::clicked, this, &DAPeakMarkerDisplayDialog::onRestoreDefault);
    connect(mEdit, &QLineEdit::textChanged, this, &DAPeakMarkerDisplayDialog::updatePreview);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/**
 * @brief 在光标处插入占位符（占位符本身是普通文本，可自由与单位/前后缀组合）
 */
void DAPeakMarkerDisplayDialog::insertPlaceholder(const QString& placeholder)
{
    if (!mEdit) {
        return;
    }
    mEdit->insert(placeholder);
    mEdit->setFocus();
}

void DAPeakMarkerDisplayDialog::onInsertIndex()
{
    insertPlaceholder(QLatin1String(DAPeakMarkerLabel::cIndexPlaceholder));
}

void DAPeakMarkerDisplayDialog::onInsertFrequency()
{
    insertPlaceholder(QLatin1String(DAPeakMarkerLabel::cFrequencyPlaceholder));
}

void DAPeakMarkerDisplayDialog::onInsertAmplitude()
{
    insertPlaceholder(QLatin1String(DAPeakMarkerLabel::cAmplitudePlaceholder));
}

void DAPeakMarkerDisplayDialog::onRestoreDefault()
{
    if (mEdit) {
        mEdit->setText(daDefaultPeakMarkerText());
        mEdit->setFocus();
    }
}

/**
 * @brief 重新渲染预览（空模板显示「不显示文本」）
 */
void DAPeakMarkerDisplayDialog::updatePreview()
{
    if (!mPreviewLabel) {
        return;
    }
    const QString textTemplate = peakMarkerText();
    const QString rendered     = daFormatPeakMarkerText(textTemplate,
                                                   cPreviewIndex,
                                                   cPreviewFrequency,
                                                   mDbMode ? cPreviewAmplitudeDb : cPreviewAmplitude,
                                                   mDbMode);
    if (rendered.isEmpty()) {
        mPreviewLabel->setText(tr("Preview: (no text)"));  // cn:预览：（不显示文本）
        return;
    }
    mPreviewLabel->setText(tr("Preview: %1").arg(rendered));  // cn:预览：%1
}

void DAPeakMarkerDisplayDialog::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
        updatePreview();
    }
    QDialog::changeEvent(e);
}

void DAPeakMarkerDisplayDialog::retranslateUi()
{
    setWindowTitle(tr("Marker Display"));  // cn:标记显示
    if (mHintLabel) {
        mHintLabel->setText(
            tr("Insert placeholders into the text shown above each peak marker. Placeholders: %1 = rank, %2 = frequency, %3 = amplitude.")
                .arg(QLatin1String(DAPeakMarkerLabel::cIndexPlaceholder),
                     QLatin1String(DAPeakMarkerLabel::cFrequencyPlaceholder),
                     QLatin1String(DAPeakMarkerLabel::cAmplitudePlaceholder)));
        // cn:把占位符插入峰值标记上方显示的文字。占位符：%1 = 排序号，%2 = 频率，%3 = 幅值。
    }
    if (mInsertIndexButton) {
        mInsertIndexButton->setText(tr("Index"));  // cn:序号
        mInsertIndexButton->setToolTip(
            tr("Insert the rank placeholder %1").arg(QLatin1String(DAPeakMarkerLabel::cIndexPlaceholder)));
        // cn:插入排序号占位符 %1
    }
    if (mInsertFrequencyButton) {
        mInsertFrequencyButton->setText(tr("Frequency"));  // cn:频率
        mInsertFrequencyButton->setToolTip(
            tr("Insert the frequency placeholder %1").arg(QLatin1String(DAPeakMarkerLabel::cFrequencyPlaceholder)));
        // cn:插入频率占位符 %1
    }
    if (mInsertAmplitudeButton) {
        mInsertAmplitudeButton->setText(tr("Amplitude"));  // cn:幅值
        mInsertAmplitudeButton->setToolTip(
            tr("Insert the amplitude placeholder %1").arg(QLatin1String(DAPeakMarkerLabel::cAmplitudePlaceholder)));
        // cn:插入幅值占位符 %1
    }
    if (mEdit) {
        mEdit->setPlaceholderText(daDefaultPeakMarkerText());
        mEdit->setToolTip(tr("Template for the text above peak markers; leave it empty to hide the text"));
        // cn:峰值标记上方文本的模板；留空则不显示文本
    }
    if (mRestoreButton) {
        mRestoreButton->setText(tr("Restore Default"));  // cn:恢复默认
    }
    if (mButtonBox) {
        mButtonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));          // cn:确定
        mButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    }
}
}  // namespace DA