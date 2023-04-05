#ifndef DASETTINGCONTAINERWIDGET_H
#define DASETTINGCONTAINERWIDGET_H
#include <QTabWidget>
#include <QWidget>
#include <QList>
#include "DAGuiAPI.h"
namespace Ui
{
class DASettingContainerWidget;
}

namespace DA
{
class DAWorkFlowNodeItemSettingWidget;

/**
 * @brief 这是一个类似QStackedWidget的窗体，只内部有一个scallview
 */
class DAGUI_API DASettingContainerWidget : public QWidget
{
    Q_OBJECT

public:
    DASettingContainerWidget(QWidget* parent = nullptr);
    ~DASettingContainerWidget();
    //设置窗口
    int addWidget(QWidget* w);
    int count() const;
    int currentIndex() const;
    int indexOf(QWidget* widget) const;
    //提取窗口
    void removeWidget(QWidget* w);
    QWidget* widget(int index) const;
    int insertWidget(int index, QWidget* widget);
    QWidget* currentWidget() const;
public slots:
    void setCurrentWidget(QWidget* w);
    void setCurrentIndex(int index);
signals:
    void currentChanged(int index);
    void widgetRemoved(int index);

public:
    void initWorkFlowSettingWidgets();
    //特化
    DAWorkFlowNodeItemSettingWidget* getWorkFlowNodeItemSettingWidget();

private:
    Ui::DASettingContainerWidget* ui;
    DAWorkFlowNodeItemSettingWidget* _workFlowNodeItemSettingWidget;
    QList< QWidget* > _widgets;
    int _currentIndex;
};
}  // namespace DA
#endif  // DASETTINGCONTAINERWIDGET_H
