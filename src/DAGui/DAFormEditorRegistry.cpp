#include "DAFormEditorRegistry.h"
#include "DAGlobals.h"
// 编辑器控件
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>
// 项目内编辑器控件
#include "DAColorPickerButton.h"
#include "DAFilePathEditWidget.h"
#include "DAFontEditPannelWidget.h"

namespace DA
{
namespace
{
/**
 * @brief 解析编辑器高度
 *
 * 优先使用字段定义的 height 成员（schema 层），其次回退到 attributes["height"]（程序化构造），
 * 都未设置时返回 @p defaultHeight。
 */
int resolveHeight(const DAFormFieldDef& field, int defaultHeight)
{
    if (field.height > 0) {
        return field.height;
    }
    QVariant h = field.attributes.value("height");
    bool ok    = false;
    int v      = h.toInt(&ok);
    if (ok && v > 0) {
        return v;
    }
    return defaultHeight;
}

//@{
//! @name str 类型编辑器
/// 占位提示文本：优先使用字段定义的 placeholder，回退到 description
inline QString resolvePlaceholder(const DAFormFieldDef& field)
{
    return field.placeholder.isEmpty() ? field.description : field.placeholder;
}

QWidget* createStrEditor(const DAFormFieldDef& field, QWidget* parent)
{
    if (field.layout.compare("below", Qt::CaseInsensitive) == 0) {
        QPlainTextEdit* edit = new QPlainTextEdit(parent);
        QString ph = resolvePlaceholder(field);
        if (!ph.isEmpty()) {
            edit->setPlaceholderText(ph);
        }
        edit->setMinimumHeight(resolveHeight(field, 80));
        QString str = field.defaultValue.toString();
        if (!str.isEmpty()) {
            edit->setPlainText(str);
        }
        return edit;
    }
    QLineEdit* edit = new QLineEdit(parent);
    QString ph = resolvePlaceholder(field);
    if (!ph.isEmpty()) {
        edit->setPlaceholderText(ph);
    }
    QString str = field.defaultValue.toString();
    if (!str.isEmpty()) {
        edit->setText(str);
    }
    if (field.readOnly) {
        edit->setReadOnly(true);
    }
    return edit;
}

QVariant readStr(QWidget* editor)
{
    if (auto* le = qobject_cast< QLineEdit* >(editor)) {
        return le->text();
    }
    if (auto* pe = qobject_cast< QPlainTextEdit* >(editor)) {
        return pe->toPlainText();
    }
    return {};
}

void writeStr(QWidget* editor, const QVariant& value)
{
    QString s = value.toString();
    if (auto* le = qobject_cast< QLineEdit* >(editor)) {
        QSignalBlocker blocker(le);
        le->setText(s);
    } else if (auto* pe = qobject_cast< QPlainTextEdit* >(editor)) {
        QSignalBlocker blocker(pe);
        pe->setPlainText(s);
    }
}

void connectStr(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (auto* le = qobject_cast< QLineEdit* >(editor)) {
        QObject::connect(le, &QLineEdit::textEdited, context, [ slot ](const QString&) { slot(); });
    } else if (auto* pe = qobject_cast< QPlainTextEdit* >(editor)) {
        QObject::connect(pe, &QPlainTextEdit::textChanged, context, [ slot ]() { slot(); });
    }
}
//@}

//@{
//! @name int 类型编辑器
QWidget* createIntEditor(const DAFormFieldDef& field, QWidget* parent)
{
    QSpinBox* spin = new QSpinBox(parent);
    spin->setRange(-9999, 9999);
    // 第三方 QSS 可能将上下箭头渲染为左右按钮，挤占数字显示区；设最小宽度保证内容可见
    spin->setMinimumWidth(120);
    bool ok = false;
    int minV = field.attributes.value("min").toInt(&ok);
    if (ok) {
        spin->setMinimum(minV);
    }
    int maxV = field.attributes.value("max").toInt(&ok);
    if (ok) {
        spin->setMaximum(maxV);
    }
    int step = field.attributes.value("step").toInt(&ok);
    if (ok) {
        spin->setSingleStep(step);
    }
    int def = field.defaultValue.toInt(&ok);
    if (ok) {
        spin->setValue(def);
    }
    return spin;
}

QVariant readInt(QWidget* editor)
{
    if (auto* spin = qobject_cast< QSpinBox* >(editor)) {
        return spin->value();
    }
    return {};
}

void writeInt(QWidget* editor, const QVariant& value)
{
    if (auto* spin = qobject_cast< QSpinBox* >(editor)) {
        bool ok    = false;
        int v      = value.toInt(&ok);
        if (ok) {
            QSignalBlocker b(spin);
            spin->setValue(v);
        }
    }
}

void connectInt(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (auto* spin = qobject_cast< QSpinBox* >(editor)) {
        QObject::connect(spin, QOverload< int >::of(&QSpinBox::valueChanged), context, [ slot ](int) { slot(); });
    }
}
//@}

//@{
//! @name float 类型编辑器
QWidget* createFloatEditor(const DAFormFieldDef& field, QWidget* parent)
{
    QDoubleSpinBox* spin = new QDoubleSpinBox(parent);
    spin->setRange(-9999.0, 9999.0);
    spin->setDecimals(2);
    // 第三方 QSS 可能将上下箭头渲染为左右按钮，挤占数字显示区；设最小宽度保证内容可见
    spin->setMinimumWidth(140);
    bool ok = false;
    double minV = field.attributes.value("min").toDouble(&ok);
    if (ok) {
        spin->setMinimum(minV);
    }
    double maxV = field.attributes.value("max").toDouble(&ok);
    if (ok) {
        spin->setMaximum(maxV);
    }
    int dec = field.attributes.value("decimals").toInt(&ok);
    if (ok) {
        spin->setDecimals(dec);
    }
    double step = field.attributes.value("step").toDouble(&ok);
    if (ok) {
        spin->setSingleStep(step);
    }
    double def = field.defaultValue.toDouble(&ok);
    if (ok) {
        spin->setValue(def);
    }
    return spin;
}

QVariant readFloat(QWidget* editor)
{
    if (auto* spin = qobject_cast< QDoubleSpinBox* >(editor)) {
        return spin->value();
    }
    return {};
}

void writeFloat(QWidget* editor, const QVariant& value)
{
    if (auto* spin = qobject_cast< QDoubleSpinBox* >(editor)) {
        bool ok    = false;
        double v   = value.toDouble(&ok);
        if (ok) {
            QSignalBlocker b(spin);
            spin->setValue(v);
        }
    }
}

void connectFloat(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (auto* spin = qobject_cast< QDoubleSpinBox* >(editor)) {
        QObject::connect(spin, QOverload< double >::of(&QDoubleSpinBox::valueChanged), context, [ slot ](double) { slot(); });
    }
}
//@}

//@{
//! @name bool 类型编辑器
QWidget* createBoolEditor(const DAFormFieldDef& field, QWidget* parent)
{
    QCheckBox* cb = new QCheckBox(parent);
    cb->setChecked(field.defaultValue.toBool());
    return cb;
}

QVariant readBool(QWidget* editor)
{
    if (auto* cb = qobject_cast< QCheckBox* >(editor)) {
        return cb->isChecked();
    }
    return {};
}

void writeBool(QWidget* editor, const QVariant& value)
{
    if (auto* cb = qobject_cast< QCheckBox* >(editor)) {
        QSignalBlocker b(cb);
        cb->setChecked(value.toBool());
    }
}

void connectBool(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (auto* cb = qobject_cast< QCheckBox* >(editor)) {
        QObject::connect(cb, &QCheckBox::toggled, context, [ slot ](bool) { slot(); });
    }
}
//@}

//@{
//! @name enum 类型编辑器
/// 填充组合框：优先 options，回退 attributes["enum"]（QStringList 或 QList<QPair<QString,int>>）
void populateEnumCombo(QComboBox* combo, const DAFormFieldDef& field)
{
    if (!field.options.isEmpty()) {
        for (const DAFormOption& opt : field.options) {
            QString text = opt.value.isValid() && !opt.value.isNull() ? opt.value.toString() : opt.label;
            if (text.isEmpty()) {
                text = opt.label;
            }
            combo->addItem(text, opt.value);
        }
        return;
    }
    QVariant enumAttr = field.attributes.value("enum");
    if (enumAttr.canConvert< QStringList >()) {
        QStringList sl = enumAttr.value< QStringList >();
        for (const QString& s : std::as_const(sl)) {
            combo->addItem(s, s);
        }
    } else if (enumAttr.canConvert< QList< QPair< QString, int > > >()) {
        QList< QPair< QString, int > > pl = enumAttr.value< QList< QPair< QString, int > > >();
        for (const QPair< QString, int >& p : std::as_const(pl)) {
            combo->addItem(p.first, p.second);
        }
    }
}

QWidget* createEnumEditor(const DAFormFieldDef& field, QWidget* parent)
{
    QComboBox* combo = new QComboBox(parent);
    combo->setEditable(false);
    populateEnumCombo(combo, field);
    // 默认值：优先按 UserRole 数据匹配，其次按文本匹配
    if (field.defaultValue.isValid()) {
        QString defStr = field.defaultValue.toString();
        int idx        = combo->findData(field.defaultValue, Qt::UserRole);
        if (idx < 0) {
            idx = combo->findText(defStr);
        }
        if (idx >= 0) {
            combo->setCurrentIndex(idx);
        }
    }
    return combo;
}

QVariant readEnum(QWidget* editor)
{
    auto* combo = qobject_cast< QComboBox* >(editor);
    if (!combo) {
        return {};
    }
    QVariant data = combo->currentData(Qt::UserRole);
    if (data.isValid() && !data.isNull()) {
        return data;
    }
    return combo->currentText();
}

void writeEnum(QWidget* editor, const QVariant& value)
{
    auto* combo = qobject_cast< QComboBox* >(editor);
    if (!combo) {
        return;
    }
    int idx = combo->findData(value, Qt::UserRole);
    if (idx < 0) {
        idx = combo->findText(value.toString());
    }
    if (idx >= 0) {
        QSignalBlocker b(combo);
        combo->setCurrentIndex(idx);
    }
}

void connectEnum(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    auto* combo = qobject_cast< QComboBox* >(editor);
    if (!combo) {
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), context, [ slot ](int) { slot(); });
#else
    QObject::connect(combo, &QComboBox::currentIndexChanged, context, [ slot ](int) { slot(); });
#endif
}
//@}

//@{
//! @name list 类型编辑器（QListWidget + 添加/删除按钮）
QWidget* createListEditor(const DAFormFieldDef& field, QWidget* parent)
{
    QWidget* container  = new QWidget(parent);
    QHBoxLayout* hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);

