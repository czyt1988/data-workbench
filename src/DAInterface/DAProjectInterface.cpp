#include "DAProjectInterface.h"
#include <QFileInfo>
#include <QSysInfo>
#include "DAWorkFlowOperateWidget.h"
#include "DAStringUtil.h"
#include "DAXmlHelper.h"
#include "DAQtContainerUtil.hpp"
#include "DADataManagerInterface.h"
namespace DA
{

//===================================================
// DAProjectInterfacePrivate
//===================================================
class DAProjectInterface::PrivateData
{
	DA_DECLARE_PUBLIC(DAProjectInterface)
public:
	PrivateData(DAProjectInterface* p);
	// 存在路径
	bool isHaveProjectFilePath() const;

public:
	bool mIsDirty { false };  ///< 脏标识
	DAWorkFlowOperateWidget* mWorkFlowOperateWidget { nullptr };
	DADataManagerInterface* mDataManagerInterface { nullptr };
    bool mIsBusy { false };
	QFileInfo mProjectFileInfo;  ///< 记录工程文件信息

	static QString s_suffix;  ///< 工程文件后缀
};

/**
 * @brief 工程文件默认后缀
 */
QString DAProjectInterface::PrivateData::s_suffix = QString("asproj");

DAProjectInterface::PrivateData::PrivateData(DAProjectInterface* p) : q_ptr(p)
{
}

bool DAProjectInterface::PrivateData::isHaveProjectFilePath() const
{
    return mProjectFileInfo.isFile();
}

//===================================================
// DAProjectInterface
//===================================================
DAProjectInterface::DAProjectInterface(DACoreInterface* c, QObject* par) : DABaseInterface(c, par), DA_PIMPL_CONSTRUCT
{
}

DAProjectInterface::~DAProjectInterface()
{
}

/**
 * @brief 设置工作流操作窗口
 * @param w
 */
void DAProjectInterface::setWorkFlowOperateWidget(DAWorkFlowOperateWidget* w)
{
    d_ptr->mWorkFlowOperateWidget = w;
}
/**
 * @brief 获取工作流操作窗口
 * @param w
 */
DAWorkFlowOperateWidget* DAProjectInterface::getWorkFlowOperateWidget() const
{
    return d_ptr->mWorkFlowOperateWidget;
}

/**
 * @brief 设置数据管理接口
 * @param d
 */
void DAProjectInterface::setDataManagerInterface(DADataManagerInterface* d)
{
    d_ptr->mDataManagerInterface = d;
}

/**
 * @brief 获取数据管理接口
 * @return
 */
DADataManagerInterface* DAProjectInterface::getDataManagerInterface()
{
    return d_ptr->mDataManagerInterface;
}

/**
 * @brief 获取工程名
 *
 * 返回工程的文件名(不含后缀)
 * @return 如果没有设置工程，将返回空字符串
 */
QString DAProjectInterface::getProjectBaseName() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return (d_ptr->mProjectFileInfo.baseName());
}

/**
 * @brief 获取工程路径
 *
 * @sa setProjectPath
 * @return 如果没有设置工程，将返回空字符串
 */
QString DAProjectInterface::getProjectDir() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return d_ptr->mProjectFileInfo.absolutePath();
}

/**
 * @brief DAProjectInterface::getProjectFilePath
 * @note 注意这个工程路径是工程文件的路径，并不是工作区的路径，但设置工程路径会把工作区设置到当前目录下
 * @return
 */
QString DAProjectInterface::getProjectFilePath() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return d_ptr->mProjectFileInfo.absoluteFilePath();
}

/**
 * @brief 设置工程路径
 * @param projectPath
 * @note 注意这个工程路径是工程文件的路径，并不是工作区的路径，但设置工程路径会把工作区设置到当前目录下
 */
void DAProjectInterface::setProjectPath(const QString& projectPath)
{
    d_ptr->mProjectFileInfo.setFile(projectPath);
}

/**
 * @brief 获取工作区
 * @note 工程文件所在目录定义为工作区
 * @return 如果没有设置工程，将返回空字符串
 */
QString DAProjectInterface::getWorkingDirectory() const
{
	if (!d_ptr->isHaveProjectFilePath()) {
		return QString();
	}
	return d_ptr->mProjectFileInfo.absolutePath();
}
/**
 * @brief 工程是否脏
 * @return
 */
bool DAProjectInterface::isDirty() const
{
    return d_ptr->mIsDirty;
}

/**
 * @brief 清空工程
 */
void DAProjectInterface::clear()
{
	setModified(false);
	d_ptr->mProjectFileInfo = QFileInfo();
	emit projectIsCleaned();
}

/**
 * @brief 工程文件的版本,版本组成有大版本.小版本.小小版本组成，例如1.0.0
 * @return
 */
QVersionNumber DAProjectInterface::getProjectVersion()
{
	static QVersionNumber s_version = QVersionNumber(1, 3, 0);
    return s_version;
}

/**
 * @brief 是否繁忙，正在保存文件过程中会为繁忙状态
 * @return
 */
bool DAProjectInterface::isBusy() const
{
    return d_ptr->mIsBusy;
}

/**
 * @brief 获取工程文件的后缀
 * @return
 */
QString DAProjectInterface::getProjectFileSuffix()
{
    return DAProjectInterface::PrivateData::s_suffix;
}

/**
 * @brief 设置工程文件的后缀
 * @param f
 * @return
 */
void DAProjectInterface::setProjectFileSuffix(const QString& f)
{
    DAProjectInterface::PrivateData::s_suffix = f;
}

/**
 * @brief 加载工程的基础实现，这个函数不会发射projectLoaded和projectBeginLoad信号，需要在具体实现中执行
 *
 * 这个函数会把加载的路径记录下来，因此，重写此函数也需要调用DAProjectInterface::load
 * @code
 * bool DAAppProject::load(const QString& path)
 * {
 *     DAProjectInterface::load(path);
 *     ...
 *     return true;
 * }
 * @endcode
 * @param path
 * @return
 */
bool DAProjectInterface::load(const QString& path)
{
    d_ptr->mProjectFileInfo.setFile(path);
    return true;
}

/**
 * @brief 保存工程的基础实现，这个函数不会发射projectSaved和projectBeginSave信号，需要在具体实现中执行
 *
 * 这个函数会把保存的路径记录下来，因此，重写此函数也需要调用DAProjectInterface::save
 * @code
 * bool DAAppProject::save(const QString& path)
 * {
 *     DAProjectInterface::save(path);
 *     ...
 *     Q_EMIT projectBeginSave(path);//启动线程
 *     return true;
 * }
 * @endcode
 * @param path
 * @return
 */
bool DAProjectInterface::save(const QString& path)
{
    d_ptr->mProjectFileInfo.setFile(path);
    return true;
}

/**
 * @brief 设置为dirty
 * @param on
 */
void DAProjectInterface::setModified(bool on)
{
	if (on != d_ptr->mIsDirty) {
		d_ptr->mIsDirty = on;
		emit dirtyStateChanged(on);
    }
}

void DAProjectInterface::setBusy(bool on)
{
    d_ptr->mIsBusy = on;
}

}
