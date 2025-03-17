﻿#ifndef DACOREINTERFACE_H
#define DACOREINTERFACE_H
#include "DAInterfaceAPI.h"
#include <QObject>
#include <QDir>
#include <QTemporaryDir>

class SARibbonMainWindow;

namespace DA
{
class DAUIInterface;
class DADataManagerInterface;
class DAProjectInterface;
/**
 * @brief APP的核心接口
 *
 * 所有接口都可以通过此接口获取，这个接口是最关键的接口
 */
class DAINTERFACE_API DACoreInterface : public QObject
{
	Q_OBJECT
public:
	DACoreInterface(QObject* parent = nullptr);
	virtual ~DACoreInterface();

	// 初始化函数，初始化函数里构造出DAAppUIInterface，DADataManagerInterface等实例
	virtual bool initialized() = 0;

	// 获取DAAppRibbonAreaInterface
	virtual DAUIInterface* getUiInterface() const = 0;
	// 获取工程管理借口
	virtual DAProjectInterface* getProjectInterface() const = 0;
	// 获取数据管理接口
	virtual DADataManagerInterface* getDataManagerInterface() const = 0;

public:
	/**
	 *  @group 以下为快捷功能的封装
	 *  {
	 */

	// 工程是否dirty的操作
	bool isProjectDirty() const;
	void setProjectDirty(bool on);

	// 工程的临时目录
	QDir getTempDir() const;
	/**
	 * }
	 */
public:
	// 调用此函数，创建DAAppRibbonAreaInterface，此函数的调用应该发生在SARibbonMainWindow的构造过程
	virtual void createUi(SARibbonMainWindow* mainwindow) = 0;

private:
	QTemporaryDir mTempDir;
};
}  // namespace DA

#endif  // DACOREINTERFACE_H