    QListWidget* listWidget = new QListWidget(container);
    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    hLayout->addWidget(listWidget, 1);

    QVBoxLayout* btnLayout = new QVBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(2);

    QPushButton* btnAdd    = new QPushButton(QObject::tr("Add"), container);  // cn:添加
    QPushButton* btnRemove = new QPushButton(QObject::tr("Remove"), container);  // cn:删除
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnRemove);
    btnLayout->addStretch();
    hLayout->addLayout(btnLayout);

    // 预填充：优先 options，其次 defaultValue（QStringList）
    if (!field.options.isEmpty()) {
        for (const DAFormOption& opt : field.options) {
            QString text = opt.value.isValid() && !opt.value.isNull() ? opt.value.toString() : opt.label;
            if (text.isEmpty()) {
                text = opt.label;
            }
            listWidget->addItem(text);
        }
    } else if (field.defaultValue.canConvert< QStringList >()) {
        QStringList sl = field.defaultValue.value< QStringList >();
        for (const QString& s : std::as_const(sl)) {
            listWidget->addItem(s);
        }
    }

    QObject::connect(btnAdd, &QPushButton::clicked, listWidget, [ listWidget ]() {
        listWidget->addItem(QObject::tr("New Item"));  // cn:新项目
    });
    QObject::connect(btnRemove, &QPushButton::clicked, listWidget, [ listWidget ]() {
        QList< QListWidgetItem* > selected = listWidget->selectedItems();
        for (QListWidgetItem* item : std::as_const(selected)) {
            listWidget->takeItem(listWidget->row(item));
            delete item;
        }
    });

    return container;
}

