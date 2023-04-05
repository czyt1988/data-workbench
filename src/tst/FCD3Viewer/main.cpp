#include <QApplication>
#include "MainWindow.h"


int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindowTest win;
	win.show();

	return app.exec();


}
