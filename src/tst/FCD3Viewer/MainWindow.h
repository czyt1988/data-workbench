#pragma once

#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class FCD3Viewer;

class MainWindowTest : public QMainWindow
{
    Q_OBJECT
public:
	MainWindowTest();
	~MainWindowTest();



private slots:
	void on_pushButtonAdd_clicked();
	void on_pushButtonFit_clicked();
    void on_pushButtonRemove_clicked();

    void on_pushButtonView_clicked();

    void on_checkBoxAutoUpdate_stateChanged(int arg1);

    void on_pushButtonImport_clicked();

private:
    Ui::MainWindow *_ui{};
    FCD3Viewer *_viewer;
};