QVariant readList(QWidget* editor)
{
    QListWidget* listWidget = editor ? editor->findChild< QListWidget* >() : nullptr;
    if (!listWidget) {
        return {};
    }
    QStringList sl;
    for (int i = 0; i < listWidget->count(); ++i) {
        if (auto* it = listWidget->item(i)) {
            sl.append(it->text());
        }
    }
    return sl;
}

void writeList(QWidget* editor, const QVariant& value)
{
    QListWidget* listWidget = editor ? editor->findChild< QListWidget* >() : nullptr;
    if (!listWidget) {
        return;
    }
    QSignalBlocker b(listWidget);
    listWidget->clear();
    QStringList sl = value.toStringList();
    for (const QString& s : std::as_const(sl)) {
        listWidget->addItem(s);
    }
}

void connectList(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (!editor) {
        return;
    }
    QList< QPushButton* > buttons = editor->findChildren< QPushButton* >();
    for (QPushButton* btn : std::as_const(buttons)) {
        QObject::connect(btn, &QPushButton::clicked, context, [ slot ]() { slot(); });
    }
}
//@}

//@{
//! @name file 类型编辑器
QWidget* createFileEditor(const DAFormFieldDef& field, QWidget* parent)
{
    DAFilePathEditWidget* edit = new DAFilePathEditWidget(parent);
    QString filter = field.attributes.value("filter").toString();
    if (!filter.isEmpty()) {
        edit->setNameFilter(filter);
    }
    QString def = field.defaultValue.toString();
    if (!def.isEmpty()) {
        edit->setFilePath(def);
    }
    return edit;
}

