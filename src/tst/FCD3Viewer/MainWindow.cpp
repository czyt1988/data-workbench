#include "MainWindow.h"
#include <ui_MainWindow.h>
#include "FCD3Viewer.h"
#include <QFileDialog>
#include <QDebug>

MainWindowTest::MainWindowTest()
{
	_ui = new Ui::MainWindow;
	_ui->setupUi(this);
    _viewer = new FCD3Viewer(this);
    _ui->graphLayout->addWidget(_viewer);
    //
    _ui->comboBoxView->addItem(QStringLiteral("无"), FCD3Viewer::UnknowView);
    _ui->comboBoxView->addItem(QStringLiteral("ViewXPlus"), FCD3Viewer::ViewXPlus);
    _ui->comboBoxView->addItem(QStringLiteral("ViewXMiuns"), FCD3Viewer::ViewXMiuns);
    _ui->comboBoxView->addItem(QStringLiteral("ViewYPlus"), FCD3Viewer::ViewYPlus);
    _ui->comboBoxView->addItem(QStringLiteral("ViewYMiuns"), FCD3Viewer::ViewYMiuns);
    _ui->comboBoxView->addItem(QStringLiteral("ViewZPlus"), FCD3Viewer::ViewZPlus);
    _ui->comboBoxView->addItem(QStringLiteral("ViewZMiuns"), FCD3Viewer::ViewZMiuns);
    _ui->comboBoxView->addItem(QStringLiteral("ViewFit"), FCD3Viewer::ViewFit);
}


MainWindowTest::~MainWindowTest()
{
}


void MainWindowTest::on_pushButtonAdd_clicked()
{
    double loc[3]{ 0, 0, 0 };

	loc[0] = _ui->locationX->value();
	loc[1] = _ui->locationY->value();
	loc[2] = _ui->locationZ->value();

    double para[3]{ 0, 0, 0 };

	para[0] = _ui->length->value();
	para[1] = _ui->width->value();
	para[2] = _ui->high->value();

	_viewer->addBox(loc, para);
}


void MainWindowTest::on_pushButtonFit_clicked()
{
	_viewer->fitView();
}


void MainWindowTest::on_pushButtonRemove_clicked()
{
    qDebug()<<"Remove viewer";
    _ui->graphLayout->removeWidget(_viewer);
}


void MainWindowTest::on_pushButtonView_clicked()
{
    FCD3Viewer::ViewType type = static_cast<FCD3Viewer::ViewType>(_ui->comboBoxView->currentData().toInt());

    _viewer->view(type);
}


void MainWindowTest::on_checkBoxAutoUpdate_stateChanged(int arg1)
{
    _viewer->setAutoRender(arg1 == Qt::Checked);
}


void MainWindowTest::on_pushButtonImport_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("打开文件")
        , QString(), QStringLiteral("igs文件(*.igs;*.iges)"));

    if (file.isEmpty()) {
        return;
    }
    _viewer->importGeometry(file);
}
