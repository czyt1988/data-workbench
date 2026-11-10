#include "DASignalImportDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QDebug>
#include "DADataManagerInterface.h"
#include "Table/Views/DAPySeriesListView.h"
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
 *
 * 选列控件按 Q5 决定 ① 使用 DAPySeriesListView（评审 I6）：时间列与幅值列各一个列表，
 * 列表项由 DADataManagerTreeModel 的 series item 提供（dtype 图标 + 名称/长度描述），
 * 预填候选用 addSeries() 完成；时间列单选、幅值列多选用 QListView 的选择模式实现。
 */
void DASignalImportDialog::buildDataFrameUI()
{
    QVBoxLayout* lay  = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout;

    mDataCombo = new QComboBox(this);
    mDataCombo->setObjectName(QStringLiteral("da-signal-import-datacombo"));
    mDataLabel = new QLabel(this);
    form->addRow(mDataLabel, mDataCombo);

    // 时间列：单选取一列（预填候选需要 addSeries 多次，故内部用多序列模式）
    mTimeColumnList = new DAPySeriesListView(this);
    mTimeColumnList->setObjectName(QStringLiteral("da-signal-import-time-column"));
    mTimeColumnList->setAcceptMode(DAPySeriesListView::AcceptMultDataframeMultSeries);
    mTimeColumnList->setSelectionMode(QAbstractItemView::SingleSelection);
    mTimeColumnList->setMinimumHeight(80);
    mTimeLabel = new QLabel(this);
    form->addRow(mTimeLabel, mTimeColumnList);

    // 「无时间列」显式入口：取代表单原来的首项「No time column」（评审 I7），勾选后按手动采样率导入
    mNoTimeColumnCheck = new QCheckBox(this);
    form->addRow(QString(), mNoTimeColumnCheck);

    // 幅值列：多选即多通道
    mValueColumnList = new DAPySeriesListView(this);
    mValueColumnList->setObjectName(QStringLiteral("da-signal-import-value-columns"));
    mValueColumnList->setAcceptMode(DAPySeriesListView::AcceptMultDataframeMultSeries);
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
    // 时间列选中行变化 → 重新推断采样率（选择模型信号参数多，槽可少收）
    connect(mTimeColumnList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &DASignalImportDialog::onTimeColumnChanged);
    connect(mValueColumnList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &DASignalImportDialog::onValueSelectionChanged);
    connect(mNoTimeColumnCheck, &QCheckBox::toggled, this, &DASignalImportDialog::onNoTimeColumnToggled);
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
        setStatusMessage(StatusNoDataFrame);
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
 * @brief 刷新时间列/幅值列两个序列列表
 *
 * DAPySeriesListView 没有 clear()，用 removeSeries() 同时清模型行与内部序列缓存；
 * 预填用 addSeries(dataframe, 列名)，列表项自带 dtype 图标与「df.列,长度」描述。
 */
void DASignalImportDialog::refreshColumnLists()
{
    if (mTimeColumnList == nullptr || mValueColumnList == nullptr) {
        return;
    }
    mUpdatingUI = true;
    clearSeriesList(mTimeColumnList);
    clearSeriesList(mValueColumnList);
    const DAData d = selectedData();
    if (d.isDataFrame()) {
        try {
            DAPyDataFrame df            = d.toDataFrame();
            const QList< QString > cols = df.columns();
            for (const QString& c : cols) {
                mTimeColumnList->addSeries(d, c);
                mValueColumnList->addSeries(d, c);
            }
        } catch (const std::exception& e) {
            qWarning() << "DASignalImportDialog::refreshColumnLists:" << e.what();
            setStatusMessage(StatusDetail, QString::fromUtf8(e.what()));
        }
    }
    mUpdatingUI = false;
    // 换了数据帧，之前的「无时间列」选择失效：恢复成让用户重新指认
    if (mNoTimeColumnCheck) {
        QSignalBlocker blocker(mNoTimeColumnCheck);
        mNoTimeColumnCheck->setChecked(false);
    }
    if (mTimeColumnList) {
        mTimeColumnList->setEnabled(true);
    }
    refreshInference();
}

/**
 * @brief 清空一个序列列表（clear() 不存在，按内部序列逐条 removeSeries）
 */
void DASignalImportDialog::clearSeriesList(DAPySeriesListView* list)
{
    if (nullptr == list) {
        return;
    }
    const QList< QPair< DAData, QStringList > > series = list->getSeries();
    for (const auto& pair : series) {
        for (const QString& name : pair.second) {
            list->removeSeries(pair.first, name);
        }
    }
}

/**
 * @brief 时间列选择变化：重算推断采样率并预填
 */
void DASignalImportDialog::onTimeColumnChanged()
{
    if (mUpdatingUI) {
        return;
    }
    // 已经勾选「无时间列」时，选中行不应生效（选择变化只可能来自程序化清理）
    if (mNoTimeColumnCheck && mNoTimeColumnCheck->isChecked()) {
        return;
    }
    refreshInference();
}

/**
 * @brief 「无时间列」勾选：清掉时间列选中行，改走手动填写采样率路径
 */
void DASignalImportDialog::onNoTimeColumnToggled(bool checked)
{
    if (mUpdatingUI) {
        return;
    }
    if (mTimeColumnList) {
        if (checked && mTimeColumnList->selectionModel()) {
            mTimeColumnList->selectionModel()->clearSelection();
            mTimeColumnList->selectionModel()->clearCurrentIndex();
        }
        // 勾选「无时间列」时禁用列表，避免选择与勾选语义互相打架
        mTimeColumnList->setEnabled(!checked);
    }
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
        setStatusMessage(StatusNeedPositiveSampleRate);
    } else {
        setStatusMessage(StatusNone);
    }
    updateOkEnabled();
}

