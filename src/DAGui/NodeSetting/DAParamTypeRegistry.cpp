#include "DAParamTypeRegistry.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QFontComboBox>
#include <QFileDialog>
#include <QToolButton>
#include "DAFilePathEditWidget.h"
#include "DAColorPickerButton.h"
#include "DAFontEditPannelWidget.h"

namespace DA
{

class DAParamTypeRegistry::PrivateData
{
    DA_DECLARE_PUBLIC(DAParamTypeRegistry)
public:
    PrivateData(DAParamTypeRegistry* p);

    std::map< QString, ParamEditorCreator > creators;

    // DAPyNodeParameter → DAParamDef 转换（供编辑器创建函数内部使用）
    static DAParamDef toParamDef(const DAPyNodeParameter& param);

    // 内置类型编辑器创建函数
    static QWidget* createStrEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createIntEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createFloatEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createBoolEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createEnumEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createListEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createFileEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createFolderEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createColorEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createFontEditor(const DAPyNodeParameter& param, QWidget* parent);
    static QWidget* createCodeEditor(const DAPyNodeParameter& param, QWidget* parent);
};

DAParamTypeRegistry::PrivateData::PrivateData(DAParamTypeRegistry* p) : q_ptr(p)
{
}

//===============================================================
// DAParamTypeRegistry
//===============================================================

/**
 * @brief 默认构造函数，自动注册所有内置类型
 *
 * 构造时即调用 registerDefaults() 注册 11 种内置类型到 creators 映射表中。
 */
DAParamTypeRegistry::DAParamTypeRegistry() : DA_PIMPL_CONSTRUCT
{
    registerDefaults();
}

DAParamTypeRegistry::~DAParamTypeRegistry()
{
}

/**
 * @brief 注册自定义类型的编辑器创建函数
 *
 * 如果 typeStr 已存在，则覆盖原有 creator。
 *
 * @param[in] typeStr 类型名称字符串（如 "str"、"int" 等）
 * @param[in] creator 编辑器创建函数
 */
void DAParamTypeRegistry::registerType(const QString& typeStr, ParamEditorCreator creator)
{
    DA_D(d);
    d->creators[ typeStr ] = creator;
}

/**
 * @brief 重新注册所有内置默认类型
 *
 * 注册 11 种内置类型的编辑器创建函数：
 * str, int, float, bool, enum, list, file, folder, color, font, code。
 * 调用此方法会覆盖任何已有类型（包括自定义注册）的 creator。
 */
void DAParamTypeRegistry::registerDefaults()
{
    DA_D(d);
    d->creators[ "str" ]    = &PrivateData::createStrEditor;
    d->creators[ "int" ]    = &PrivateData::createIntEditor;
    d->creators[ "float" ]  = &PrivateData::createFloatEditor;
    d->creators[ "bool" ]   = &PrivateData::createBoolEditor;
    d->creators[ "enum" ]   = &PrivateData::createEnumEditor;
    d->creators[ "list" ]   = &PrivateData::createListEditor;
    d->creators[ "file" ]   = &PrivateData::createFileEditor;
    d->creators[ "folder" ] = &PrivateData::createFolderEditor;
    d->creators[ "color" ]  = &PrivateData::createColorEditor;
    d->creators[ "font" ]   = &PrivateData::createFontEditor;
    d->creators[ "code" ]   = &PrivateData::createCodeEditor;
}

/**
 * @brief 根据类型字符串和参数代理创建对应的编辑器控件
 *
 * 在 creators 映射表中查找 typeStr 对应的创建函数并调用。
 * 若 typeStr 未注册，返回 nullptr。
 *
 * @param[in] typeStr 类型名称字符串
 * @param[in] param 参数代理
 * @param[in] parent 父控件指针
 * @return 创建的编辑器 QWidget 指针，未注册类型返回 nullptr
 */
QWidget* DAParamTypeRegistry::createEditor(const QString& typeStr, const DAPyNodeParameter& param, QWidget* parent) const
{
    DA_DC(d);
    auto it = d->creators.find(typeStr);
    if (it == d->creators.end()) {
        return nullptr;
    }
    return it->second(param, parent);
}

/**
 * @brief 获取所有已注册类型的名称列表
 *
 * @return 已注册类型名称的 QStringList
 */
QStringList DAParamTypeRegistry::supportedTypes() const
{
    DA_DC(d);
    QStringList result;
    for (const auto& pair : d->creators) {
        result.append(pair.first);
    }
    return result;
}

/**
 * @brief 检查指定类型是否已注册
 *
 * @param[in] typeStr 类型名称字符串
 * @return 已注册返回 true，否则返回 false
 */
bool DAParamTypeRegistry::isRegistered(const QString& typeStr) const
{
    DA_DC(d);
    return d->creators.find(typeStr) != d->creators.end();
}

//===============================================================
// 转换辅助
//===============================================================

/**
 * @brief 将DAPyNodeParameter转换为DAParamDef
 *
 * 从参数代理读取元数据，构建等价的DAParamDef结构体，
 * 供编辑器创建函数复用DAParamDef的helper方法。
 *
 * @param[in] param 参数代理
 * @return 等价的DAParamDef
 */
DAParamDef DAParamTypeRegistry::PrivateData::toParamDef(const DAPyNodeParameter& param)
{
    DAParamDef def;
    def.name         = param.name();
    def.type         = param.typeLabel();
    def.description  = param.description();
    def.defaultValue = param.defaultValue();
    def.propertys    = param.properties();
    return def;
}

//===============================================================
// 内置编辑器创建函数
//===============================================================

/**
 * @brief 创建字符串类型编辑器
 *
 * 根据 layout 扩展属性选择编辑器控件：
 * - "inline"（默认）: QLineEdit 单行输入
 * - "below": QPlainTextEdit 多行输入，占满整行宽度，默认最小高度 80px
 *
 * below 模式下可通过 height 扩展属性覆盖默认高度。
 * 从参数代理的 description 设置 placeholder。
 * 若有默认值，设置为初始文本。
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return QLineEdit 或 QPlainTextEdit 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createStrEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc = toParamDef(param);
    if (desc.isLayoutBelow()) {
        // below 模式：多行编辑器
        QPlainTextEdit* edit = new QPlainTextEdit(parent);
        // 从 description 设置 placeholder
        if (!desc.description.isEmpty()) {
            edit->setPlaceholderText(desc.description);
        }
        // 高度：默认 80，可被 height 属性覆盖
        bool heightOk = false;
        int h         = desc.getHeightProperty(&heightOk);
        if (!heightOk || h <= 0) {
            h = 80;
        }
        edit->setMinimumHeight(h);
        // 设置默认值
        QString str = desc.defaultValue.toString();
        if (!str.isEmpty()) {
            edit->setPlainText(str);
        }
        return edit;
    }
    // inline 模式：保持原有 QLineEdit 行为
    QLineEdit* edit = new QLineEdit(parent);
    // 从 description 设置 placeholder
    if (!desc.description.isEmpty()) {
        edit->setPlaceholderText(desc.description);
    }
    // 设置默认值
    QString str = desc.defaultValue.toString();
    if (!str.isEmpty()) {
        edit->setText(str);
    }
    return edit;
}

/**
 * @brief 创建整数类型编辑器（QSpinBox）
 *
 * 默认范围 -9999 到 9999，默认值 0。
 * 从扩展属性的 min/max 覆盖范围，从默认值设置初始值。
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return QSpinBox 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createIntEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc   = toParamDef(param);
    QSpinBox* spinBox = new QSpinBox(parent);
    spinBox->setRange(-9999, 9999);
    // 读取 min/max
    int val;
    bool isok = false;
    val       = desc.getMinProperty(&isok);
    if (isok) {
        spinBox->setMinimum(val);
    }
    val = desc.getMaxProperty(&isok);
    if (isok) {
        spinBox->setMaximum(val);
    }
    // 设置默认值
    val = desc.defaultValue.toInt(&isok);
    if (isok) {
        spinBox->setValue(val);
    }
    return spinBox;
}

/**
 * @brief 创建浮点类型编辑器（QDoubleSpinBox）
 *
 * 默认范围 -9999.0 到 9999.0，小数位 2，默认值 0.0。
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return QDoubleSpinBox 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createFloatEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc         = toParamDef(param);
    QDoubleSpinBox* spinBox = new QDoubleSpinBox(parent);
    spinBox->setRange(-9999.0, 9999.0);
    spinBox->setDecimals(2);
    // 从描述符读取 min/max/decimals
    double val;
    bool isok = false;
    val       = desc.getMinFProperty(&isok);
    if (isok) {
        spinBox->setMinimum(val);
    }
    val = desc.getMaxFProperty(&isok);
    if (isok) {
        spinBox->setMaximum(val);
    }
    val = desc.getDecimalsProperty(&isok);
    if (isok) {
        spinBox->setDecimals(val);
    }
    // 设置默认值
    val = desc.defaultValueToInt(&isok);
    if (isok) {
        spinBox->setValue(val);
    }
    return spinBox;
}

/**
 * @brief 创建布尔类型编辑器（QCheckBox）
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return QCheckBox 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createBoolEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc     = toParamDef(param);
    QCheckBox* checkBox = new QCheckBox(parent);
    // 设置默认值
    bool defaultVal = desc.defaultValueToBool();
    checkBox->setChecked(defaultVal);
    return checkBox;
}

/**
 * @brief 创建枚举类型编辑器（QComboBox）
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return QComboBox 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createEnumEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc     = toParamDef(param);
    QComboBox* comboBox = new QComboBox(parent);
    comboBox->setEditable(false);
    // 获取enum属性
    const QList< QPair< QString, int > > enumList = desc.getEnumListProperty();

    for (const QPair< QString, int >& item : enumList) {
        comboBox->addItem(item.first, item.second);
    }

    return comboBox;
}

/**
 * @brief 创建列表类型编辑器（复合控件：QListWidget + 添加/删除按钮）
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return 复合 QWidget 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createListEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc      = toParamDef(param);
    QWidget* container   = new QWidget(parent);
    QHBoxLayout* hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);

    QListWidget* listWidget = new QListWidget(container);
    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    hLayout->addWidget(listWidget, 1);

    QVBoxLayout* btnLayout = new QVBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(2);

    QPushButton* btnAdd    = new QPushButton(QObject::tr("添加"), container);
    QPushButton* btnRemove = new QPushButton(QObject::tr("删除"), container);
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnRemove);
    btnLayout->addStretch();
    hLayout->addLayout(btnLayout);
    // 添加枚举
    const QList< QPair< QString, int > > enumList = desc.getEnumListProperty();
    for (const QPair< QString, int >& item : enumList) {
        QListWidgetItem* listitem = new QListWidgetItem(item.first);
        listitem->setData(Qt::UserRole, item.second);
        listWidget->addItem(listitem);
    }
    // 连接按钮信号
    QObject::connect(
        btnAdd, &QPushButton::clicked, listWidget, [ listWidget ]() { listWidget->addItem(QObject::tr("新项目")); });
    QObject::connect(btnRemove, &QPushButton::clicked, listWidget, [ listWidget ]() {
        QList< QListWidgetItem* > selected = listWidget->selectedItems();
        for (QListWidgetItem* item : selected) {
            listWidget->takeItem(listWidget->row(item));
            delete item;
        }
    });

    return container;
}

/**
 * @brief 创建文件路径类型编辑器（DAFilePathEditWidget）
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return DAFilePathEditWidget 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createFileEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc                = toParamDef(param);
    DAFilePathEditWidget* fileEdit = new DAFilePathEditWidget(parent);
    // 设置默认值
    QString defaultVal = desc.defaultValueToString();
    if (!defaultVal.isEmpty()) {
        fileEdit->setFilePath(defaultVal);
    }
    // 设置文件过滤器
    QString filterVal = desc.getFilterProperty();
    if (!filterVal.isEmpty()) {
        fileEdit->setNameFilter(filterVal);
    }
    return fileEdit;
}

/**
 * @brief 创建文件夹路径类型编辑器（DAFilePathEditWidget，目录模式）
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return DAFilePathEditWidget 指针（已切换为目录模式）
 */
QWidget* DAParamTypeRegistry::PrivateData::createFolderEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc                  = toParamDef(param);
    DAFilePathEditWidget* folderEdit = new DAFilePathEditWidget(parent);

