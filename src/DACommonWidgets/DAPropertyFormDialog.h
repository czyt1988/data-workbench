#ifndef DAPROPERTYFORMDIALOG_H
#define DAPROPERTYFORMDIALOG_H

#include "DACommonWidgetsAPI.h"
#include <QDialog>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>

namespace DA
{
class DAPropertyFormWidget;

/**
 * @brief 通用属性表单模态对话框
 *
 * 以 DAPropertyFormWidget 为主体，底部附带 OK/Cancel 按钮盒，
 * 提供 JSON 配置驱动的属性编辑弹窗。支持从 QJsonObject 或 JSON 字符串加载表单规格，
 * 并提供静态便捷方法 showSettingsDialog 一次性完成 加载→弹窗→收集结果。
 *
 * @code
 * QJsonObject result = DAPropertyFormDialog::showSettingsDialog(jsonConfig, parent);
 * if (!result.isEmpty()) {
 *     // 用户点击 OK
 * }
 * @endcode
 *
 * @see DAPropertyFormWidget, DAFormSchemaIO
 */
class DACOMMONWIDGETS_API DAPropertyFormDialog : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPropertyFormDialog)
public:
    explicit DAPropertyFormDialog(QWidget* parent = nullptr);
    ~DAPropertyFormDialog();

    // 从 JSON 对象加载表单规格，失败返回 false
    bool loadFromJsonObject(const QJsonObject& jsonObj);
    // 从 JSON 字符串加载表单规格，失败返回 false
    bool loadFromJson(const QString& jsonStr);

    // 设置字段值（转发给内部表单）
    void setValues(const QVariantMap& values);
    // 收集字段值（转发给内部表单）
    QVariantMap values() const;

    // 静态便捷方法：加载 JSON 对象配置并弹出模态表单，确认返回字段值 JSON，取消返回空对象
    static QJsonObject showSettingsDialog(const QJsonObject& jsonConfig, QWidget* parent = nullptr);

    // 静态便捷方法：加载 JSON 字符串配置并弹出模态表单，确认返回字段值 JSON，取消返回空对象
    static QJsonObject showSettingsDialog(const QString& jsonConfig, QWidget* parent = nullptr);
};
}  // namespace DA

#endif  // DAPROPERTYFORMDIALOG_H
