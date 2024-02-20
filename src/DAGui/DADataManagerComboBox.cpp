#include "DADataManagerComboBox.h"
#include <QTreeView>
#include "Models/DADataManagerTreeModel.h"
namespace DA
{
class DADataManagerComboBoxPrivate
{
public:
    DA_IMPL_PUBLIC(DADataManagerComboBox)
    DADataManagerComboBoxPrivate(DADataManagerComboBox* p);

public:
    DADataManagerTreeModel* _treeDataModel { nullptr };
};

DADataManagerComboBoxPrivate::DADataManagerComboBoxPrivate(DADataManagerComboBox* p) : q_ptr(p)
{
    _treeDataModel = new DADataManagerTreeModel(q_ptr);
    _treeDataModel->setExpandDataframeToSeries(true);
    q_ptr->setModel(_treeDataModel);
}

//==============================================================
// DADataManagerComboBox
//==============================================================

DADataManagerComboBox::DADataManagerComboBox(QWidget* par)
    : ctkTreeComboBox(par), d_ptr(new DADataManagerComboBoxPrivate(this))
{
#if QT_VERSION_MAJOR >= 6
    connect(this, &QComboBox::currentTextChanged, this, &DADataManagerComboBox::onCurrentIndexChanged);
#else
    connect(this,
            QOverload< const QString& >::of(&QComboBox::currentIndexChanged),
            this,
            &DADataManagerComboBox::onCurrentIndexChanged);
#endif
}

DADataManagerComboBox::~DADataManagerComboBox()
{
}

void DADataManagerComboBox::showPopup()
{
    treeView()->expandAll();
    ctkTreeComboBox::showPopup();
}

void DADataManagerComboBox::onCurrentIndexChanged(const QString& text)
{
    QVariant dtype = currentData(DADATAMANAGERTREEMODEL_ROLE_DETAIL_DATA_TYPE);
    if (dtype.isNull()) {
        // 说明不是dataframe下的series
        return;
    }
    DADataManagerTreeModel::DetailDataTypeMark m = static_cast< DADataManagerTreeModel::DetailDataTypeMark >(dtype.toInt());
    if (DADataManagerTreeModel::SeriesInnerDataframe == m) {
        QVariant dvar = currentData(DADATAMANAGERTREEMODEL_ROLE_DATA_ID);
        if (dvar.isNull()) {
            // 说明不是异常
            return;
        }
        DAData::IdType did  = dvar.toULongLong();
        DADataManager* dmgr = getDataManager();
        if (!dmgr) {
            return;
        }
        DAData d = dmgr->getDataById(did);
        if (!d.isDataFrame()) {
            return;
        }
        emit currentDataframeSeriesChanged(d, text);
    }
}

void DADataManagerComboBox::setDataManager(DADataManager* dmgr)
{
    d_ptr->_treeDataModel->setDataManager(dmgr);
}

DADataManager* DADataManagerComboBox::getDataManager() const
{
    return d_ptr->_treeDataModel->getDataManager();
}

/**
 * @brief 获取当前的Data
 * @return 如果是Series，也返回DAData,不过这个DAData是没有DataManager的
 */
DAData DADataManagerComboBox::getCurrentDAData() const
{
    QVariant dvar = currentData(DADATAMANAGERTREEMODEL_ROLE_DATA_ID);
    if (dvar.isNull()) {
        // 说明不是异常
        return DAData();
    }
    DAData::IdType did  = dvar.toULongLong();
    DADataManager* dmgr = getDataManager();
    if (!dmgr) {
        return DAData();
    }
    DAData d = dmgr->getDataById(did);

    QVariant dtype = currentData(DADATAMANAGERTREEMODEL_ROLE_DETAIL_DATA_TYPE);
    if (dtype.isNull()) {
        // 说明选择的是dataframe这些直接是变量管理器的
        return d;
    } else {
#if DA_ENABLE_PYTHON
        // 说明是Dataframe下的Series
        DADataManagerTreeModel::DetailDataTypeMark m = static_cast< DADataManagerTreeModel::DetailDataTypeMark >(
            dtype.toInt());
        if (DADataManagerTreeModel::SeriesInnerDataframe == m) {
            QString serName  = currentText();
            DAPyDataFrame df = d.toDataFrame();
            if (df.isNone()) {
                return DAData();
            }
            return df[ serName ];
        }
#endif
    }
    return DAData();
}

}
