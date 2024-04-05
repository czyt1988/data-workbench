#include "DATxtFileImportDialog.h"
#include "ui_DATxtFileImportDialog.h"
#include <QTextCodec>
#include <QThread>
#include <QMessageBox>
#include <QFileInfo>
#include "DATextReadWriter.h"
//
#include "DADataManager.h"

#if DA_ENABLE_PYTHON
#include "pandas/DAPyDataFrame.h"
#include "DAPyScriptsIO.h"
#include "Models/DAPyDataFrameTableModule.h"
#endif
namespace DA
{
DATxtFileImportDialog::DATxtFileImportDialog(QWidget* parent) : QDialog(parent), ui(new Ui::DATxtFileImportDialog)
{
    ui->setupUi(this);
#if DA_ENABLE_PYTHON
    // 第一个参数为nullptr，说明不用redo/undo
    mModule = new DAPyDataFrameTableModule(nullptr, ui->tableView);
    ui->tableView->setModel(mModule);
#endif
    ui->comboBoxCodec->setCurrentText(QString(QTextCodec::codecForLocale()->name()));
    // 存在qt6不兼容性
    QList< QByteArray > codecs = QTextCodec::availableCodecs();
    for (int i = 0; i < codecs.size(); ++i) {
        ui->comboBoxCodec->addItem(QString(codecs[ i ]), codecs[ i ]);
    }
    // 分隔符
    ui->comboBoxDelimiter->addItem(tr(",(comma)"), QVariant(","));        // cn:,逗号
    ui->comboBoxDelimiter->addItem(tr(" (space)"), QVariant(" "));        // cn: 空格
    ui->comboBoxDelimiter->addItem(tr("\\t(tab stop)"), QVariant("\t"));  // cn:tab制表位
    ui->comboBoxDelimiter->addItem(tr(";(semicolon)"), QVariant(";"));    // cn:;分号
    ui->comboBoxDelimiter->addItem(tr("_(underscore)"), QVariant("_"));   // cn:_下横杠
    ui->comboBoxDelimiter->addItem(tr("-(dash)"), QVariant("-"));         // cn:-横杠
    connect(ui->filePathEditWidget, &DAFilePathEditWidget::selectedPath, this, &DATxtFileImportDialog::onFilePathEditSelectedPath);
    connect(ui->pushButtonRefresh, &QPushButton::clicked, this, &DATxtFileImportDialog::refresh);
    connect(this, &QDialog::accepted, this, &DATxtFileImportDialog::onAccept);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(ui->spinBoxSkipFooter,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DATxtFileImportDialog::onSpinBoxSkipFooterValueChanged);
#else
    connect(ui->spinBoxSkipFooter, &QSpinBox::valueChanged, this, &DATxtFileImportDialog::onSpinBoxSkipFooterValueChanged);
#endif
    ui->pushButtonRefresh->setFocus();
}

DATxtFileImportDialog::~DATxtFileImportDialog()
{
    delete ui;
}

QString DATxtFileImportDialog::getTextFilePath() const
{
    return ui->filePathEditWidget->getFilePath();
}

void DATxtFileImportDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DATxtFileImportDialog::readTextFile(const QString& p)
{
    QThread* readThread         = new QThread();
    DATextReadWriter* txtReader = new DATextReadWriter();
    txtReader->moveToThread(readThread);
    txtReader->setReadOnceCharCount(-1);
    txtReader->setReadOnceLineCount(-1);
    txtReader->setTotalReadLineCount(ui->spinBoxPreviewMaxRow->value());
    txtReader->setTotalReadCharCount(100000);
    txtReader->setSkipHeader(static_cast< std::size_t >(ui->spinBoxSkipHeader->value()));
    txtReader->setFileName(p);
    txtReader->setCodec(ui->comboBoxCodec->currentText());
    connect(readThread, &QThread::finished, readThread, &QThread::deleteLater);
    connect(readThread, &QThread::finished, txtReader, &DATextReadWriter::deleteLater);
    connect(readThread, &QThread::started, txtReader, &DATextReadWriter::startReadText);

    connect(txtReader, &DATextReadWriter::readComplete, this, &DATxtFileImportDialog::onTextReadComplete);
    connect(txtReader, &DATextReadWriter::finished, readThread, &QThread::quit);  // 必要，否则不会触发finished而清除内存
    connect(txtReader, &DATextReadWriter::finished, this, &DATxtFileImportDialog::onTextReadFinished);
    readThread->start();
}

/**
 * @brief 从界面获取设置内容
 * @return
 */
QVariantMap DATxtFileImportDialog::getSettingFromUI()
{
    QVariantMap r;
    if (!ui->checkBoxDelimiter->isChecked()) {
        // 不是自动判断，需要delimiter参数
        auto d = ui->comboBoxDelimiter->currentData();
        QString delimiter;
        if (d.isNull()) {
            delimiter = ui->comboBoxDelimiter->currentText();
        } else {
            delimiter = d.toString();
        }
        r[ "delimiter" ] = delimiter;
    }
    auto skip_header = ui->spinBoxSkipHeader->value();
    if (skip_header > 0) {
        r[ "skip_header" ] = skip_header;
    }
    auto skip_footer = ui->spinBoxSkipFooter->value();
    if (skip_footer > 0) {
        r[ "skip_footer" ] = skip_footer;
    }
    if (ui->checkBoxMaxRows->isChecked()) {
        r[ "max_rows" ] = ui->spinBoxMaxRows->value();
    }
    r[ "encoding" ] = ui->comboBoxCodec->currentText();
    if (ui->checkBoxNames->isChecked()) {
        r[ "names" ] = true;
    }
    return r;
}

void DATxtFileImportDialog::refresh()
{
    QString path = getTextFilePath();
    // 打开文档
    ui->plainTextEdit->clear();
    //
    readTextFile(path);
    //
    QVariantMap args = getSettingFromUI();
    // 预览状态下，预览的最大行数单独再设置
    args[ "max_rows" ] = ui->spinBoxPreviewMaxRow->value();
#if DA_ENABLE_PYTHON
    DAPyScriptsIO io;
    DAPyDataFrame df = io.read_txt(path, args);
    mModule->setDataFrame(df);
#endif
}

void DATxtFileImportDialog::onFilePathEditSelectedPath(const QString& p)
{
    refresh();
}

void DATxtFileImportDialog::onTextReadComplete(const QString& txt, bool isReadAll)
{
    ui->plainTextEdit->appendPlainText(txt);
}

void DATxtFileImportDialog::onTextReadFinished(int code)
{
    if (code != DATextReadWriter::NoError) {
        QMessageBox::critical(this,
                              tr("error"),
                              tr("read txt file(%1) occure error,reason:%3")
                                  .arg(getTextFilePath())
                                  .arg(DATextReadWriter::errorCodeToString(static_cast< DATextReadWriter::ErrorCode >(code))));
        return;
    }
}

void DATxtFileImportDialog::onSpinBoxSkipFooterValueChanged(int v)
{
}

void DATxtFileImportDialog::onAccept()
{
    QString path = getTextFilePath();
    // 判断文本的大小，确定是否开启线程
    QFileInfo fi(path);
    if (!fi.exists()) {
        QMessageBox::warning(this, tr("warning"), tr("file %1 not exists").arg(path));
        return;
    }
    QVariantMap args = getSettingFromUI();
    int maxRow       = 0;
    if (args.contains("max_rows")) {
        maxRow = args[ "max_rows" ].toInt();
    }
    float fileSizeMB = fi.size() / 1024.0 / 1024.0;
    if (fileSizeMB > 50 || maxRow == 0 || maxRow > 1e6) {
        QMessageBox::
            warning(this,
                    tr("warning"),
                    tr("file %1 size is %2 MB,is exceeds 50MB and data loading will be performed in the background")  // cn:文件%1的尺寸为%2MB，超过了50MB，将在后台进行数据加载
                        .arg(path)
                        .arg(fileSizeMB));
        // 后台加载
    } else {
        // 前台加载
    }
}

}  // end DA
