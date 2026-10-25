#ifndef DARICHTEXTEDITWIDGET_H
#define DARICHTEXTEDITWIDGET_H

#include "DAFigureAPI.h"
#include <QColor>
#include <QFont>
#include <QString>
#include <QTextCharFormat>
#include <QWidget>

class QTextEdit;
class QToolButton;

namespace DA
{
class DAFontEditPannelWidget;
class DAColorPickerButton;

/**
 * @brief 富文本编辑控件（工具栏 + QTextEdit）
 *
 * 工具栏：字体/字号/粗斜体下划线/文字颜色（DAFontEditPannelWidget SingleRowLayout）、
 * 文字背景色、删除线/上标/下标/清除格式。
 *
 * 本控件只负责编辑与格式化，不拥有 OK/Cancel；弹窗形态由调用方套壳
 * （见 DAChartTextEditorPopup）。表格批注升级富文本等第二消费者出现时可整体迁入 DAWidgets。
 */
class DAFIGURE_API DARichTextEditWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DARichTextEditWidget)
public:
    explicit DARichTextEditWidget(QWidget* parent = nullptr);
    ~DARichTextEditWidget();

    // 设置初始富文本内容（HTML片段，不触发 textChanged）
    void setHtml(const QString& html);
    // 获取富文本内容（提取body内层HTML片段）
    QString toHtml() const;
    // 获取纯文本内容
    QString toPlainText() const;
    // 内部编辑区（弹窗需要安装 Esc 过滤器 / 设置焦点）
    QTextEdit* textEdit() const;

Q_SIGNALS:
    /**
     * @brief 文本或格式发生变化
     * @param html 当前的富文本HTML片段
     */
    void textChanged(const QString& html);

private Q_SLOTS:
    void onFontChanged(const QFont& f);
    void onFontColorChanged(const QColor& c);
    void onBackgroundColorChanged(const QColor& c);
    void onTextEditCharFormatChanged(const QTextCharFormat& fmt);
    void onTextChanged();

private:
    // 应用字符格式到当前选中段（无选中时作用于后续输入）
    void mergeCharFormat(const QTextCharFormat& fmt);
    void buildUI();
};

}  // namespace DA

#endif  // DARICHTEXTEDITWIDGET_H
