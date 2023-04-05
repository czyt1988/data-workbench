#include "DASettingWidget.h"
#include "ui_DASettingWidget.h"
#include "DAAbstractSettingPage.h"
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
void DASettingWidget::addPage(const QIcon& icon, const QString& title, DAAbstractSettingPage* page)
{
    Q_CHECK_PTR(page);
    QListWidgetItem* item = new QListWidgetItem(icon, title);
    ui->listWidget->addItem(item);
    ui->stackedWidget->addWidget(page);
    connect(page, &DAAbstractSettingPage::settingChanged, this, &DASettingWidget::onPageSettingChanged);
}

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
    _changedPages.clear();
}

void DASettingWidget::onPageSettingChanged()
{
    DAAbstractSettingPage* page = qobject_cast< DAAbstractSettingPage* >(sender());
    if (!page) {
        return;
    }
    _changedPages.insert(page);
    emit settingChanged();
}
}  // DA
