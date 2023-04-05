#include "DASettingPageCommon.h"
#include "ui_DASettingPageCommon.h"
namespace DA
{

DASettingPageCommon::DASettingPageCommon(QWidget* parent)
    : DAAbstractSettingPage(parent), ui(new Ui::DASettingPageCommon)
{
    ui->setupUi(this);
}

DASettingPageCommon::~DASettingPageCommon()
{
    delete ui;
}

void DASettingPageCommon::apply()
{
}
}
