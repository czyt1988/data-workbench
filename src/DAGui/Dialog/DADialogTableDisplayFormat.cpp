#include "DADialogTableDisplayFormat.h"
#include "numpy/DAPyDType.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QHeaderView>

namespace DA
{

/**
 * @brief 构造函数：构建 UI 并按 current 初始化
 */
DADialogTableDisplayFormat::DADialogTableDisplayFormat(const DATableDisplayFormat& current,
                                                       const DAPyDType& dtype,
                                                       const QVariant& sample,
                                                       QWidget* parent)
    : QDialog(parent), mCurrent(current), mSample(sample)
{
    setupUi(dtype);
    // 按 current 初始化选项
    DATableDisplayFormat::Category c = mCurrent.isValid() ? mCurrent.category() : DATableDisplayFormat::General;
    for (int i = 0; i < mCategoryList->count(); ++i) {
        if (static_cast< DATableDisplayFormat::Category >(mCategoryList->item(i)->data(Qt::UserRole).toInt()) == c) {
            mCategoryList->setCurrentRow(i);
            break;
        }
    }
    mSpinPrecision->setValue(mCurrent.isValid() ? mCurrent.precision() : 2);
    mEditDateTimePattern->setText(mCurrent.pattern());
    // pattern 预设匹配
    {
        QSignalBlocker b(mComboDateTimePreset);
        Q_UNUSED(b);
        int matched = -1;
        for (int i = 0; i < mComboDateTimePreset->count() - 1; ++i) {  // 末项为 custom
            if (mComboDateTimePreset->itemText(i) == mCurrent.pattern()) {
                matched = i;
                break;
            }
        }
        mComboDateTimePreset->setCurrentIndex(matched >= 0 ? matched : mComboDateTimePreset->count() - 1);
    }
    if (mCurrent.isValid() && mCurrent.epochUnit() == DATableDisplayFormat::EpochSeconds) {
        mRadioEpochSeconds->setChecked(true);
    } else {
        mRadioEpochMillis->setChecked(true);
    }
    switchToCategory(c);
    updatePreview();
}

DADialogTableDisplayFormat::~DADialogTableDisplayFormat()
{
}

/**
 * @brief 构建 UI（代码构建，无 .ui 文件）
 */
void DADialogTableDisplayFormat::setupUi(const DAPyDType& dtype)
{
    setWindowTitle(tr("Format Cells"));  // cn:设置单元格格式
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 预览
    QGroupBox* previewBox = new QGroupBox(tr("Preview"), this);  // cn:预览
    QVBoxLayout* pl        = new QVBoxLayout(previewBox);
    mPreviewLabel          = new QLabel(previewBox);
    mPreviewLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    pl->addWidget(mPreviewLabel);
    mainLayout->addWidget(previewBox);

    // 中部：类别列表 + 选项堆栈
    QHBoxLayout* midLayout = new QHBoxLayout();
    mainLayout->addLayout(midLayout, 1);

    // 类别列表
    QGroupBox* catBox = new QGroupBox(tr("Category"), this);  // cn:类别
    QVBoxLayout* cl    = new QVBoxLayout(catBox);
    mCategoryList      = new QListWidget(catBox);
    struct CatItem
    {
        DATableDisplayFormat::Category c;
        const char* txt;
    };
    static const CatItem items[] = {
        { DATableDisplayFormat::General, "General" },
        { DATableDisplayFormat::Number, "Number" },
        { DATableDisplayFormat::Scientific, "Scientific" },
        { DATableDisplayFormat::Percent, "Percentage" },
        { DATableDisplayFormat::DateTime, "Date/Time" },
        { DATableDisplayFormat::DatetimeAsNumber, "Datetime as Number" },
        { DATableDisplayFormat::Text, "Text" },
    };
    bool noneType = dtype.isNone();
    bool numeric  = !noneType && (dtype.isFloat() || dtype.isInt() || dtype.isUInt()
                                  || dtype.isNullableInt() || dtype.isNullableUInt());
    bool datetime = !noneType && (dtype.isDatetime() || dtype.isDatetimeTZ());
    for (const CatItem& it : items) {
        QListWidgetItem* li = new QListWidgetItem(tr(it.txt), mCategoryList);
        li->setData(Qt::UserRole, static_cast< int >(it.c));
        bool enabled = false;
        switch (it.c) {
        case DATableDisplayFormat::General:
        case DATableDisplayFormat::Text:
            enabled = true;
            break;
        case DATableDisplayFormat::Number:
        case DATableDisplayFormat::Scientific:
        case DATableDisplayFormat::Percent:
            enabled = numeric;
            break;
        case DATableDisplayFormat::DateTime:
        case DATableDisplayFormat::DatetimeAsNumber:
            enabled = datetime;
            break;
        }
        if (!enabled) {
            li->setFlags(li->flags() & ~Qt::ItemIsEnabled);
            li->setBackground(Qt::gray);
        }
        mCategoryList->addItem(li);
    }
    cl->addWidget(mCategoryList);
    midLayout->addWidget(catBox);

    // 选项堆栈
    QGroupBox* optBox = new QGroupBox(tr("Options"), this);  // cn:选项
    QVBoxLayout* ol    = new QVBoxLayout(optBox);
    mStack             = new QStackedWidget(optBox);

    // page 0: General
    mStack->addWidget(new QLabel(tr("No options (default display)"), mStack));  // cn:无选项（默认显示）
    // page 1: 精度（Number/Scientific/Percent 共用）
    {
        QWidget* w = new QWidget(mStack);
        QFormLayout* f = new QFormLayout(w);
        mSpinPrecision = new QSpinBox(w);
        mSpinPrecision->setRange(0, 20);
        mSpinPrecision->setValue(2);
        f->addRow(tr("Decimal places:"), mSpinPrecision);  // cn:小数位数:
        mStack->addWidget(w);
    }
    // page 2: DateTime
    {
        QWidget* w = new QWidget(mStack);
        QFormLayout* f = new QFormLayout(w);
        mComboDateTimePreset = new QComboBox(w);
        mComboDateTimePreset->addItems(DATableDisplayFormat::dateTimePresets());
        mComboDateTimePreset->addItem(tr("(custom)"));  // cn:(自定义)
        mEditDateTimePattern = new QLineEdit(w);
        f->addRow(tr("Preset:"), mComboDateTimePreset);    // cn:预设:
        f->addRow(tr("Pattern:"), mEditDateTimePattern);   // cn:格式串:
        mStack->addWidget(w);
    }
    // page 3: DatetimeAsNumber
    {
        QWidget* w = new QWidget(mStack);
        QVBoxLayout* v = new QVBoxLayout(w);
        mRadioEpochSeconds = new QRadioButton(tr("Seconds since epoch"), w);  // cn:epoch 秒
        mRadioEpochMillis  = new QRadioButton(tr("Milliseconds since epoch"), w);  // cn:epoch 毫秒
        mRadioEpochMillis->setChecked(true);
        v->addWidget(mRadioEpochSeconds);
        v->addWidget(mRadioEpochMillis);
        v->addStretch();
        mStack->addWidget(w);
    }
    // page 4: Text
    mStack->addWidget(new QLabel(tr("Display as plain text"), mStack));  // cn:按纯文本显示
    ol->addWidget(mStack);
    midLayout->addWidget(optBox, 1);

    // 按钮
    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 信号
    connect(mCategoryList, &QListWidget::currentRowChanged, this, [ this ](int row) {
        if (row < 0) {
            return;
        }
        auto c = static_cast< DATableDisplayFormat::Category >(mCategoryList->item(row)->data(Qt::UserRole).toInt());
        switchToCategory(c);
        updatePreview();
    });
    connect(mSpinPrecision, QOverload< int >::of(&QSpinBox::valueChanged), this, [ this ]() { updatePreview(); });
    connect(mComboDateTimePreset,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            [ this ](int index) {
                if (index >= 0 && index < mComboDateTimePreset->count() - 1) {
                    mEditDateTimePattern->setText(mComboDateTimePreset->itemText(index));
                }
                updatePreview();
            });
    connect(mEditDateTimePattern, &QLineEdit::textChanged, this, [ this ]() { updatePreview(); });
    connect(mRadioEpochSeconds, &QRadioButton::toggled, this, [ this ]() { updatePreview(); });
}

/**
 * @brief 切换选项堆栈到类别
 */
void DADialogTableDisplayFormat::switchToCategory(DATableDisplayFormat::Category c)
{
    int page = 0;  // General
    switch (c) {
    case DATableDisplayFormat::General:
        page = 0;
        break;
    case DATableDisplayFormat::Number:
    case DATableDisplayFormat::Scientific:
    case DATableDisplayFormat::Percent:
        page = 1;
        break;
    case DATableDisplayFormat::DateTime:
        page = 2;
        break;
    case DATableDisplayFormat::DatetimeAsNumber:
        page = 3;
        break;
    case DATableDisplayFormat::Text:
        page = 4;
        break;
    }
    mStack->setCurrentIndex(page);
}

/**
 * @brief 更新预览
 */
void DADialogTableDisplayFormat::updatePreview()
{
    DATableDisplayFormat fmt = buildFormat();
    QString preview = fmt.isValid() ? fmt.formatValue(mSample) : mSample.toString();
    mPreviewLabel->setText(preview);
}

/**
 * @brief 根据当前 UI 状态构造格式
 * @return 格式，General 返回 invalid（表示清除）
 */
DATableDisplayFormat DADialogTableDisplayFormat::buildFormat() const
{
    if (mCategoryList->currentRow() < 0) {
        return DATableDisplayFormat();
    }
    auto c = static_cast< DATableDisplayFormat::Category >(
        mCategoryList->currentItem()->data(Qt::UserRole).toInt());
    if (c == DATableDisplayFormat::General) {
        return DATableDisplayFormat();  // invalid → 清除
    }
    DATableDisplayFormat fmt(c);  // valid
    switch (c) {
    case DATableDisplayFormat::Number:
    case DATableDisplayFormat::Scientific:
    case DATableDisplayFormat::Percent:
        fmt.setPrecision(mSpinPrecision->value());
        break;
    case DATableDisplayFormat::DateTime:
        fmt.setPattern(mEditDateTimePattern->text().trimmed().isEmpty()
                            ? DATableDisplayFormat::defaultDateTimePattern()
                            : mEditDateTimePattern->text().trimmed());
        break;
    case DATableDisplayFormat::DatetimeAsNumber:
        fmt.setEpochUnit(mRadioEpochSeconds->isChecked() ? DATableDisplayFormat::EpochSeconds
                                                         : DATableDisplayFormat::EpochMilliseconds);
        break;
    default:
        break;
    }
    return fmt;
}

/**
 * @brief 获取结果格式
 * @return OK 返回 buildFormat()，General 返回 invalid
 */
DATableDisplayFormat DADialogTableDisplayFormat::getResult() const
{
    return buildFormat();
}

}  // end of namespace DA
