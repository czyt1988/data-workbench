#include "DADataManagerComboBox.h"
#include <QTreeView>
#include "DADataManagerTreeModel.h"
namespace DA
{
class DADataManagerComboBoxPrivate
{
public:
    DA_IMPL_PUBLIC(DADataManagerComboBox)
    DADataManagerComboBoxPrivate(DADataManagerComboBox* p);

public:
    QTreeView* _treeView { nullptr };
    DADataManagerTreeModel* _treeDataModel { nullptr };
};

DADataManagerComboBoxPrivate::DADataManagerComboBoxPrivate(DADataManagerComboBox* p) : q_ptr(p)
{
    _treeView = new QTreeView(p);
    _treeView->setHeaderHidden(true);
    _treeDataModel = new DADataManagerTreeModel(_treeView);
    _treeDataModel->setExpandDataframeToSeries(true);
    _treeView->setModel(_treeDataModel);
    // view必须有model
    q_ptr->setView(_treeView);
    q_ptr->setModel(_treeDataModel);
}

//==============================================================
// DADataManagerComboBox
//==============================================================

DADataManagerComboBox::DADataManagerComboBox(QWidget* par)
    : QComboBox(par), d_ptr(new DADataManagerComboBoxPrivate(this))
{
}

DADataManagerComboBox::~DADataManagerComboBox()
{
}

void DADataManagerComboBox::showPopup()
{
    if (d_ptr->_treeView) {
        d_ptr->_treeView->expandAll();
    }
    QComboBox::showPopup();
}

void DADataManagerComboBox::setDataManager(DADataManager* dmgr)
{
    d_ptr->_treeDataModel->setDataManager(dmgr);
}

DADataManager* DADataManagerComboBox::getDataManager() const
{
    return d_ptr->_treeDataModel->getDataManager();
}

}
