#include "DADialogFillSeries.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include <QRadioButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QDateTime>

namespace DA
{

/**
 * @brief 构造
 * @param previewCount 预览生成的值个数（<=0 时用 5）
 * @param parent 父窗口
 */
DADialogFillSeries::DADialogFillSeries(int previewCount, QWidget* parent) : QDialog(parent), mPreviewCount(previewCount > 0 ? previewCount : 5)
{
    setupUI();
    retranslateUi();
    onKindChanged();
    updatePreview();
}

DADialogFillSeries::~DADialogFillSeries()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）：种类单选 + 参数表单 + 预览
 */
void DADialogFillSeries::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 种类
    QHBoxLayout* kindLayout = new QHBoxLayout();
    mRadioLinear            = new QRadioButton(this);
    mRadioLinear->setChecked(true);
    mRadioDate              = new QRadioButton(this);
    QButtonGroup* kindGroup = new QButtonGroup(this);
    kindGroup->addButton(mRadioLinear);
    kindGroup->addButton(mRadioDate);
    kindLayout->addWidget(mRadioLinear);
    kindLayout->addWidget(mRadioDate);
    kindLayout->addStretch();
    mainLayout->addLayout(kindLayout);
    // 参数表单
    QFormLayout* form = new QFormLayout();
    mEditStart        = new QLineEdit(this);
    mEditStep         = new QLineEdit(this);
    mEditStop         = new QLineEdit(this);
    mComboDateUnit    = new QComboBox(this);
    mComboDateUnit->addItem(QString(), QStringLiteral("D"));
    mComboDateUnit->addItem(QString(), QStringLiteral("W"));
    mComboDateUnit->addItem(QString(), QStringLiteral("M"));
    mComboDateUnit->addItem(QString(), QStringLiteral("Y"));
    mLabelStopTitle   = new QLabel(this);
    form->addRow(QString(), mEditStart);
    form->addRow(QString(), mEditStep);
    form->addRow(mLabelStopTitle, mEditStop);
    QLabel* labelUnit = new QLabel(this);
    labelUnit->setObjectName(QStringLiteral("fillSeriesDateUnitLabel"));
    form->addRow(labelUnit, mComboDateUnit);
    mainLayout->addLayout(form);
    // 预览
    mLabelPreview = new QLabel(this);
    mLabelPreview->setWordWrap(true);
    mLabelPreview->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(mLabelPreview);
    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtonOk                   = buttonBox->button(QDialogButtonBox::Ok);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // 参数变化实时预览
    connect(mRadioLinear, &QRadioButton::toggled, this, &DADialogFillSeries::onKindChanged);
    connect(mRadioDate, &QRadioButton::toggled, this, &DADialogFillSeries::onKindChanged);
    connect(mEditStart, &QLineEdit::textChanged, this, &DADialogFillSeries::updatePreview);
    connect(mEditStep, &QLineEdit::textChanged, this, &DADialogFillSeries::updatePreview);
    connect(mEditStop, &QLineEdit::textChanged, this, &DADialogFillSeries::updatePreview);
    connect(mComboDateUnit, &QComboBox::currentIndexChanged, this, &DADialogFillSeries::updatePreview);
    // objectName 供离屏测试定位
    mRadioLinear->setObjectName(QStringLiteral("fillSeriesKindLinear"));
    mRadioDate->setObjectName(QStringLiteral("fillSeriesKindDate"));
    mEditStart->setObjectName(QStringLiteral("fillSeriesStart"));
    mEditStep->setObjectName(QStringLiteral("fillSeriesStep"));
    mEditStop->setObjectName(QStringLiteral("fillSeriesStop"));
    mComboDateUnit->setObjectName(QStringLiteral("fillSeriesDateUnit"));
    mLabelPreview->setObjectName(QStringLiteral("fillSeriesPreview"));
    setMinimumWidth(360);
}

/**
 * @brief 文案（多语言切换经 changeEvent→retranslateUi 由基类 QDialog 触发重译需求，
 * 本对话框为模态短生命周期，构造时设置一次即可）
 */
