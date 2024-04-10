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

void DATxtFileImportDialog::setTextFilePath(const QString& p)
{
	ui->filePathEditWidget->setFilePath(p);
	refresh();
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
QVariantMap DATxtFileImportDialog::getSetting()
{
	QVariantMap r;
	if (ui->checkBoxDelimiter->isChecked()) {
		r[ "sep" ] = "\\s+";
	} else {
		// 不是自动判断，需要delimiter参数
		auto d = ui->comboBoxDelimiter->currentData();
		QString delimiter;
		if (d.isNull()) {
			delimiter = ui->comboBoxDelimiter->currentText();
		} else {
			delimiter = d.toString();
		}
		r[ "sep" ] = delimiter;
	}
	if (ui->checkBoxHeaderRow->isChecked()) {
		r[ "header" ] = ui->spinBoxHeaderRow->value() - 1;
	} else {
		r[ "header" ] = QVariant();  // 没有表头为none
	}
	r[ "skiprows" ]   = ui->spinBoxSkipHeader->value();
	r[ "skipfooter" ] = ui->spinBoxSkipFooter->value();
	if (ui->checkBoxMaxRows->isChecked()) {
		r[ "nrows" ] = ui->spinBoxMaxRows->value();
	}
	r[ "encoding" ]         = ui->comboBoxCodec->currentText();
	r[ "skip_blank_lines" ] = ui->checkBoxSkipBlankLines->isChecked();
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
	QVariantMap args = getSetting();
	// 预览状态下，预览的最大行数单独再设置
	args[ "nrows" ] = ui->spinBoxPreviewMaxRow->value();
#if DA_ENABLE_PYTHON
	DAPyScriptsIO io;
	QString err;
	DAPyDataFrame df = io.read_txt(path, args, &err);
	if (err.isEmpty()) {
		ui->labelError->setText("");
		ui->tabWidget->setCurrentIndex(1);
	} else {
		ui->labelError->setText(err);
	}
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

}  // end DA