    // 找到内部工具按钮，断开原始连接，重连目录选择逻辑
    QToolButton* toolBtn = folderEdit->findChild< QToolButton* >();
    if (toolBtn) {
        // 断开 DAFilePathEditWidget 内部所有到 toolBtn clicked 的连接
        toolBtn->disconnect(folderEdit);
        // 重新连接：目录模式文件对话框
        QObject::connect(toolBtn, &QToolButton::clicked, folderEdit, [ folderEdit ]() {
            QFileDialog fileDialog;
            fileDialog.setFileMode(QFileDialog::Directory);
            fileDialog.setOption(QFileDialog::ShowDirsOnly, true);
            if (fileDialog.exec()) {
                auto files = fileDialog.selectedFiles();
                if (!files.isEmpty()) {
                    QString p = files.back();
                    folderEdit->setFilePath(p);
                }
            }
        });
    }

    // 设置默认值
    QString defaultVal = desc.defaultValueToString();
    if (!defaultVal.isEmpty()) {
        folderEdit->setFilePath(defaultVal);
    }
    return folderEdit;
}

/**
 * @brief 创建颜色类型编辑器（DAColorPickerButton）
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return DAColorPickerButton 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createColorEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc               = toParamDef(param);
    DAColorPickerButton* colorBtn = new DAColorPickerButton(parent);
    // 设置默认颜色
    QString defaultVal = desc.defaultValueToString();
    if (!defaultVal.isEmpty()) {
        QColor c(defaultVal);
        if (c.isValid()) {
            colorBtn->setColor(c);
        }
    }
    return colorBtn;
}

/**
 * @brief 创建字体类型编辑器（DAFontEditPannelWidget）
 *
 * 支持两种默认值格式：
 * - 字典（QVariantMap）：{"family":str, "size":int, "bold":bool, "italic":bool, "color":str}
 * - 字符串：QFont::toString() 格式（向后兼容）
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return DAFontEditPannelWidget 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createFontEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc                  = toParamDef(param);
    DAFontEditPannelWidget* fontEdit = new DAFontEditPannelWidget(parent);
    // 字典格式默认值：{"family", "size", "bold", "italic", "color"}
    if (desc.defaultValue.canConvert< QVariantMap >()) {
        QVariantMap fontMap = desc.defaultValue.toMap();
        QFont f;
        f.setFamily(fontMap.value("family").toString());
        f.setPointSize(fontMap.value("size", 9).toInt());
        f.setBold(fontMap.value("bold").toBool());
        f.setItalic(fontMap.value("italic").toBool());
        fontEdit->setCurrentFont(f);
        QColor c(fontMap.value("color").toString());
        if (c.isValid()) {
            fontEdit->setCurrentFontColor(c);
        }
    } else {
        // 向后兼容：QFont::toString() 字符串格式
        QString defaultVal = desc.defaultValueToString();
        if (!defaultVal.isEmpty()) {
            QFont f;
            if (f.fromString(defaultVal)) {
                fontEdit->setCurrentFont(f);
            }
        }
    }
    return fontEdit;
}

/**
 * @brief 创建代码类型编辑器（QPlainTextEdit）
 *
 * 固定等宽字体，默认高度 100px。
 * 若设置 height 扩展属性且为正值，覆盖默认高度。
 *
 * @param[in] param 参数代理
 * @param[in] parent 父控件
 * @return QPlainTextEdit 指针
 */
QWidget* DAParamTypeRegistry::PrivateData::createCodeEditor(const DAPyNodeParameter& param, QWidget* parent)
{
    DAParamDef desc          = toParamDef(param);
    QPlainTextEdit* codeEdit = new QPlainTextEdit(parent);
    // 设置等宽字体
    QFont monoFont("Monospace", 10);
    codeEdit->setFont(monoFont);
    // 高度：默认 100，可被 height 属性覆盖
    bool heightOk = false;
    int h         = desc.getHeightProperty(&heightOk);
    if (!heightOk || h <= 0) {
        h = 100;
    }
    codeEdit->setFixedHeight(h);
    // 设置默认值
    QString defaultVal = desc.defaultValueToString();
    if (!defaultVal.isEmpty()) {
        codeEdit->setPlainText(defaultVal);
    }
    return codeEdit;
}

}  // namespace DA
