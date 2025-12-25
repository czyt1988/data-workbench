#include "DADataManagerComboBox.h"
#include <QTreeView>
#include "DADataManager.h"
#include "Models/DADataManagerTreeModel.h"
namespace DA
{
class DADataManagerComboBox::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManagerComboBox)
public:
    PrivateData(DADataManagerComboBox* p);

public:
    DADataManagerTreeModel* _treeDataModel { nullptr };
    bool mShowSeriesUnderDataframe { true };
    DAData lastData;  ///< 记录最后的data
};

DADataManagerComboBox::PrivateData::PrivateData(DADataManagerComboBox* p) : q_ptr(p)
{
    _treeDataModel = new DADataManagerTreeModel(q_ptr);
    _treeDataModel->setExpandDataframeToSeries(mShowSeriesUnderDataframe);
    q_ptr->setModel(_treeDataModel);
}

//==============================================================
// DADataManagerComboBox
//==============================================================

DADataManagerComboBox::DADataManagerComboBox(QWidget* par) : ctkTreeComboBox(par), DA_PIMPL_CONSTRUCT
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
    QVariant dvar = currentData(DADataManagerTreeModel::RoleDataId);

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
    if (d_ptr->lastData != d) {
        // 值不相等
        d_ptr->lastData = d;
        Q_EMIT currentDataChanged(d);
    }

    // 判断完值后，判断series，
    // 看看series
    if (!d.isDataFrame()) {
        return;
    }
    QVariant dtype = currentData(DADataManagerTreeModel::RoleItemType);
    QString seriesName;
    if (!dtype.isValid()) {
        return;
    }
    if (dtype.toInt() == DADataManagerTreeModel::SeriesInnerDataframe) {
        seriesName = text;
        Q_EMIT currentDataframeSeriesChanged(d, seriesName);
    }
}

/**
 * @brief 数据发生改变
 * @param d
 * @param t
 */
void DADataManagerComboBox::onDataChanged(const DAData& d, DADataManager::ChangeType t)
{
}

void DADataManagerComboBox::setDataManager(DADataManager* dmgr)
{
    d_ptr->_treeDataModel->setDataManager(dmgr);
    connect(dmgr, &DADataManager::dataChanged, this, &DADataManagerComboBox::onDataChanged);
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
    QVariant dvar = currentData(DADataManagerTreeModel::RoleDataId);
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

    QVariant dtype = currentData(DADataManagerTreeModel::RoleItemType);
    if (dtype.isNull()) {
        // 说明选择的是dataframe这些直接是变量管理器的
        return d;
    } else {
#if DA_ENABLE_PYTHON
        // 说明是Dataframe下的Series
        DADataManagerTreeModel::DetailDataTypeMark m =
            static_cast< DADataManagerTreeModel::DetailDataTypeMark >(dtype.toInt());
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

void DADataManagerComboBox::setCurrentDAData(const DAData& d)
{
    auto i = d_ptr->_treeDataModel->findFirstDataframeItemByData(d);
    if (i) {
        int ni = i->index().row();
        if (ni < count()) {
            setCurrentIndex(ni);
        }
    }
}

void DADataManagerComboBox::setShowSeriesUnderDataframe(bool on)
{
    if (d_ptr->mShowSeriesUnderDataframe != on) {
        d_ptr->mShowSeriesUnderDataframe = on;
        d_ptr->_treeDataModel->setExpandDataframeToSeries(on);
    }
}

bool DADataManagerComboBox::isShowSeriesUnderDataframe() const
{
    return d_ptr->mShowSeriesUnderDataframe;
}

}
