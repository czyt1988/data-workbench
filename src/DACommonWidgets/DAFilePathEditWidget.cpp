#include "DAFilePathEditWidget.h"
#include "ui_DAFilePathEditWidget.h"
#include <QFileDialog>
namespace DA
{

class DAFilePathEditWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAFilePathEditWidget)
public:
    PrivateData(DAFilePathEditWidget* p);

public:
    QStringList mNameFilters;
};

DAFilePathEditWidget::PrivateData::PrivateData(DAFilePathEditWidget* p) : q_ptr(p)
{
}

//===============================================================
// DAFilePathEditWidget
//===============================================================

DAFilePathEditWidget::DAFilePathEditWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAFilePathEditWidget)
{
    ui->setupUi(this);
    connect(ui->toolButtonOpen, &QToolButton::clicked, this, &DAFilePathEditWidget::onToolButtonOpenClicked);
    connect(ui->lineEdit, &QLineEdit::editingFinished, this, &DAFilePathEditWidget::onLineEditEditingFinished);
}

DAFilePathEditWidget::~DAFilePathEditWidget()
{
    delete ui;
}

void DAFilePathEditWidget::setNameFilter(const QString& filter)
{
    QStringList filters;
    filters.append(filter);
    setNameFilters(filters);
}

void DAFilePathEditWidget::setNameFilters(const QStringList& filters)
{
    d_ptr->mNameFilters = filters;
}

/**
 * @brief 获取用户输入框的路径
 * @return
 */
QString DAFilePathEditWidget::getFilePath() const
{
    return ui->lineEdit->text();
}

/**
 * @brief 设置输入框的路径
 * @param v
 */
void DAFilePathEditWidget::setFilePath(const QString& v)
{
    ui->lineEdit->setText(v);
}

void DAFilePathEditWidget::onToolButtonOpenClicked()
{
    QFileDialog fileDialog;
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    if (!d_ptr->mNameFilters.isEmpty()) {
        fileDialog.setNameFilters(d_ptr->mNameFilters);
    }
    if (fileDialog.exec()) {
        auto files = fileDialog.selectedFiles();
        if (!files.isEmpty()) {
            QString p = files.back();
            ui->lineEdit->setText(p);
            emit selectedPath(p);
        }
    }
}

void DAFilePathEditWidget::onLineEditEditingFinished()
{
    QString p = getFilePath();
    emit selectedPath(p);
}

void DAFilePathEditWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

}
