#include "DASignalImportDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include "DADataManagerInterface.h"
#include "pandas/DAPyDataFrame.h"
#include "Signal/DASignalImporter.h"

namespace DA
{
DASignalImportDialog::DASignalImportDialog(Mode mode, DADataManagerInterface* dataMgr, QWidget* parent)
    : QDialog(parent), mMode(mode), mDataMgr(dataMgr)
{
    setModal(true);
    setMinimumWidth(460);
    if (mMode == DataFrameMode) {
        buildDataFrameUI();
    } else {
        buildAudioVideoUI();
    }
    // 按钮区
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout()->addWidget(mButtonBox);
    retranslateUi();
    updateOkEnabled();
}

DASignalImportDialog::~DASignalImportDialog()
{
}

DASignalImportDialog::Mode DASignalImportDialog::mode() const
{
    return mMode;
}

// ---------------------------------------------------------------- UI 构建

/**
 * @brief 音频/视频模式界面：文件路径 + 浏览按钮 + 文件信息（只读采样率）
 */
void DASignalImportDialog::buildAudioVideoUI()
{
    QVBoxLayout* lay  = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout;
    mFilePathEdit     = new QLineEdit(this);
    mFilePathEdit->setObjectName(QStringLiteral("da-signal-import-file-path"));
    mBrowseButton = new QPushButton(this);
    QHBoxLayout* fileRow = new QHBoxLayout;
    fileRow->addWidget(mFilePathEdit, 1);
    fileRow->addWidget(mBrowseButton);
    mFileLabel = new QLabel(this);
    form->addRow(mFileLabel, fileRow);
    // 文件采样率只读展示（评审 I4）：直读格式探测文件头，需转码格式提示导入后显示
    mFileInfoLabel = new QLabel(this);
    mFileInfoLabel->setWordWrap(true);
    mFileInfoLabel->setStyleSheet(QStringLiteral("color:#727272;"));
    form->addRow(QString(), mFileInfoLabel);
    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(true);
    form->addRow(QString(), mStatusLabel);
    lay->addLayout(form);
    connect(mBrowseButton, &QPushButton::clicked, this, &DASignalImportDialog::onBrowseFile);
    connect(mFilePathEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        updateFileInfo();
        updateOkEnabled();
    });
}

/**
 * @brief 探测所选文件的采样率/通道/时长并只读展示
 */
void DASignalImportDialog::updateFileInfo()
{
    if (!mFileInfoLabel) {
        return;
    }
    const QString path = mFilePathEdit ? mFilePathEdit->text().trimmed() : QString();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        mFileInfoLabel->clear();
        return;
    }
    double fs = 0.0, duration = 0.0;
    int channels = 0;
    QString err;
    if (DASignalImporter::probeAudioFile(path, &fs, &channels, &duration, &err)) {
        mFileInfoLabel->setStyleSheet(QStringLiteral("color:#727272;"));
        mFileInfoLabel->setText(tr("Sample rate: %1 Hz, channels: %2, duration: %3 s")
                                    .arg(fs, 0, 'f', 3)
                                    .arg(channels)
                                    .arg(duration, 0, 'f', 3));
        // cn:采样率：%1 赫兹，通道数：%2，时长：%3 秒
    } else {
        mFileInfoLabel->setStyleSheet(QStringLiteral("color:#727272;"));
        mFileInfoLabel->setText(err);
    }
}

/**
 * @brief 数据帧模式界面：源数据帧 / 时间列 / 幅值列 / 采样率 / 信号名
 */
