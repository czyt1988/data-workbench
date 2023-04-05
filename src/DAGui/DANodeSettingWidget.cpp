#include "DANodeSettingWidget.h"
#include "ui_DANodeSettingWidget.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DANodeSettingWidget
//===================================================
DANodeSettingWidget::DANodeSettingWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DANodeSettingWidget)
{
    ui->setupUi(this);
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &DANodeSettingWidget::onLineEditNameTextEdited);
}

DANodeSettingWidget::~DANodeSettingWidget()
{
    delete ui;
}

void DANodeSettingWidget::setNode(DAAbstractNode::SharedPointer p)
{
    _nodePtr = p;
    updateData();
}

DAAbstractNode::SharedPointer DANodeSettingWidget::getNode() const
{
    return _nodePtr.lock();
}

void DANodeSettingWidget::updateData()
{
    DAAbstractNode::SharedPointer n = getNode();
    if (n) {
        QSignalBlocker b(ui->lineEditName);
        Q_UNUSED(b);
        ui->lineEditGroup->setText(n->getNodeGroup());
        ui->lineEditPrototype->setText(n->getNodePrototype());
        ui->lineEditName->setText(n->getNodeName());
    } else {
        ui->lineEditGroup->clear();
        ui->lineEditPrototype->clear();
        ui->lineEditName->clear();
    }
}

void DANodeSettingWidget::onLineEditNameTextEdited(const QString& t)
{
    DAAbstractNode::SharedPointer p = getNode();
    if (p) {
        p->setNodeName(t);
        //设置完成后重新设置回editor
        QString n = p->getNodeName();
        ui->lineEditName->setText(n);
    }
}
