#ifndef DAABSTRACTSETTINGWIDGET_H
#define DAABSTRACTSETTINGWIDGET_H
#include <QWidget>
#include "DACommonWidgetsAPI.h"
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
class DACOMMONWIDGETS_API DAAbstractSettingPage : public QWidget
{
	Q_OBJECT
public:
	DAAbstractSettingPage(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~DAAbstractSettingPage();
	/**
	 * @brief 应用设置，用户点击设置窗口的应用按钮或者确定按钮，会触发apply接口，在此接口上，可以用于保存此配置页想要保存的信息
	 *
	 * @note 注意，页面ui的设置变更时要通过@ref settingChanged 信号通知设置窗口，顶层设置窗口会把这个配置页标记为dirty
	 * 如果没有发射@ref settingChanged 信号,顶层的设置管理窗口在用户点确定或应用时不会调用此窗口的apply接口
	 */
	virtual void apply() = 0;
	/**
	 * @brief 设置页的标题，此函数影响DASettingWidget的listwidget的显示
	 * @return
	 */
	virtual QString getSettingPageTitle() const = 0;
	/**
	 * @brief 设置页的图标,此函数影响DASettingWidget的listwidget的显示
	 * @return
	 */
	virtual QIcon getSettingPageIcon() const = 0;

	/**
	 * @brief 获取配置文件推荐保存的目录
	 *
	 * 此函数会返回一个路径，这个路径正常情况必定存在，如果无法创建将会报错（qCritual）
	 *
	 * 此函数返回的是DADir::getConfigPath
	 * @return 如果创建不成功会返回一个空的字符串
	 */
	static QString getConfigFileSavePath();
signals:
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
