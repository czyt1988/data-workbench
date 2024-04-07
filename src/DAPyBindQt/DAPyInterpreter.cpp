#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

namespace DA
{

class DAPyInterpreter::PrivateData
{
	DA_DECLARE_PUBLIC(DAPyInterpreter)
public:
	PrivateData(DAPyInterpreter* p);
public:
    std::unique_ptr<pybind11::scoped_interpreter> interpreter;
};

DAPyInterpreter::PrivateData::PrivateData(DAPyInterpreter* p) : q_ptr(p)
{
}
//===================================================
// DAPyInterpreter
//===================================================
DAPyInterpreter::DAPyInterpreter():DA_PIMPL_CONSTRUCT
{
}

DAPyInterpreter::~DAPyInterpreter()
{
}

DAPyInterpreter& DAPyInterpreter::getInstance()
{
	static DAPyInterpreter s_python;
	return s_python;
}

/**
 * @brief 获取系统记录的python环境
 * @return
 */
QList< QFileInfo > DAPyInterpreter::wherePython()
{
	QList< QFileInfo > validFis;
	QProcess process;
	QString command = "where python";
	process.start(command);
	if (!process.waitForFinished()) {
		return QList< QFileInfo >();
	}
	QString res = process.readAll();
	qDebug() << res;
	const QList< QString > pys = res.split("\r\n");
	// 遍历所有环境，确认是否的确是ptython路径,where 有时候会返回一些不正确的路径
	for (QString p : pys) {
		QFileInfo fi(p);
		if (fi.isExecutable()) {
			// 说明是可执行文件，windows下就是pythhon.exe
			validFis.append(fi);
		}
	}

	return validFis;
}

QList< QFileInfo > DA::DAPyInterpreter::wherePythonFromConfig()
{
	QList< QFileInfo > validFis;
	//! 先查看根目录下python-config.json
	QString cfgPath = getAppPythonConfigFile();
	QFile file(cfgPath);
	if (!file.exists()) {
		return validFis;
	}
	if (!file.open(QIODevice::ReadOnly)) {
		return validFis;
	}
	QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
	if (jsonDoc.isNull() || jsonDoc.isEmpty()) {
		return validFis;
	}
	if (!jsonDoc.isObject()) {
		return validFis;
	}
	QJsonObject root = jsonDoc.object();
	if (!root.contains("config")) {
		return validFis;
	}
	QJsonObject config  = root[ "config" ].toObject();
	QString interpreter = config[ "interpreter" ].toString();
	QFileInfo fi(interpreter);
	if (!fi.exists()) {
		return validFis;
	}
	validFis.append(fi);
	return validFis;
}

/**
 * @brief 设置python的运行路径
 *
 * @param path
 */
void DAPyInterpreter::setPythonHomePath(const QString& path)
{
	if (path.isEmpty()) {
		return;
	}
	std::vector< wchar_t > wp((path.size() + 1) * 4, 0);

	path.toWCharArray(wp.data());
	try {
		Py_SetPythonHome(wp.data());
	} catch (const std::exception& e) {
		qCritical() << e.what();
	}
}

void DAPyInterpreter::initializePythonInterpreter()
{
	try {
        d_ptr->interpreter = std::make_unique<pybind11::scoped_interpreter>();
	} catch (const std::exception& e) {
		qWarning() << e.what();
	}
}

void DAPyInterpreter::registerFinalizeCallback(DAPyInterpreter::callback_finalize fp)
{
	mFinalizeCallbacks.push_back(fp);
}

/**
   @brief 获取python配置文件

   @return $RETURN
 */
QString DA::DAPyInterpreter::getAppPythonConfigFile()
{
	QString appDir = QCoreApplication::applicationDirPath();
	return QDir::toNativeSeparators(appDir + "/python-config.json");
}

/**
   @brief 获取python的路径

   @param $PARAMS
   @return $RETURN
 */
QString DA::DAPyInterpreter::getPythonInterpreterPath()
{
	QList< QFileInfo > validFis;
	//! 先查看根目录下python-config.json
	validFis = wherePythonFromConfig();
	if (!validFis.empty()) {
		return validFis.back().absoluteFilePath();
	}
	//! 如果没有，就看看wherepython
	validFis = wherePython();
	if (!validFis.empty()) {
		return validFis.back().absoluteFilePath();
	}
	return QString();
}

}