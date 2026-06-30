#include "DAPropertyFormDialog.h"
#include "DAGlobals.h"
#include "DAFormSchemaIO.h"
#include "DAFormSpec.h"
#include "DAPropertyFormWidget.h"
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QVBoxLayout>
#include <QtGlobal>

namespace DA
{
namespace
{
/**
 * @brief 递归收集表单规格中所有字段的默认值
 *
 * 遍历顶层条目与分组子条目，将具有有效 defaultValue 的字段收集为 字段名→默认值 映射。
 * @param[in] items 条目列表
 * @param[out] out 收集结果
 */
void collectDefaults(const QList< DAFormItemDef >& items, QVariantMap& out)
{
    for (const DAFormItemDef& item : items) {
        if (item.kind == DAFormItemDef::Group && item.group) {
            collectDefaults(item.group->items, out);
        } else if (item.kind == DAFormItemDef::Field) {
            if (item.field.defaultValue.isValid() && !item.field.defaultValue.isNull()) {
                out[ item.field.name ] = item.field.defaultValue;
            }
        }
    }
}
}  // namespace

class DAPropertyFormDialog::PrivateData
{
    DA_DECLARE_PUBLIC(DAPropertyFormDialog)
public:
    PrivateData(DAPropertyFormDialog* p) : q_ptr(p) {}
    DAPropertyFormWidget* formWidget { nullptr };
};

/**
 * @brief 构造函数，搭建表单主体与 OK/Cancel 按钮盒
 */
DAPropertyFormDialog::DAPropertyFormDialog(QWidget* parent) : QDialog(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->formWidget = new DAPropertyFormWidget(this);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DAPropertyFormDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DAPropertyFormDialog::reject);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(d->formWidget);
    layout->addWidget(buttonBox);
}

DAPropertyFormDialog::~DAPropertyFormDialog()
{
}

/**
 * @brief 从 JSON 对象加载表单规格
 *
 * 使用 DAFormSchemaIO 解析，失败时输出 qWarning 并返回 false。
 * 成功后设置表单规格、窗口标题，并将字段默认值写入表单。
 * @param[in] jsonObj 表单规格 JSON 对象
 * @return 解析成功返回 true，否则 false
 */
bool DAPropertyFormDialog::loadFromJsonObject(const QJsonObject& jsonObj)
{
    DA_D(d);
    DAFormSpec spec;
    QString errMsg;
    if (!DAFormSchemaIO::fromJsonObject(jsonObj, spec, &errMsg)) {
        qWarning("DAPropertyFormDialog: failed to load form spec: %s", qPrintable(errMsg));
        return false;
    }
    d->formWidget->setFormSpec(spec);
    if (!spec.title.isEmpty()) {
        setWindowTitle(spec.title);
    }
    // 收集并写入字段默认值（表单控件已在构建时应用默认值，此处显式同步确保一致）
    QVariantMap defaults;
    collectDefaults(spec.items, defaults);
    if (!defaults.isEmpty()) {
        d->formWidget->setValues(defaults);
    }
    // 根据表单内容和屏幕几何调整初始尺寸，避免弹出窗口过小或超出屏幕
    adjustSizeToForm();
    return true;
}

/**
 * @brief 从 JSON 字符串加载表单规格
 *
 * @param[in] jsonStr 表单规格 JSON 字符串
 * @return 解析成功返回 true，否则 false
 */
bool DAPropertyFormDialog::loadFromJson(const QString& jsonStr)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning("DAPropertyFormDialog: JSON parse error at offset %d: %s", int(err.offset), qPrintable(err.errorString()));
        return false;
    }
    if (!doc.isObject()) {
        qWarning("DAPropertyFormDialog: JSON document is not an object");
        return false;
    }
    return loadFromJsonObject(doc.object());
}

/**
 * @brief 设置字段值，转发给内部表单
 */
void DAPropertyFormDialog::setValues(const QVariantMap& values)
{
    DA_D(d);
    d->formWidget->setValues(values);
}