/**
 * @brief 按时间列重算推断采样率与时长，并预填输入框
 *
 * 规则（方案 §4.4）：
 * - 有时间列：fs = 1 / median(diff(t))，预填；
 * - 无时间列：采样率清空，确定按钮置灰并给内联提示。
 * 推断结果缓存在 mInferenceState，语言切换时由 updateInferenceText() 按状态重新取词（评审 I7）。
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
        mInferenceState = InferenceNoTimeColumn;
        mSampleRateSpin->setValue(0.0);
        mUpdatingUI = false;
        updateInferenceText();
        updateOkEnabled();
        return;
    }
    double fs = 0.0, dur = 0.0;
    QString err;
    if (DASignalImporter::inferSampleRate(d, timeCol, &fs, &dur, &err)) {
        mInferenceState    = InferenceOk;
        mInferredRate      = fs;
        mInferredDuration  = dur;
        mSampleRateSpin->setValue(fs);
        setStatusMessage(StatusNone);
    } else {
        mInferenceState  = InferenceFailed;
        mInferenceError  = err;
        mSampleRateSpin->setValue(0.0);
    }
    mUpdatingUI = false;
    updateInferenceText();
    updateOkEnabled();
}

/**
 * @brief 按当前推断状态渲染提示文本（语言切换时也走这里，评审 I7）
 */
void DASignalImportDialog::updateInferenceText()
{
    if (nullptr == mInferenceLabel) {
        return;
    }
    switch (mInferenceState) {
    case InferenceNoTimeColumn:
        mInferenceLabel->setText(tr("No time column: enter the sample rate manually."));
        // cn:无时间列：请手动填写采样率。
        break;
    case InferenceOk:
        mInferenceLabel->setText(tr("Inferred sample rate = %1 Hz, duration = %2 s")
                                     .arg(mInferredRate, 0, 'f', 3)
                                     .arg(mInferredDuration, 0, 'f', 3));
        // cn:推断采样率 = %1 赫兹，时长 = %2 秒
        break;
    case InferenceFailed:
        mInferenceLabel->setText(tr("Cannot infer the sample rate from the time column: %1").arg(mInferenceError));
        // cn:无法从时间列推断采样率：%1
        break;
    case InferenceNone:
    default:
        mInferenceLabel->clear();
        break;
    }
}

/**
 * @brief 内联状态提示（字段校验不用模态消息框）
 *
 * 只记录状态种类与明细，文本在 updateStatusText() 里取词渲染；
 * 语言切换时 retranslateUi() 会按同一种类重新取 tr 文本（评审 I7）。
 */
void DASignalImportDialog::setStatusMessage(StatusKind kind, const QString& detail)
{
    mStatusKind   = kind;
    mStatusDetail = detail;
    updateStatusText();
}

/**
 * @brief 按当前状态渲染内联提示文本（含语言切换后的重译）
 */
void DASignalImportDialog::updateStatusText()
{
    if (nullptr == mStatusLabel) {
        return;
    }
    QString text;
    bool isError = true;
    switch (mStatusKind) {
    case StatusNoDataFrame:
        text = tr("No data frame is available. Import data first.");  // cn:没有可用的数据帧，请先导入数据。
        break;
    case StatusNeedPositiveSampleRate:
        text = tr("Please enter a positive sample rate.");  // cn:请输入合法的正数采样率。
        break;
    case StatusNoFFmpeg:
        text = tr("FFmpeg was not detected. Specify its path in Settings > Signal Analysis.");
        // cn:未检测到 FFmpeg，请在「设置 → 信号分析」里指定路径。
        break;
    case StatusDetail:
        text = mStatusDetail;
        break;
    case StatusNone:
    default:
        isError = false;
        break;
    }
    mStatusLabel->setText(text);
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

/**
 * @brief 当前选中的时间列名（未选中 = 无时间列）
 */
QString DASignalImportDialog::selectedTimeColumn() const
{
    if (mNoTimeColumnCheck && mNoTimeColumnCheck->isChecked()) {
        return QString();
    }
    if (nullptr == mTimeColumnList || nullptr == mTimeColumnList->selectionModel()) {
        return QString();
    }
    const QModelIndexList selected = mTimeColumnList->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        return QString();
    }
    return selected.first().data(Qt::DisplayRole).toString();
}

/**
 * @brief 当前选中的幅值列（可多选，顺序按列表顺序）
 */
QStringList DASignalImportDialog::selectedValueColumns() const
{
    QStringList cols;
    if (nullptr == mValueColumnList || nullptr == mValueColumnList->selectionModel()) {
        return cols;
    }
    const QModelIndexList selected = mValueColumnList->selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : selected) {
        const QString name = idx.data(Qt::DisplayRole).toString();
        if (!name.isEmpty() && !cols.contains(name)) {
            cols.append(name);
        }
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
    if (mNoTimeColumnCheck) {
        mNoTimeColumnCheck->setText(tr("No time column"));  // cn:无时间列
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
    } else {
        // 推断提示（无时间列 / 推断成功 / 推断失败）随语言重新取词（评审 I7）
        updateInferenceText();
    }
    if (mButtonBox) {
        mButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Import"));  // cn:导入
        mButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    }
    if (mMode == VideoMode && mStatusLabel && !DASignalImporter::isFFmpegAvailable()) {
        setStatusMessage(StatusNoFFmpeg);
    } else {
        // 数据帧模式的状态提示（无可用数据帧 / 采样率非法等）随语言刷新（评审 I7）
        updateStatusText();
    }
}
}  // namespace DA
