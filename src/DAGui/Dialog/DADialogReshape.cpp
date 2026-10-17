#include "DADialogReshape.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAWaitCursorScoped.h"
#include <QRadioButton>
#include <QListWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QMessageBox>

namespace DA
{

/**
 * @brief 构造
 * @param df 数据（只读；预览在 head 样本副本上做，不触碰原表）
 * @param meltMode true=初始为宽转长模式，false=转置模式（模式可切换）
 * @param parent 父窗口
 */
DADialogReshape::DADialogReshape(const DAPyDataFrame& df, bool meltMode, QWidget* parent) : QDialog(parent), mDf(df)
{
    setupUI();
    mRadioMelt->setChecked(meltMode);
    mRadioTranspose->setChecked(!meltMode);
    onModeChanged();
    setWindowTitle(tr("Reshape"));  // cn:形状变换
    runPreview();
}

DADialogReshape::~DADialogReshape()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）：左=模式与参数，右=状态+预览
 */
void DADialogReshape::setupUI()
{
    setModal(true);
    const auto shape = mDf.isNone() ? std::make_pair(static_cast< std::size_t >(0), static_cast< std::size_t >(0)) : mDf.shape();
    mSrcRows         = static_cast< qint64 >(shape.first);
    const QList< QString > cols = mDf.columns();

    QWidget* leftPanel   = new QWidget(this);
    QVBoxLayout* leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    QGroupBox* modeGroup = new QGroupBox(tr("Mode"), leftPanel);  // cn:模式
    QVBoxLayout* modeLay = new QVBoxLayout(modeGroup);
    mRadioTranspose      = new QRadioButton(tr("Transpose (rows become columns)"), modeGroup);  // cn:转置（行列互换）
    mRadioMelt           = new QRadioButton(tr("Unpivot / Melt (wide to long)"), modeGroup);    // cn:宽转长（melt）
    mRadioTranspose->setChecked(true);
    modeLay->addWidget(mRadioTranspose);
    modeLay->addWidget(mRadioMelt);
    leftLay->addWidget(modeGroup);

    // melt 参数面板（转置模式隐藏）
    mMeltPanel                 = new QWidget(leftPanel);
    QVBoxLayout* meltLay       = new QVBoxLayout(mMeltPanel);
    meltLay->setContentsMargins(0, 0, 0, 0);
    QGroupBox* idGroup         = new QGroupBox(tr("ID columns (keep, multi-select)"), mMeltPanel);  // cn:保留列（id_vars，多选）
    QVBoxLayout* idLay         = new QVBoxLayout(idGroup);
    idLay->setContentsMargins(4, 4, 4, 4);
    mListIdVars                = new QListWidget(idGroup);
    mListIdVars->setObjectName(QStringLiteral("listIdVars"));
    mListIdVars->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const QString& c : cols) {
        mListIdVars->addItem(c);
    }
    idLay->addWidget(mListIdVars);
    meltLay->addWidget(idGroup, 2);
    QGroupBox* valueGroup = new QGroupBox(tr("Value columns (unpivot; none = all others)"), mMeltPanel);  // cn:展开列（value_vars；不选=其余全部列）
    QVBoxLayout* valueLay = new QVBoxLayout(valueGroup);
    valueLay->setContentsMargins(4, 4, 4, 4);
    mListValueVars        = new QListWidget(valueGroup);
    mListValueVars->setObjectName(QStringLiteral("listValueVars"));
    mListValueVars->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const QString& c : cols) {
        mListValueVars->addItem(c);
    }
    valueLay->addWidget(mListValueVars);
    meltLay->addWidget(valueGroup, 2);
    QFormLayout* nameForm = new QFormLayout();
    mEditVarName          = new QLineEdit(QStringLiteral("variable"), mMeltPanel);
    mEditVarName->setObjectName(QStringLiteral("lineEditVarName"));
    nameForm->addRow(tr("Variable column name:"), mEditVarName);  // cn:变量名列名：
    mEditValueName = new QLineEdit(QStringLiteral("value"), mMeltPanel);
    mEditValueName->setObjectName(QStringLiteral("lineEditValueName"));
    nameForm->addRow(tr("Value column name:"), mEditValueName);  // cn:值列名：
    meltLay->addLayout(nameForm);
    leftLay->addWidget(mMeltPanel, 4);

    // 就地替换勾选（R14：默认产出新数据集）
    mCheckInplace = new QCheckBox(tr("Replace current table in place (default: create a new dataset)"), leftPanel);  // cn:就地替换当前表（默认：产出新数据集）
    mCheckInplace->setObjectName(QStringLiteral("checkInplace"));
    mCheckInplace->setChecked(false);
    leftLay->addWidget(mCheckInplace);

    QWidget* rightPanel   = new QWidget(this);
    QVBoxLayout* rightLay = new QVBoxLayout(rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    mLabelStatus = new QLabel(rightPanel);
    mLabelStatus->setWordWrap(true);
    rightLay->addWidget(mLabelStatus);
    QGroupBox* previewGroup = new QGroupBox(tr("Preview (first %1 rows of a sample)").arg(c_previewRows), rightPanel);  // cn:预览（样本前 %1 行）
    QVBoxLayout* previewLay = new QVBoxLayout(previewGroup);
    previewLay->setContentsMargins(4, 4, 4, 4);
    mTablePreview = new QTableWidget(previewGroup);
    mTablePreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTablePreview->verticalHeader()->setVisible(false);
    previewLay->addWidget(mTablePreview);
    rightLay->addWidget(previewGroup, 1);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtonOk                 = buttons->button(QDialogButtonBox::Ok);
    mButtonOk->setText(tr("OK"));                                      // cn:确定
    mButtonOk->setObjectName(QStringLiteral("buttonOk"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    mButtonOk->setEnabled(false);
    QHBoxLayout* bodyLay = new QHBoxLayout();
    bodyLay->addWidget(leftPanel, 2);
    bodyLay->addWidget(rightPanel, 3);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->addLayout(bodyLay, 1);
    mainLay->addWidget(buttons);
    // OK 走守卫确认点（melt 规模守卫在 accept 前拦截，§6.3-4）
    connect(mButtonOk, &QPushButton::clicked, this, &DADialogReshape::onOkClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(300);
    connect(mDebounceTimer, &QTimer::timeout, this, &DADialogReshape::runPreview);
    connect(mRadioTranspose, &QRadioButton::toggled, this, &DADialogReshape::onModeChanged);
    connect(mRadioMelt, &QRadioButton::toggled, this, &DADialogReshape::onModeChanged);
    connect(mListIdVars, &QListWidget::itemSelectionChanged, this, &DADialogReshape::onInputChanged);
    connect(mListValueVars, &QListWidget::itemSelectionChanged, this, &DADialogReshape::onInputChanged);
    connect(mEditVarName, &QLineEdit::textChanged, this, &DADialogReshape::onInputChanged);
    connect(mEditValueName, &QLineEdit::textChanged, this, &DADialogReshape::onInputChanged);

    resize(880, 520);
}

void DADialogReshape::onModeChanged()
{
    mMeltPanel->setVisible(mRadioMelt->isChecked());
    setWindowTitle(mRadioMelt->isChecked() ? tr("Unpivot / Melt")   // cn:宽转长（melt）
                                           : tr("Transpose"));      // cn:转置
    onInputChanged();
}

void DADialogReshape::onInputChanged()
{
    mDebounceTimer->start();
}

bool DADialogReshape::isMeltMode() const
{
    return mRadioMelt->isChecked();
}

bool DADialogReshape::isInplace() const
{
    return mCheckInplace->isChecked();
}

QStringList DADialogReshape::selectedNames(QListWidget* list) const
{
    QStringList names;
    for (int i = 0; i < list->count(); ++i) {
        if (list->item(i)->isSelected()) {
            names.append(list->item(i)->text());
        }
    }
    return names;
}

QStringList DADialogReshape::idVars() const
{
    return selectedNames(mListIdVars);
}

QStringList DADialogReshape::valueVars() const
{
    return selectedNames(mListValueVars);
}

QString DADialogReshape::varName() const
{
    const QString n = mEditVarName->text().trimmed();
    return n.isEmpty() ? QStringLiteral("variable") : n;
}

QString DADialogReshape::valueName() const
{
    const QString n = mEditValueName->text().trimmed();
    return n.isEmpty() ? QStringLiteral("value") : n;
}

/**
 * @brief 有效展开列数（valueVars 空 = 全部列 - 保留列数，pandas melt 语义）
 */
int DADialogReshape::effectiveValueVarCount() const
{
    if (mDf.isNone()) {
        return 0;
    }
    const int totalCols = static_cast< int >(mDf.shape().second);
    const QStringList vv = valueVars();
    if (!vv.isEmpty()) {
        return vv.size();
    }
    const QStringList idv = idVars();
    // 保留列中真实存在于表里的才扣减
    const QList< QString > cols = mDf.columns();
    int idCount                 = 0;
    for (const QString& n : idv) {
        if (cols.contains(n)) {
            ++idCount;
        }
    }
    return qMax(0, totalCols - idCount);
}

qint64 DADialogReshape::estimateMeltOutputRows(qint64 srcRows, int valueVarCount)
{
    if (srcRows <= 0 || valueVarCount <= 0) {
        return 0;
    }
    return srcRows * static_cast< qint64 >(valueVarCount);
}

qint64 DADialogReshape::estimatedOutputRows() const
{
    return estimateMeltOutputRows(mSrcRows, effectiveValueVarCount());
}

void DADialogReshape::setStatusError(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#CE6043;"));  // 橙红（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(false);
}

void DADialogReshape::setStatusOk(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#669E8B;"));  // 绿（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(true);
}

/**
 * @brief 预览（与执行同一算子：da_head_copy 样本 + da_transpose / da_melt）
 *
 * 转置样本必须限行（c_sampleRows）：全表转置在预览阶段就会产出「行数列」宽的表；
 * melt 预览同样基于样本，状态行给出**全表**预估输出行数（守卫依据）。
 */
void DADialogReshape::runPreview()
{
    mTablePreview->clear();
    mTablePreview->setRowCount(0);
    mTablePreview->setColumnCount(0);
    if (mDf.isNone()) {
        setStatusError(tr("No data"));  // cn:无数据
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QString err;
    DAPyDataFrame sample;
    DAPyDataFrame result;
    {
        DAWaitCursorScoped waitCursor;
        sample = py.headCopy(mDf, c_sampleRows);
        if (sample.isNone()) {
            setStatusError(tr("Failed to build preview sample"));  // cn:预览样本构建失败
            return;
        }
        if (isMeltMode()) {
            const QStringList idv = idVars();
            const QStringList vv  = valueVars();
            // 保留列与展开列不允许交叠（pandas 会报重复列，提前拦截给明确提示）
            for (const QString& n : vv) {
                if (idv.contains(n)) {
                    setStatusError(tr("Column \"%1\" is in both ID and value columns").arg(n));  // cn:列“%1”同时出现在保留列与展开列
                    return;
                }
            }
            if (effectiveValueVarCount() <= 0) {
                setStatusError(tr("No column left to unpivot (all columns are kept as ID)"));  // cn:没有可展开的列（全部列都被设为保留列）
                return;
            }
            if (varName().isEmpty() || valueName().isEmpty()) {
                setStatusError(tr("Variable/value column names must not be empty"));  // cn:变量名/值列名不能为空
                return;
            }
            result = py.melt(sample, idv, vv, varName(), valueName(), &err);
        } else {
            result = py.transpose(sample, &err);
        }
    }
    if (result.isNone()) {
        setStatusError(tr("Reshape failed: %1").arg(err));  // cn:形状变换失败：%1
        return;
    }
    // 预览表（结果前 c_previewRows 行、前 12 列，样本口径）
    const QStringList resCols = result.columns();
    const auto resShape       = result.shape();
    const int nrows           = qMin(static_cast< int >(resShape.first), c_previewRows);
    const int ncols           = qMin(static_cast< int >(resShape.second), 12);
    mTablePreview->setColumnCount(ncols);
    mTablePreview->setRowCount(nrows);
    mTablePreview->setHorizontalHeaderLabels(resCols.mid(0, ncols));
    for (int c = 0; c < ncols; ++c) {
        for (int r = 0; r < nrows; ++r) {
            mTablePreview->setItem(r, c, new QTableWidgetItem(result.iat(r, c).toString()));
        }
    }
    mTablePreview->resizeColumnsToContents();
    if (isMeltMode()) {
        const qint64 est = estimatedOutputRows();
        QString warn;
        if (est > c_meltRowLimit) {
            warn = tr(" — exceeds the %1-row guard, confirmation required on OK")
                       .arg(c_meltRowLimit);  // cn: — 超过 %1 行守卫，点确定时需确认
        }
        setStatusOk(tr("OK — estimated output rows: %1 (rows %2 x unpivoted columns %3)%4")
                        .arg(est)
                        .arg(mSrcRows)
                        .arg(effectiveValueVarCount())
                        .arg(warn));  // cn:通过 — 预估输出行数：%1（%2 行 × %3 展开列）%4
    } else {
        setStatusOk(tr("OK — result will have %1 row(s) (original columns) and one column per original row plus the \"index\" name column")
                        .arg(mDf.shape().second));  // cn:通过 — 结果将有 %1 行（原列数），列为原行索引 + "index" 命名列
    }
}

/**
 * @brief OK 点击：melt 规模守卫确认点（§6.3-4：超阈值默认拒绝，给预估行数，可强制继续）
 */
void DADialogReshape::onOkClicked()
{
    if (!mButtonOk->isEnabled()) {
        return;
    }
    if (isMeltMode()) {
        const qint64 est = estimatedOutputRows();
        if (est > c_meltRowLimit) {
            const auto answer = QMessageBox::question(this,
                                                      tr("Melt Scale Guard"),  // cn:melt 规模守卫
                                                      tr("The melt operation is estimated to produce %1 rows (limit %2), "
                                                         "which may consume a large amount of memory. Force continue?")
                                                          .arg(est)
                                                          .arg(c_meltRowLimit),  // cn:melt 预估产出 %1 行（上限 %2），可能占用大量内存。强制继续？
                                                      QMessageBox::Yes | QMessageBox::No,
                                                      QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                return;  // 默认拒绝
            }
        }
    }
    accept();
}

}  // end of namespace DA
