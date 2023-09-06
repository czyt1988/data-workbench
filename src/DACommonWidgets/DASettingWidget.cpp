#include "DASettingWidget.h"
#include "ui_DASettingWidget.h"
#include "DAAbstractSettingPage.h"
#include <QDebug>
namespace DA
{

DASettingWidget::DASettingWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DASettingWidget)
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 4);
    connect(ui->listWidget, &QListWidget::currentRowChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
}

DASettingWidget::~DASettingWidget()
{
    delete ui;
}

/**
 * @brief 获取页面的数量
 * @return
 */
int DASettingWidget::count() const
{
    return ui->stackedWidget->count();
}

/**
 * @brief 添加页面
 * @param icon
 * @param title
 * @param page
 */
void DASettingWidget::addPage(DAAbstractSettingPage* page)
{
    Q_CHECK_PTR(page);
    QListWidgetItem* item = new QListWidgetItem(page->getSettingPageIcon(), page->getSettingPageTitle());
    ui->listWidget->addItem(item);
    ui->stackedWidget->addWidget(page);
    connect(page, &DAAbstractSettingPage::settingChanged, this, &DASettingWidget::onPageSettingChanged);
}

/**
 * @brief 应用所有不管有没有改变
 */
void DASettingWidget::applyAll()
{
    const int c = count();
    for (int i = 0; i < c; ++i) {
        DAAbstractSettingPage* page = qobject_cast< DAAbstractSettingPage* >(ui->stackedWidget->widget(i));
        if (!page) {
            continue;
        }
        page->apply();
    }
    mChangedPages.clear();
    emit settingApplyed();
}

/**
 * @brief 应用改变
 */
void DASettingWidget::applyChanged()
{
    for (DAAbstractSettingPage* p : qAsConst(mChangedPages)) {
        p->apply();
    }
    mChangedPages.clear();
    emit settingApplyed();
}

/**
 * @brief 获取改变的页面
 * @return
 */
QList< DAAbstractSettingPage* > DASettingWidget::getChanggedPages() const
{
    return mChangedPages.toList();
}

/**
 * @brief 设置当前页面
 * @param index
 */
void DASettingWidget::setPage(int index)
{
    ui->listWidget->setCurrentRow(index);
}

void DASettingWidget::onPageSettingChanged()
{
    DAAbstractSettingPage* page = qobject_cast< DAAbstractSettingPage* >(sender());
    if (!page) {
        qCritical() << tr("page changed,but can not catch the sender widget");  // cn:检测到配置页的改变，但无法捕获改变的配置页
        return;
    }
    mChangedPages.insert(page);
    emit settingChanged();
}
}  // DA
