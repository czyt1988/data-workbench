#include "DAAbstractNodeSettingWidget.h"
#include "DAPyDictConverter.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include <QJsonArray>

namespace DA
{

class DAAbstractNodeSettingWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractNodeSettingWidget)

public:
    explicit PrivateData(DAAbstractNodeSettingWidget* q) : q_ptr(q)
    {
    }

    DAPyNode* mNodeProxy = nullptr;
    DAPyNodeMetaData mMetaData;
    QVector< DAParamDef > mParamDefs;
};

// ============================================================
// 构造与析构
// ============================================================

DAAbstractNodeSettingWidget::DAAbstractNodeSettingWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
}

DAAbstractNodeSettingWidget::~DAAbstractNodeSettingWidget()
{
}

// ============================================================
// 节点代理管理
// ============================================================

/**
 * @brief 设置节点代理
 *
 * 使用 QPointer 安全持有 DAPyNode，并在设置时缓存元数据和参数定义。
 * 传入 nullptr 时清除缓存。
 *
 * @param[in] proxy 节点代理指针，可为 nullptr
 */
void DAAbstractNodeSettingWidget::setNodeProxy(DAPyNode* proxy)
{
    DA_D(d);
    d->mNodeProxy = proxy;
    d->mParamDefs.clear();
    if (proxy) {
        // 缓存元数据
        d->mMetaData.name          = proxy->getNodeName();
        d->mMetaData.qualifiedName = proxy->getQualifiedName();
        d->mMetaData.group         = proxy->getNodeGroup();
        d->mMetaData.iconPath      = proxy->getIcon();
        d->mMetaData.inputKeys     = proxy->getInputKeys();
        d->mMetaData.outputKeys    = proxy->getOutputKeys();

        // 从Python对象读取parameters，转换为DAParamDef列表
        if (!proxy->isNone()) {
            try {
                if (proxy->hasattr("parameters")) {
                    pybind11::list pyParams = proxy->attr("parameters").cast< pybind11::list >();
                    for (auto item : pyParams) {
                        pybind11::dict dict = pybind11::cast< pybind11::dict >(item);
                        DAParamDef pd;
                        if (dict.contains("name"))
                            pd.name = pybind11::cast< QString >(dict[ "name" ]);
                        if (dict.contains("type"))
                            pd.type = pybind11::cast< QString >(dict[ "type" ]);
                        if (dict.contains("description"))
                            pd.description = pybind11::cast< QString >(dict[ "description" ]);
                        if (dict.contains("default")) {
                            pybind11::object defaultObj = dict[ "default" ];
                            if (!defaultObj.is_none()) {
                                if (pybind11::isinstance< pybind11::str >(defaultObj))
                                    pd.defaultValue = pybind11::cast< QString >(defaultObj);
                                else if (pybind11::isinstance< pybind11::int_ >(defaultObj))
                                    pd.defaultValue = pybind11::cast< int >(defaultObj);
                                else if (pybind11::isinstance< pybind11::float_ >(defaultObj))
                                    pd.defaultValue = pybind11::cast< double >(defaultObj);
                                else if (pybind11::isinstance< pybind11::bool_ >(defaultObj))
                                    pd.defaultValue = pybind11::cast< bool >(defaultObj);
                            }
                        }
                        if (dict.contains("properties")) {
                            pybind11::object propObj = dict[ "properties" ];
                            if (pybind11::isinstance< pybind11::dict >(propObj)) {
                                pybind11::dict propDict = pybind11::cast< pybind11::dict >(propObj);
                                QVariantHash props;
                                for (auto propItem : propDict) {
                                    std::string key = pybind11::cast< std::string >(propItem.first);
                                    pybind11::object val = pybind11::reinterpret_borrow< pybind11::object >(propItem.second);
                                    QString qKey = QString::fromStdString(key);
                                    if (pybind11::isinstance< pybind11::str >(val))
                                        props[ qKey ] = pybind11::cast< QString >(val);
                                    else if (pybind11::isinstance< pybind11::int_ >(val))
                                        props[ qKey ] = pybind11::cast< int >(val);
                                    else if (pybind11::isinstance< pybind11::float_ >(val))
                                        props[ qKey ] = pybind11::cast< double >(val);
                                    else if (pybind11::isinstance< pybind11::bool_ >(val))
                                        props[ qKey ] = pybind11::cast< bool >(val);
                                    else if (pybind11::isinstance< pybind11::list >(val)) {
                                        QStringList strList;
                                        pybind11::list pyList = pybind11::cast< pybind11::list >(val);
                                        for (auto listItem : pyList)
                                            strList.append(pybind11::cast< QString >(listItem));
                                        props[ qKey ] = strList;
                                    }
                                }
                                pd.propertys = props;
                            }
                        }
                        d->mParamDefs.append(pd);
                    }
                }
            } catch (const std::exception& e) {
                qWarning() << "DAAbstractNodeSettingWidget::setNodeProxy: failed to read parameters:" << e.what();
            }
        }
    }
}

DAPyNode* DAAbstractNodeSettingWidget::getNodeProxy() const
{
    DA_DC(d);
    return d->mNodeProxy;
}

// ============================================================
// 元数据访问
// ============================================================

const DAPyNodeMetaData& DAAbstractNodeSettingWidget::getMetaData() const
{
    return d_ptr->mMetaData;
}

const QVector< DAParamDef >& DAAbstractNodeSettingWidget::getParamDefs() const
{
    return d_ptr->mParamDefs;
}

}  // namespace DA
