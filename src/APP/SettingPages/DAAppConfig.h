#ifndef DACOMMONCONFIG_H
#define DACOMMONCONFIG_H
#include "DAProperties.h"
#include "DAXMLFileInterface.h"
#include "DAAppCore.h"
/**
 *@def ribbon的样式
 */
#define DA_CONFIG_KEY_RIBBON_STYLE "ribbon-style"
/**
 *@def 显示日志的条数
 */
#define DA_CONFIG_KEY_SHOW_LOG_NUM "show-log-num"
namespace DA
{
class AppMainWindow;
/**
 * @brief 此类为本程序的设置类
 */
class DAAppConfig : public DAProperties, public DAXMLFileInterface
{
public:
    DAAppConfig();
    virtual ~DAAppConfig();
    void setCore(DAAppCore* core);
    //加载配置
    bool loadConfig(bool noFileCreateNewOne = true);
    //保存配置
    bool saveConfig();
    //保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const;
    virtual bool loadFromXml(const QDomElement* parentElement);
    //获取配置文件名字
    static QString getConfigFileName();
    //获取配置文件的绝对路径
    static QString getAbsoluteConfigFilePath();

public:
    virtual bool apply();

private:
private:
    const QString cConfigName { "da-app" };
    QString mConfigFilePath;  ///< 配置文件路径
    DAAppCore* mCore { nullptr };
    DAAppUI* mUI { nullptr };
    AppMainWindow* mMainWindow { nullptr };
};
}

#endif  // DACOMMONCONFIG_H