QVariant readFile(QWidget* editor)
{
    if (auto* edit = qobject_cast< DAFilePathEditWidget* >(editor)) {
        return edit->getFilePath();
    }
    return {};
}

void writeFile(QWidget* editor, const QVariant& value)
{
    if (auto* edit = qobject_cast< DAFilePathEditWidget* >(editor)) {
        QSignalBlocker b(edit);
        edit->setFilePath(value.toString());
    }
}

void connectFile(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (auto* edit = qobject_cast< DAFilePathEditWidget* >(editor)) {
        QObject::connect(edit, &DAFilePathEditWidget::selectedPath, context, [ slot ](const QString&) { slot(); });
    }
}
//@}

//@{
//! @name folder 类型编辑器（DAFilePathEditWidget 切换为目录模式）
QWidget* createFolderEditor(const DAFormFieldDef& field, QWidget* parent)
{
    DAFilePathEditWidget* edit = new DAFilePathEditWidget(parent);

    // 找到内部工具按钮，断开原始连接，重连为目录选择对话框
    QToolButton* toolBtn = edit->findChild< QToolButton* >();
    if (toolBtn) {
        // 仅断开 clicked 信号，避免影响 DAFilePathEditWidget 的其他内部连接
        QObject::disconnect(toolBtn, &QToolButton::clicked, edit, nullptr);
        QObject::connect(toolBtn, &QToolButton::clicked, edit, [ edit ]() {
            QFileDialog fileDialog;
            fileDialog.setFileMode(QFileDialog::Directory);
            fileDialog.setOption(QFileDialog::ShowDirsOnly, true);
            if (fileDialog.exec()) {
                auto files = fileDialog.selectedFiles();
                if (!files.isEmpty()) {
                    QString p = files.back();
                    edit->setFilePath(p);
                    Q_EMIT edit->selectedPath(p);   // 显式发射，触发 onFieldChanged
                }
            }
        });
    }

    QString def = field.defaultValue.toString();
    if (!def.isEmpty()) {
        edit->setFilePath(def);
    }
    return edit;
}
// folder 与 file 共用 read/write/connect
//@}

//@{
//! @name color 类型编辑器
QWidget* createColorEditor(const DAFormFieldDef& field, QWidget* parent)
{
    DAColorPickerButton* btn = new DAColorPickerButton(parent);
    QString def = field.defaultValue.toString();
    if (!def.isEmpty()) {
        QColor c(def);
        if (c.isValid()) {
            btn->setColor(c);
        }
    }
    return btn;
}

QVariant readColor(QWidget* editor)
{
    if (auto* btn = qobject_cast< DAColorPickerButton* >(editor)) {
        return btn->color().name();
    }
    return {};
}

void writeColor(QWidget* editor, const QVariant& value)
{
    if (auto* btn = qobject_cast< DAColorPickerButton* >(editor)) {
        QColor c(value.toString());
        if (c.isValid()) {
            QSignalBlocker b(btn);
            btn->setColor(c);
        }
    }
}

void connectColor(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (auto* btn = qobject_cast< DAColorPickerButton* >(editor)) {
        QObject::connect(btn, &DAColorPickerButton::colorChanged, context, [ slot ](const QColor&) { slot(); });
    }
}
//@}

//@{
//! @name font 类型编辑器
/// 从 QVariantMap 形式的默认值构造 QFont
QFont fontFromMap(const QVariantMap& m)
{
    QFont f;
    f.setFamily(m.value("family").toString());
    f.setPointSize(m.value("size", 9).toInt());
    f.setBold(m.value("bold").toBool());
    f.setItalic(m.value("italic").toBool());
    return f;
}