/**
 * @brief 收集字段值，转发给内部表单
 */
QVariantMap DAPropertyFormDialog::values() const
{
    DA_DC(d);
    return d->formWidget->values();
}

/**
 * @brief 根据表单内容和所在屏幕可用几何调整对话框初始尺寸
 *
 * 依据内部表单控件的 sizeHint 估算理想尺寸，并以所在屏幕的可用几何
 * （排除任务栏、多屏边界）作为上下限进行 clamp，保证：
 *   - 弹出即可见全部关键控件，无需手动拉大；
 *   - 永不超出屏幕边界；
 *   - 字段过多时由内部 QScrollArea 自动出现垂直滚动条。
 *
 * 多屏幕处理：优先取父窗口所在屏幕；无父窗口或父窗口几何异常时回退到主屏幕。
 */
void DAPropertyFormDialog::adjustSizeToForm()
{
    DA_D(d);
    // 强制布局重新计算，确保 sizeHint 反映当前已构建的表单内容
    if (auto* lay = layout()) {
        lay->activate();
    }
    // 表单内容理想尺寸。DAPropertyPanelContainerWidget 内部的 QScrollArea 在
    // widgetResizable=true 下，sizeHint 会反映所有字段的累积高度，字段越多高度越大。
    QSize formHint = d->formWidget->sizeHint().expandedTo(QSize(0, 0));

    // 解析对话框应所在的屏幕（多屏幕感知）
    QScreen* screen = nullptr;
    if (QWidget* p = parentWidget()) {
        screen = QGuiApplication::screenAt(p->mapToGlobal(QPoint(0, 0)));
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    // 上限：屏幕可用区的 90%（宽）/ 85%（高），为任务栏与多屏边界留出余量
    const int maxW = int(avail.width() * 0.9);
    const int maxH = int(avail.height() * 0.85);
    // 下限：保证基本可用，且不超过上限（小屏设备兜底）
    const int minW = qMin(400, maxW);
    const int minH = qMin(320, maxH);

    // 理想窗口尺寸 = 表单 sizeHint + 对话框 chrome 预留
    // 宽度：+40（左右内容边距 + 可能的垂直滚动条宽度）
    // 高度：+100（OK/Cancel 按钮盒 ~32px + 上下边距 + spacing + 分组标题余量）
    const int idealW = formHint.width() + 40;
    const int idealH = formHint.height() + 100;

    const int w = qBound(minW, idealW, maxW);
    const int h = qBound(minH, idealH, maxH);

    resize(w, h);
}

/**
 * @brief 静态便捷方法：加载 JSON 对象配置并弹出模态表单
 *
 * @param[in] jsonConfig 表单规格 JSON 对象
 * @param[in] parent 父窗口
 * @return 用户确认时返回字段值的 JSON 对象，取消或加载失败时返回空对象
 */
QJsonObject DAPropertyFormDialog::showSettingsDialog(const QJsonObject& jsonConfig, QWidget* parent)
{
    DAPropertyFormDialog dialog(parent);
    if (!dialog.loadFromJsonObject(jsonConfig)) {
        return QJsonObject();
    }
    if (dialog.exec() == QDialog::Accepted) {
        return QJsonObject::fromVariantMap(dialog.values());
    }
    return QJsonObject();
}

/**
 * @brief 静态便捷方法：加载 JSON 字符串配置并弹出模态表单
 *
 * @param[in] jsonConfig 表单规格 JSON 字符串
 * @param[in] parent 父窗口
 * @return 用户确认时返回字段值的 JSON 对象，取消或加载失败时返回空对象
 */
QJsonObject DAPropertyFormDialog::showSettingsDialog(const QString& jsonConfig, QWidget* parent)
{
    DAPropertyFormDialog dialog(parent);
    if (!dialog.loadFromJson(jsonConfig)) {
        return QJsonObject();
    }
    if (dialog.exec() == QDialog::Accepted) {
        return QJsonObject::fromVariantMap(dialog.values());
    }
    return QJsonObject();
}

}  // namespace DA
