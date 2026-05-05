#pragma once

#include <QObject>

class QStackedWidget;

namespace DA
{

class DANodeParamSettingPanelWidget;
class DANodeParamSettingPanel;

/**
 * @brief DANodeParamSettingPanelWidget 控件的单元测试
 *
 * 验证调度器的 QStackedWidget 布局、惰性缓存逻辑、
 * null 代理处理、面板复用和缓存清除等功能。
 */
class TestDANodeParamSettingPanelWidget : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    // 控件创建和布局验证
    void test_createWidget_hasStackedWidget();
    void test_createWidget_hasPlaceholder();

    // null 代理处理
    void test_nullProxy_showsPlaceholder();

    // 面板创建和缓存
    void test_setNodeProxy_createsPanel();
    void test_cacheReuse_sameQualifiedName();
    void test_clearCache_removesAllPanels();

    // 当前面板查询
    void test_currentPanel_returnsCurrent();
    void test_currentPanel_nullWhenNoProxy();

private:
    DA::DANodeParamSettingPanelWidget* mWidget = nullptr;
};

}  // namespace DA