QWidget* createFontEditor(const DAFormFieldDef& field, QWidget* parent)
{
    DAFontEditPannelWidget* edit = new DAFontEditPannelWidget(parent);
    if (field.defaultValue.canConvert< QVariantMap >()) {
        QVariantMap m = field.defaultValue.toMap();
        edit->setCurrentFont(fontFromMap(m));
        QColor c(m.value("color").toString());
        if (c.isValid()) {
            edit->setCurrentFontColor(c);
        }
    } else {
        QString def = field.defaultValue.toString();
        if (!def.isEmpty()) {
            QFont f;
            if (f.fromString(def)) {
                edit->setCurrentFont(f);
            }
        }
    }
    return edit;
}

QVariant readFont(QWidget* editor)
{
    auto* edit = qobject_cast< DAFontEditPannelWidget* >(editor);
    if (!edit) {
        return {};
    }
    QFont f = edit->getCurrentFont();
    QVariantMap m;
    m[ "family" ] = f.family();
    m[ "size" ]   = f.pointSize() > 0 ? f.pointSize() : 9;
    m[ "bold" ]   = f.bold();
    m[ "italic" ] = f.italic();
    m[ "color" ]  = edit->getCurrentFontColor().name();
    return m;
}

void writeFont(QWidget* editor, const QVariant& value)
{
    auto* edit = qobject_cast< DAFontEditPannelWidget* >(editor);
    if (!edit) {
        return;
    }
    QSignalBlocker b(edit);
    if (value.canConvert< QVariantMap >()) {
        QVariantMap m = value.toMap();
        edit->setCurrentFont(fontFromMap(m));
        QColor c(m.value("color").toString());
        if (c.isValid()) {
            edit->setCurrentFontColor(c);
        }
    } else {
        QString s = value.toString();
        if (!s.isEmpty()) {
            QFont f;
            if (f.fromString(s)) {
                edit->setCurrentFont(f);
            }
        }
    }
}

void connectFont(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    auto* edit = qobject_cast< DAFontEditPannelWidget* >(editor);
    if (!edit) {
        return;
    }
    QObject::connect(edit, &DAFontEditPannelWidget::currentFontChanged, context, [ slot ](const QFont&) { slot(); });
    QObject::connect(edit, &DAFontEditPannelWidget::currentFontColorChanged, context, [ slot ](const QColor&) { slot(); });
}
//@}

//@{
//! @name code 类型编辑器（QPlainTextEdit，等宽字体）
QWidget* createCodeEditor(const DAFormFieldDef& field, QWidget* parent)
{
    QPlainTextEdit* edit = new QPlainTextEdit(parent);
    QFont monoFont("Monospace", 10);
    monoFont.setStyleHint(QFont::Monospace);
    edit->setFont(monoFont);
    edit->setFixedHeight(resolveHeight(field, 100));
    QString ph = resolvePlaceholder(field);
    if (!ph.isEmpty()) {
        edit->setPlaceholderText(ph);
    }
    QString def = field.defaultValue.toString();
    if (!def.isEmpty()) {
        edit->setPlainText(def);
    }
    if (field.readOnly) {
        edit->setReadOnly(true);
    }
    return edit;
}

QVariant readCode(QWidget* editor)
{
    if (auto* edit = qobject_cast< QPlainTextEdit* >(editor)) {
        return edit->toPlainText();
    }
    return {};
}

void writeCode(QWidget* editor, const QVariant& value)
{
    if (auto* edit = qobject_cast< QPlainTextEdit* >(editor)) {
        QSignalBlocker b(edit);
        edit->setPlainText(value.toString());
    }
}

void connectCode(QWidget* editor, QObject* context, const std::function< void() >& slot)
{
    if (auto* edit = qobject_cast< QPlainTextEdit* >(editor)) {
        QObject::connect(edit, &QPlainTextEdit::textChanged, context, [ slot ]() { slot(); });
    }
}
//@}

}  // namespace

class DAFormEditorRegistry::PrivateData
{
    DA_DECLARE_PUBLIC(DAFormEditorRegistry)
public:
    PrivateData(DAFormEditorRegistry* p) : q_ptr(p) {}
    QHash< QString, EditorAdapter > adapters;
};

/**
 * @brief 默认构造函数
 *
 * 不自动注册内置类型，需显式调用 registerDefaults()。
 */
DAFormEditorRegistry::DAFormEditorRegistry() : DA_PIMPL_CONSTRUCT
{
}

DAFormEditorRegistry::~DAFormEditorRegistry()
{
}

