#include "DAAbstractNodeSettingWidget.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
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

    DAPyNode mNodeProxy;
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
 * 按值持有 DAPyNode（内部引用计数 pybind11::object），并在设置时缓存元数据和参数定义。
 * 传入 isNone() 代理时清除缓存。
 *
 * @param[in] proxy 节点代理常量引用
 */
void DAAbstractNodeSettingWidget::setNode(const DAPyNode& proxy)
{
    DA_D(d);
    d->mNodeProxy = proxy;
    d->mParamDefs.clear();
    if (!proxy.isNone()) {
        // 缓存元数据
        d->mMetaData.name          = proxy.getNodeName();
        d->mMetaData.qualifiedName = proxy.getQualifiedName();
        d->mMetaData.category      = proxy.getNodeCategory();
        d->mMetaData.iconPath      = proxy.getIcon();

        // 从Python对象读取parameters，转换为DAParamDef列表
        try {
            if (proxy.hasattr("parameters")) {
                pybind11::list pyParams = proxy.attr("parameters").cast< pybind11::list >();
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
                            // bool 必须在 int 之前检查，因为 Python 的 bool 是 int 的子类
                            if (pybind11::isinstance< pybind11::bool_ >(defaultObj))
                                pd.defaultValue = pybind11::cast< bool >(defaultObj);
                            else if (pybind11::isinstance< pybind11::int_ >(defaultObj))
                                pd.defaultValue = pybind11::cast< int >(defaultObj);
                            else if (pybind11::isinstance< pybind11::float_ >(defaultObj))
                                pd.defaultValue = pybind11::cast< double >(defaultObj);
                            else if (pybind11::isinstance< pybind11::str >(defaultObj))
                                pd.defaultValue = pybind11::cast< QString >(defaultObj);
                        }
                    }
                    if (dict.contains("properties")) {
                        pybind11::object propObj = dict[ "properties" ];
                        if (pybind11::isinstance< pybind11::dict >(propObj)) {
                            pybind11::dict propDict = pybind11::cast< pybind11::dict >(propObj);
                            QVariantHash props;
                            for (auto it = propDict.begin(); it != propDict.end(); ++it) {
                                QString qKey = pybind11::cast< QString >(it->first);
                                pybind11::handle val = it->second;
                                // bool 必须在 int 之前检查，因为 Python 的 bool 是 int 的子类
                                if (pybind11::isinstance< pybind11::bool_ >(val))
                                    props[ qKey ] = pybind11::cast< bool >(val);
                                else if (pybind11::isinstance< pybind11::int_ >(val))
                                    props[ qKey ] = pybind11::cast< int >(val);
                                else if (pybind11::isinstance< pybind11::float_ >(val))
                                    props[ qKey ] = pybind11::cast< double >(val);
                                else if (pybind11::isinstance< pybind11::str >(val))
                                    props[ qKey ] = pybind11::cast< QString >(val);
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

const DAPyNode& DAAbstractNodeSettingWidget::getNode() const
{
    return d_ptr->mNodeProxy;
}

DAPyNode& DAAbstractNodeSettingWidget::node()
{
    return d_ptr->mNodeProxy;
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