void DASignalImportDialog::buildDataFrameUI()
{
    QVBoxLayout* lay  = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout;

    mDataCombo = new QComboBox(this);
    mDataCombo->setObjectName(QStringLiteral("da-signal-import-datacombo"));
    mDataLabel = new QLabel(this);
    form->addRow(mDataLabel, mDataCombo);

    mTimeColumnCombo = new QComboBox(this);
    mTimeLabel       = new QLabel(this);
    form->addRow(mTimeLabel, mTimeColumnCombo);

    mValueColumnList = new QListWidget(this);
    mValueColumnList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mValueColumnList->setMinimumHeight(110);
    mValueLabel = new QLabel(this);
    form->addRow(mValueLabel, mValueColumnList);

    mSampleRateSpin = new QDoubleSpinBox(this);
    mSampleRateSpin->setDecimals(3);
    mSampleRateSpin->setRange(0.0, 1.0e9);
    mSampleRateSpin->setSingleStep(1000.0);
    mSampleRateSpin->setSpecialValueText(QStringLiteral(""));
    mSampleRateLabel = new QLabel(this);
    form->addRow(mSampleRateLabel, mSampleRateSpin);

    mInferenceLabel = new QLabel(this);
    mInferenceLabel->setWordWrap(true);
    mInferenceLabel->setStyleSheet(QStringLiteral("color:#727272;"));  // 中性次要文字
    form->addRow(QString(), mInferenceLabel);

    mNameEdit  = new QLineEdit(this);
    mNameLabel = new QLabel(this);
    form->addRow(mNameLabel, mNameEdit);

    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(true);
    form->addRow(QString(), mStatusLabel);

    lay->addLayout(form);

    connect(mDataCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DASignalImportDialog::onSourceDataChanged);
    connect(mTimeColumnCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DASignalImportDialog::onTimeColumnChanged);
    connect(mValueColumnList, &QListWidget::itemSelectionChanged, this, &DASignalImportDialog::onValueSelectionChanged);
    connect(mSampleRateSpin, QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this, &DASignalImportDialog::onSampleRateEdited);

    refreshDataFrameList();
}

/**
 * @brief 刷新数据帧下拉（只列数据管理器里的数据帧）
 */
void DASignalImportDialog::refreshDataFrameList()
{
    if (nullptr == mDataCombo || nullptr == mDataMgr) {
        return;
    }
    mUpdatingUI = true;
    mDataCombo->clear();
    mDataFrames.clear();
    const QList< DAData > all = mDataMgr->getAllDatas();
    for (const DAData& d : all) {
        if (d.isDataFrame()) {
            mDataFrames.append(d);
            mDataCombo->addItem(d.getName());
        }
    }
    mUpdatingUI = false;
    if (mDataFrames.isEmpty()) {
        setStatusMessage(tr("No data frame is available. Import data first."), true);  // cn:没有可用的数据帧，请先导入数据。
    }
    onSourceDataChanged(mDataCombo->currentIndex());
}

/**
 * @brief 源数据帧变化：刷新时间列与幅值列列表
 */
void DASignalImportDialog::onSourceDataChanged(int index)
{
    Q_UNUSED(index)
    refreshColumnLists();
}

/**
 * @brief 刷新列下拉与多选列表
 */
void DASignalImportDialog::refreshColumnLists()
{
    if (mTimeColumnCombo == nullptr || mValueColumnList == nullptr) {
        return;
    }
    mUpdatingUI = true;
    mTimeColumnCombo->clear();
    mValueColumnList->clear();
    mTimeColumnCombo->addItem(tr("No time column"), QString());  // cn:无时间列
    const DAData d = selectedData();
    if (d.isDataFrame()) {
        try {
            DAPyDataFrame df = d.toDataFrame();
            const QList< QString > cols = df.columns();
            for (const QString& c : cols) {
                mTimeColumnCombo->addItem(c, c);
                auto item = new QListWidgetItem(c, mValueColumnList);
                item->setData(Qt::UserRole, c);
            }
        } catch (const std::exception& e) {
            qWarning() << "DASignalImportDialog::refreshColumnLists:" << e.what();
            setStatusMessage(QString::fromUtf8(e.what()), true);
        }
    }
    mUpdatingUI = false;
    onTimeColumnChanged(mTimeColumnCombo->currentIndex());
}

/**
 * @brief 时间列变化：重算推断采样率并预填
 */