/**
 * @brief 注册全部 11 种内置默认类型
 *
 * 覆盖任何已注册的同名类型适配器。类型与编辑器控件映射：
 * str→QLineEdit/QPlainTextEdit, int→QSpinBox, float→QDoubleSpinBox, bool→QCheckBox,
 * enum→QComboBox, list→QListWidget+按钮, file/folder→DAFilePathEditWidget,
 * color→DAColorPickerButton, font→DAFontEditPannelWidget, code→QPlainTextEdit。
 */
void DAFormEditorRegistry::registerDefaults()
{
    DA_D(d);
    auto reg = [ &d ](const QString& type, EditorCreator c, EditorReader r, EditorWriter w, SignalConnector s) {
        EditorAdapter a;
        a.create              = std::move(c);
        a.read                = std::move(r);
        a.write               = std::move(w);
        a.connectValueChanged = std::move(s);
        d->adapters[ type ]   = std::move(a);
    };
    reg("str", createStrEditor, readStr, writeStr, connectStr);
    reg("int", createIntEditor, readInt, writeInt, connectInt);
    reg("float", createFloatEditor, readFloat, writeFloat, connectFloat);
    reg("bool", createBoolEditor, readBool, writeBool, connectBool);
    reg("enum", createEnumEditor, readEnum, writeEnum, connectEnum);
    reg("list", createListEditor, readList, writeList, connectList);
    reg("file", createFileEditor, readFile, writeFile, connectFile);
    reg("folder", createFolderEditor, readFile, writeFile, connectFile);
    reg("color", createColorEditor, readColor, writeColor, connectColor);
    reg("font", createFontEditor, readFont, writeFont, connectFont);
    reg("code", createCodeEditor, readCode, writeCode, connectCode);
}

/**
 * @brief 注册或覆盖指定类型的编辑器适配器
 *
 * @param[in] type 类型名称字符串
 * @param[in] adapter 编辑器适配器
 */
void DAFormEditorRegistry::registerType(const QString& type, const EditorAdapter& adapter)
{
    DA_D(d);
    d->adapters[ type ] = adapter;
}

/**
 * @brief 检查指定类型是否已注册
 */
bool DAFormEditorRegistry::hasType(const QString& type) const
{
    DA_DC(d);
    return d->adapters.contains(type);
}

/**
 * @brief 根据字段定义创建编辑器控件
 *
 * @param[in] field 字段定义
 * @param[in] parent 父控件
 * @return 创建的编辑器指针，未注册类型返回 nullptr
 */
QWidget* DAFormEditorRegistry::createEditor(const DAFormFieldDef& field, QWidget* parent) const
{
    DA_DC(d);
    auto it = d->adapters.find(field.type);
    if (it == d->adapters.end() || !it->create) {
        return nullptr;
    }
    return it->create(field, parent);
}

/**
 * @brief 从编辑器控件读取当前值
 *
 * @param[in] field 字段定义（用于定位适配器）
 * @param[in] editor 编辑器控件
 * @return 编辑器当前值，未注册类型返回无效 QVariant
 */
QVariant DAFormEditorRegistry::readValue(const DAFormFieldDef& field, QWidget* editor) const
{
    DA_DC(d);
    auto it = d->adapters.find(field.type);
    if (it == d->adapters.end() || !it->read) {
        return {};
    }
    return it->read(editor);
}

/**
 * @brief 将值写入编辑器控件
 *
 * 写入期间阻塞编辑器信号，避免触发反馈回路。
 *
 * @param[in] field 字段定义（用于定位适配器）
 * @param[in] editor 编辑器控件
 * @param[in] value 要写入的值
 */
void DAFormEditorRegistry::writeValue(const DAFormFieldDef& field, QWidget* editor, const QVariant& value) const
{
    DA_DC(d);
    auto it = d->adapters.find(field.type);
    if (it == d->adapters.end() || !it->write) {
        return;
    }
    it->write(editor, value);
}

/**
 * @brief 将编辑器原生值变化信号连接到指定槽
 *
 * @param[in] field 字段定义（用于定位适配器）
 * @param[in] editor 编辑器控件
 * @param[in] context 连接上下文（决定连接生命周期）
 * @param[in] slot 值变化时调用的回调
 */
void DAFormEditorRegistry::connectValueChanged(const DAFormFieldDef& field, QWidget* editor, QObject* context, const std::function< void() >& slot) const
{
    DA_DC(d);
    auto it = d->adapters.find(field.type);
    if (it == d->adapters.end() || !it->connectValueChanged) {
        return;
    }
    it->connectValueChanged(editor, context, slot);
}

}  // namespace DA
