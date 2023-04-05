#ifndef DAABSTRACTSETTINGWIDGET_H
#define DAABSTRACTSETTINGWIDGET_H

#include <QWidget>
#include "DACommonWidgetsAPI.h"
namespace DA
{

/**
 * @brief 配置页面的基类，所有配置页面都继承此类
 */
class DACOMMONWIDGETS_API DAAbstractSettingPage : public QWidget
{
    Q_OBJECT
public:
    DAAbstractSettingPage(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~DAAbstractSettingPage();
    //应用设置
    virtual void apply() = 0;
signals:
    /**
     * @brief 配置信息改变信号
     *
     * 此信号只要配置页面有任何的改变都应该发出通知到配置窗口
     */
    void settingChanged();
    /**
     * @brief 设置应用完成
     */
    void settingApplyed();
};
}

#endif  // DAABSTRACTSETTINGWIDGET_H