void DASignalImportDialog::onTimeColumnChanged(int index)
{
    Q_UNUSED(index)
    refreshInference();
}

/**
 * @brief 幅值列选择变化
 */
void DASignalImportDialog::onValueSelectionChanged()
{
    if (mUpdatingUI) {
        return;
    }
    // 默认信号名 = 数据帧名.首个幅值列
    const DAData d = selectedData();
    const QStringList cols = selectedValueColumns();
    if (d.isDataFrame() && !cols.isEmpty()) {
        mNameEdit->setText(QStringLiteral("%1.%2").arg(d.getName(), cols.first()));
    }
    updateOkEnabled();
}

/**
 * @brief 采样率手动编辑
 */
void DASignalImportDialog::onSampleRateEdited(double v)
{
    if (mUpdatingUI) {
        return;
    }
    if (v <= 0.0) {
        setStatusMessage(tr("Please enter a positive sample rate."), true);  // cn:请输入合法的正数采样率。
    } else {
        setStatusMessage(QString(), false);
    }
    updateOkEnabled();
}

/**
 * @brief 按时间列重算推断采样率与时长，并预填输入框
 *
 * 规则（方案 §4.4）：
 * - 有时间列：fs = 1 / median(diff(t))，预填；
 * - 无时间列：采样率清空，确定按钮置灰并给内联提示。
 */
void DASignalImportDialog::refreshInference()
{
    if (mSampleRateSpin == nullptr || mInferenceLabel == nullptr) {
        return;
    }
    const QString timeCol = selectedTimeColumn();
    const DAData d        = selectedData();
    mUpdatingUI           = true;
    if (timeCol.isEmpty() || !d.isDataFrame()) {
        mSampleRateSpin->setValue(0.0);
        mInferenceLabel->setText(tr("No time column: enter the sample rate manually."));
        // cn:无时间列：请手动填写采样率。
        mUpdatingUI = false;
        updateOkEnabled();
        return;
    }
    double fs = 0.0, dur = 0.0;
    QString err;
    if (DASignalImporter::inferSampleRate(d, timeCol, &fs, &dur, &err)) {
        mSampleRateSpin->setValue(fs);
        mInferenceLabel->setText(tr("Inferred sample rate = %1 Hz, duration = %2 s")
                                     .arg(fs, 0, 'f', 3)
                                     .arg(dur, 0, 'f', 3));
        // cn:推断采样率 = %1 赫兹，时长 = %2 秒
        setStatusMessage(QString(), false);
    } else {
        mSampleRateSpin->setValue(0.0);
        mInferenceLabel->setText(tr("Cannot infer the sample rate from the time column: %1").arg(err));
        // cn:无法从时间列推断采样率：%1
    }
    mUpdatingUI = false;
    updateOkEnabled();
}

/**
 * @brief 内联状态提示（字段校验不用模态消息框）
 */
void DASignalImportDialog::setStatusMessage(const QString& msg, bool isError)
{
    if (nullptr == mStatusLabel) {
        return;
    }
    mStatusLabel->setText(msg);
    // 错误用橙红 #CE6043，正常提示用中性灰 #727272（DESIGN.md §2.3）
    mStatusLabel->setStyleSheet(isError ? QStringLiteral("color:#CE6043;") : QStringLiteral("color:#727272;"));
}

/**
 * @brief 校验并更新确定按钮可用性
 */