void DADialogFillSeries::retranslateUi()
{
    setWindowTitle(tr("Fill Series"));  // cn:序列填充
    mRadioLinear->setText(tr("Linear series (start / step / stop)"));  // cn:数值等差（起始值/步长/终止值）
    mRadioDate->setText(tr("Date series (start / step / unit)"));      // cn:日期步进（起始日期/步长/单位）
    mEditStart->setPlaceholderText(tr("Start value (empty = default)"));  // cn:起始值（留空用默认）
    mEditStep->setPlaceholderText(tr("Step (empty = 1)"));                // cn:步长（留空为 1）
    mLabelStopTitle->setText(tr("Stop value (optional)"));                // cn:终止值（可选）
    mEditStop->setPlaceholderText(tr("Stop value (empty = no limit)"));   // cn:终止值（留空不限）
    mComboDateUnit->setItemText(0, tr("Day"));    // cn:天
    mComboDateUnit->setItemText(1, tr("Week"));   // cn:周
    mComboDateUnit->setItemText(2, tr("Month"));  // cn:月
    mComboDateUnit->setItemText(3, tr("Year"));   // cn:年
}

/**
 * @brief 种类切换：日期模式隐藏终止值（date 序列由个数决定）、显示单位下拉
 */
void DADialogFillSeries::onKindChanged()
{
    const bool dateMode = mRadioDate->isChecked();
    mEditStop->setEnabled(!dateMode);
    mLabelStopTitle->setEnabled(!dateMode);
    mComboDateUnit->setEnabled(dateMode);
    if (dateMode) {
        mEditStart->setPlaceholderText(tr("Start date, e.g. 2020-01-01 (empty = today)"));  // cn:起始日期，如 2020-01-01（留空为今天）
    } else {
        mEditStart->setPlaceholderText(tr("Start value (empty = default)"));  // cn:起始值（留空用默认）
    }
    updatePreview();
}

/**
 * @brief 实时预览：调 da_fill_series 生成前几个值（预览与写入同一算子，红线5）
 *
 * 生成失败（日期无法解析/步长非法）显示错误并置灰 OK；成功显示值列表。
 */
void DADialogFillSeries::updatePreview()
{
    bool ok              = false;
    const QVariantList v = DAPyScripts::getDataFrame().fillSeries(mPreviewCount, kind(), startText(), stepText(), stopText(), dateUnit(), &ok);
    mSeriesValid         = ok;
    if (!ok) {
        mLabelPreview->setText(tr("Invalid parameters: unable to generate the series"));  // cn:参数无效：无法生成序列
        mButtonOk->setEnabled(false);
        return;
    }
    if (v.isEmpty()) {
        mLabelPreview->setText(tr("Preview: (empty)"));  // cn:预览：（空）
        mButtonOk->setEnabled(true);
        return;
    }
    QStringList parts;
    parts.reserve(v.size());
    for (const QVariant& item : v) {
        if (item.userType() == QMetaType::QDateTime) {
            parts.append(item.toDateTime().toString(QStringLiteral("yyyy-MM-dd")));
        } else {
            parts.append(item.toString());
        }
    }
    mLabelPreview->setText(tr("Preview: %1").arg(parts.join(QStringLiteral(", "))));  // cn:预览：%1
    mButtonOk->setEnabled(true);
}

QString DADialogFillSeries::kind() const
{
    return mRadioDate->isChecked() ? QStringLiteral("date") : QStringLiteral("linear");
}

QString DADialogFillSeries::startText() const
{
    return mEditStart->text().trimmed();
}

QString DADialogFillSeries::stepText() const
{
    return mEditStep->text().trimmed();
}

QString DADialogFillSeries::stopText() const
{
    return mRadioDate->isChecked() ? QString() : mEditStop->text().trimmed();
}

QString DADialogFillSeries::dateUnit() const
{
    return mComboDateUnit->currentData().toString();
}

bool DADialogFillSeries::isSeriesValid() const
{
    return mSeriesValid;
}

}  // end of namespace DA
