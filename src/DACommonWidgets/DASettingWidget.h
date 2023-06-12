#ifndef DASETTINGWIDGET_H
#define DASETTINGWIDGET_H

#include <QWidget>
#include <QSet>
#include "DACommonWidgetsAPI.h"
namespace Ui
{
class DASettingWidget;
}

namespace DA
{
class DAAbstractSettingPage;

/**
 * @brief 一个基于列表索引的设置页面
 */
class DACOMMONWIDGETS_API DASettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DASettingWidget(QWidget* parent = nullptr);
    ~DASettingWidget();
    //获取页面的数量
    int count() const;
    //添加一个设置页面
    void addPage(DAAbstractSettingPage* page);
    //获取改变的页面
    QList< DAAbstractSettingPage* > getChanggedPages() const;
public slots:
    //设置页面
    void setPage(int index);
    //应用所有的改变
    void applyAll();
    //应用改变
    void applyChanged();
private slots:
    //页面配置改变
    void onPageSettingChanged();
signals:
    /**
     * @brief 配置信息改变信号
     *
     * 此信号只要配置页面有任何的改变都应该发出通知到配置窗口
     */
    void settingChanged();
    /**
     * @brief 设置应用的信号
     */
    void settingApplyed();

private:
    Ui::DASettingWidget* ui;
    QSet< DAAbstractSettingPage* > mChangedPages;  ///< 记录改变了的页面
};
}
#endif  // DASETTINGWIDGET_H
