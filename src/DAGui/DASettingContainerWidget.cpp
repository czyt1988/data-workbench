#include "DASettingContainerWidget.h"
#include "ui_DASettingContainerWidget.h"
#include "DAWorkFlowNodeItemSettingWidget.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DASettingContainerWidget
//===================================================
DASettingContainerWidget::DASettingContainerWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DASettingContainerWidget), _currentIndex(-1)
{
    ui->setupUi(this);
    initWorkFlowSettingWidgets();
}

DASettingContainerWidget::~DASettingContainerWidget()
{
    ui->scrollArea->takeWidget();
    for (QWidget* w : qAsConst(_widgets)) {
        delete w;
    }
    delete ui;
}

/**
 * @brief 添加配置窗口
 *
 * @note 窗口原来的父类会改变
 * @note 窗口的所有权归@sa DASettingContainerWidget 管理
 * @param w
 */
int DASettingContainerWidget::addWidget(QWidget* w)
{
    if (_widgets.contains(w)) {
        return _widgets.indexOf(w);
    }
    _widgets.append(w);
    setCurrentWidget(w);
    return _widgets.size() - 1;
}
/**
 * @brief same as QStackedWidget::count
 * @return
 */
int DASettingContainerWidget::count() const
{
    return _widgets.size();
}

int DASettingContainerWidget::currentIndex() const
{
    return _currentIndex;
}

int DASettingContainerWidget::indexOf(QWidget* w) const
{
    return _widgets.indexOf(w);
}

/**
 * @brief widget is not deleted but simply removed from the stacked layout, causing it to be hidden
 * @param w
 */
void DASettingContainerWidget::removeWidget(QWidget* w)
{
    int i = indexOf(w);
    if (i < 0) {
        return;
    }
    if (ui->scrollArea->widget() == w) {
        ui->scrollArea->takeWidget();
    }
    _widgets.removeAt(i);
    emit widgetRemoved(i);
    if (i < _widgets.size()) {
        setCurrentIndex(i);
    } else if (i - 1 >= 0) {
        setCurrentIndex(i - 1);
    } else {
        setCurrentIndex(-1);
    }
}

QWidget* DASettingContainerWidget::widget(int index) const
{
    if (index >= 0 && index < _widgets.size()) {
        return _widgets[ index ];
    }
    return nullptr;
}

int DASettingContainerWidget::insertWidget(int index, QWidget* widget)
{
    _widgets.insert(index, widget);
    if (_currentIndex < 0) {
        setCurrentIndex(index);
    }
    return _widgets.indexOf(widget);
}

QWidget* DASettingContainerWidget::currentWidget() const
{
    return ui->scrollArea->widget();
}

DAWorkFlowNodeItemSettingWidget* DASettingContainerWidget::getWorkFlowNodeItemSettingWidget()
{
    return _workFlowNodeItemSettingWidget;
}

void DASettingContainerWidget::setCurrentWidget(QWidget* w)
{
    int i = _widgets.indexOf(w);
    if (i < 0) {
        return;
    }
    setCurrentIndex(i);
}

void DASettingContainerWidget::setCurrentIndex(int index)
{
    if (index == _currentIndex) {
        return;
    }
    if (index < 0) {
        //传入-坐标，把显示清除
        ui->scrollArea->takeWidget();
        _currentIndex = -1;
        emit currentChanged(_currentIndex);
        return;
    } else if (index >= _widgets.size()) {
        index = _widgets.size() - 1;
    }
    _currentIndex = index;
    if (ui->scrollArea->widget() == _widgets[ index ]) {
        _widgets[ index ]->show();
    } else {
        ui->scrollArea->takeWidget();
        ui->scrollArea->setWidget(_widgets[ index ]);
        _widgets[ index ]->show();
    }
    emit currentChanged(_currentIndex);
}

void DASettingContainerWidget::initWorkFlowSettingWidgets()
{
    _workFlowNodeItemSettingWidget = new DAWorkFlowNodeItemSettingWidget();
    addWidget(_workFlowNodeItemSettingWidget);
}
