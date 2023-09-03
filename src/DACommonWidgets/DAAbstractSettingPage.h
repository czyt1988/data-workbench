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
 * 数据的交互通过DAConfig类
 *
 * 首先程序会加载DAConfig对象，在构建页面的时候，会调用setConfig把配置传入
 *
 * 在调用apply函数的时候需要应用设置，这时候也要同步更改DAConfig内容，DASettingWidget会调用getConfig把配置信息重新获取
 *
 * 同时会把对应的配置信息重新保存
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
     * @brief 应用设置
     * @note 注意要更新返回的config
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
     * 此函数返回的是QStandardPaths::AppConfigLocation
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
