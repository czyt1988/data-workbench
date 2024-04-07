#include "DialogWithButton.h"
#include "ui_DialogWithButton.h"

DialogWithButton::DialogWithButton(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogWithButton)
{
    ui->setupUi(this);
}

DialogWithButton::~DialogWithButton()
{
    delete ui;
}
