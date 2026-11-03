#ifndef DAABSTRACTSETTINGWIDGET_H
#define DAABSTRACTSETTINGWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
namespace DA
{

/**
 * @brief 配置页面的基类，所有配置页面都继承此类
 *
 * DASettingWidget管理所有的DAAbstractSettingPage
 *
 * @ref settingChanged 信号用于通知主窗口设置发生了变换，顶层设置窗口会把这个配置页标记为dirty如果没有发射settingChanged信号,
 * 顶层的设置管理窗口在用户点确定或应用时不会调用此窗口的apply接口
 *
 * @ref settingApplyed 信号用于通知主窗口设置已经完成，顶层设置窗口会把这个配置页标记为clean,顶层的设置管理窗口在用户点确定或应用时不会调用此窗口的apply接口
 *
 * @note 重载时务必调用DAAbstractSettingPage::setConfig(c);否则getConfig函数将不起作用
 *
 */
class DAGUI_API DAAbstractSettingPage : public QWidget
{
	Q_OBJECT
public:
	DAAbstractSettingPage(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~DAAbstractSettingPage();
	// 应用设置
	virtual void apply() = 0;
	// 设置页的标题
	virtual QString getSettingPageTitle() const = 0;
	// 设置页的图标
	virtual QIcon getSettingPageIcon() const = 0;
	// 获取配置文件推荐保存的目录
	static QString getConfigFileSavePath();
Q_SIGNALS:
	/**
	 * @brief 配置信息改变信号
	 *
	 * 此信号只要配置页面有任何的改变都应该发出通知到配置窗口
	 *
	 * @note 参数改变后一定要发射此信号，否则不会被感知到设置的改变
	 */
	void settingChanged();
	/**
	 * @brief 设置应用完成
	 */
	void settingApplyed();
};
}

#endif  // DAABSTRACTSETTINGWIDGET_H
