#include "DADataManagerTreeWidget.h"
#include "ui_DADataManagerTreeWidget.h"
#include "Models/DADataManagerTreeModel.h"
#include "DADataManager.h"
namespace DA
{

class DADataManagerTreeWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManagerTreeWidget)
public:
    PrivateData(DADataManagerTreeWidget* p);
    void init();

public:
    DADataManagerTreeModel* model { nullptr };
};

DADataManagerTreeWidget::PrivateData::PrivateData(DADataManagerTreeWidget* p) : q_ptr(p)
{
}

void DADataManagerTreeWidget::PrivateData::init()
{
    Ui::DADataManagerTreeWidget* ui = q_ptr->ui;
    model                           = new DADataManagerTreeModel(q_ptr);
    ui->treeView->setModel(model);
}
//===============================================================
// name
//===============================================================

DADataManagerTreeWidget::DADataManagerTreeWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADataManagerTreeWidget)
{
    ui->setupUi(this);
    d_ptr->init();
    connect(ui->toolButtonExpand, &QToolButton::clicked, this, &DADataManagerTreeWidget::onToolButtonExpandClicked);
    connect(ui->toolButtonCollapse, &QToolButton::clicked, this, &DADataManagerTreeWidget::onToolButtonCollapseClicked);
    connect(ui->comboBoxFilter, &QComboBox::editTextChanged, this, &DADataManagerTreeWidget::onComboBoxEditTextChanged);
}

DADataManagerTreeWidget::~DADataManagerTreeWidget()
{
    delete ui;
}

void DADataManagerTreeWidget::setDataManager(DADataManager* dataMgr)
{
    DA_D(d);
    d->model->setDataManager(dataMgr);
}

DADataManager* DADataManagerTreeWidget::getDataManager() const
{
    return d_ptr->model->getDataManager();
}

void DADataManagerTreeWidget::setExpandDataframeToSeries(bool on)
{
}

bool DADataManagerTreeWidget::isExpandDataframeToSeries() const
{
}

void DADataManagerTreeWidget::setColumnStyle(DADataManagerTreeModel::ColumnStyle style)
{
}

DADataManagerTreeModel::ColumnStyle DADataManagerTreeWidget::getColumnStyle() const
{
}

void DADataManagerTreeWidget::changeEvent(QEvent* e)
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

/**
 * @brief 展开树形所有节点
 */
void DADataManagerTreeWidget::onToolButtonExpandClicked()
{
}

/**
 * @brief 收起树形说有节点
 */
void DADataManagerTreeWidget::onToolButtonCollapseClicked()
{
}

/**
 * @brief DADataManagerTreeWidget::onComboBoxEditTextChanged
 * @param text
 */
void DADataManagerTreeWidget::onComboBoxEditTextChanged(const QString& text)
{
}

void DADataManagerTreeWidget::updateCompleterModel()
{
}

void DADataManagerTreeWidget::initCompleter()
{
}

void DADataManagerTreeWidget::updateFilter(const QString& text)
{
}

QStringList DADataManagerTreeWidget::collectAllDataNames() const
{
}

}  // end DA