void DASignalImportDialog::updateOkEnabled()
{
    if (nullptr == mButtonBox) {
        return;
    }
    bool ok = false;
    if (mMode == DataFrameMode) {
        ok = !selectedData().getName().isEmpty() && !selectedValueColumns().isEmpty() && sampleRate() > 0.0;
    } else {
        const QString path = filePath();
        ok                = !path.isEmpty() && QFileInfo::exists(path);
        if (ok && mMode == VideoMode && !DASignalImporter::isFFmpegAvailable()) {
            ok = false;
        }
    }
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

// ---------------------------------------------------------------- 交互

/**
 * @brief 浏览选择文件
 */
void DASignalImportDialog::onBrowseFile()
{
    QString filter;
    if (mMode == VideoMode) {
        filter = tr("Video Files (*.mp4 *.mov *.m4v *.avi *.mkv *.webm *.flv *.wmv *.ts *.mts);;All Files (*)");
        // cn:视频文件
    } else {
        filter = tr("Audio Files (*.wav *.aiff *.aif *.flac *.ogg *.oga *.mp3 *.m4a *.aac *.wma *.opus);;All Files (*)");
        // cn:音频文件
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Select File"), QString(), filter);  // cn:选择文件
    if (!path.isEmpty()) {
        mFilePathEdit->setText(QDir::toNativeSeparators(path));
    }
}

// ---------------------------------------------------------------- 结果

QString DASignalImportDialog::filePath() const
{
    return mFilePathEdit ? mFilePathEdit->text().trimmed() : QString();
}

DAData DASignalImportDialog::selectedData() const
{
    if (nullptr == mDataCombo) {
        return DAData();
    }
    const int idx = mDataCombo->currentIndex();
    if (idx < 0 || idx >= mDataFrames.size()) {
        return DAData();
    }
    return mDataFrames.at(idx);
}

QString DASignalImportDialog::selectedTimeColumn() const
{
    if (nullptr == mTimeColumnCombo) {
        return QString();
    }
    return mTimeColumnCombo->currentData().toString();
}

QStringList DASignalImportDialog::selectedValueColumns() const
{
    QStringList cols;
    if (nullptr == mValueColumnList) {
        return cols;
    }
    const QList< QListWidgetItem* > items = mValueColumnList->selectedItems();
    for (const QListWidgetItem* item : items) {
        cols.append(item->data(Qt::UserRole).toString());
    }
    return cols;
}

double DASignalImportDialog::sampleRate() const
{
    return mSampleRateSpin ? mSampleRateSpin->value() : 0.0;
}

QString DASignalImportDialog::signalName() const
{
    return mNameEdit ? mNameEdit->text().trimmed() : QString();
}

// ---------------------------------------------------------------- 文本

void DASignalImportDialog::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(e);
}

void DASignalImportDialog::retranslateUi()
{
    switch (mMode) {
    case AudioMode:
        setWindowTitle(tr("Import Audio"));  // cn:导入音频
        break;
    case VideoMode:
        setWindowTitle(tr("Import Video"));  // cn:导入视频
        break;
    case DataFrameMode:
    default:
        setWindowTitle(tr("Import Data Frame"));  // cn:导入数据帧
        break;
    }
    if (mBrowseButton) {
        mBrowseButton->setText(tr("Browse..."));  // cn:浏览
    }
    // 表单行标签随语言切换（评审 I7）
    if (mFileLabel) {
        mFileLabel->setText(tr("File"));  // cn:文件
    }
    if (mDataLabel) {
        mDataLabel->setText(tr("Data Frame"));  // cn:源数据帧
    }
    if (mTimeLabel) {
        mTimeLabel->setText(tr("Time Column"));  // cn:时间列
    }
    if (mValueLabel) {
        mValueLabel->setText(tr("Value Columns"));  // cn:幅值列（可多选）
    }
    if (mSampleRateLabel) {
        mSampleRateLabel->setText(tr("Sample Rate (Hz)"));  // cn:采样率（赫兹）
    }
    if (mNameLabel) {
        mNameLabel->setText(tr("Signal Name"));  // cn:信号名
    }
    if (mMode != DataFrameMode) {
        updateFileInfo();  // 文件信息提示随语言刷新
    }
    if (mButtonBox) {
        mButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Import"));  // cn:导入
        mButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    }
    if (mMode == VideoMode && mStatusLabel && !DASignalImporter::isFFmpegAvailable()) {
        setStatusMessage(tr("FFmpeg was not detected. Specify its path in the signal analysis settings."), true);
        // cn:未检测到 FFmpeg，请在分析设置里指定路径。
    }
}
}  // namespace DA
