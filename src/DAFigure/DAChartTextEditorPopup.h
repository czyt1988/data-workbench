#ifndef DACHARTTEXTEDITORPOPUP_H
#define DACHARTTEXTEDITORPOPUP_H
#include "DAFigureAPI.h"
#include <QWidget>
#include <QString>

class QKeyEvent;
class QHideEvent;

namespace DA
{

/**
 * @brief 富文本编辑弹窗（DAChartTextMarker 的实时编辑壳）
 *
 * 内容由 DARichTextEditWidget 提供（工具栏 + 富文本编辑区）；本类只负责
 * Qt::Popup 定位、OK/Cancel 按钮与关闭语义：
 * - 在锚点附近弹出，自动避开锚点且不超出屏幕
 * - 点击弹窗外部或「OK」= 确认（accepted）
 * - Esc 或「Cancel」= 取消（rejected）
 * - 文本或格式变化时发射 textChanged(html)，调用方据此实时刷新绘图
 */
class DAFIGURE_API DAChartTextEditorPopup : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartTextEditorPopup)
public:
    explicit DAChartTextEditorPopup(QWidget* parent = nullptr);
    ~DAChartTextEditorPopup();

    // 设置初始富文本内容（HTML片段，不会触发textChanged）
    void setHtml(const QString& html);
    // 获取富文本内容（提取body内层HTML片段）
    QString toHtml() const;
    // 获取纯文本内容
    QString toPlainText() const;

    // 在锚点（全局坐标）附近弹出，优先放在锚点左下，自动避开锚点并保证完整可见
    void popupAt(const QPoint& globalAnchor);

Q_SIGNALS:
    /**
     * @brief 文本或格式发生变化
     * @param html 当前的富文本HTML片段
     */
    void textChanged(const QString& html);
    /**
     * @brief 确认编辑（点击外部或OK）
     */
    void accepted();
    /**
     * @brief 取消编辑（Esc或Cancel）
     */
    void rejected();

protected:
    virtual void keyPressEvent(QKeyEvent* e) override;
    virtual void hideEvent(QHideEvent* e) override;
    virtual bool eventFilter(QObject* obj, QEvent* e) override;

private Q_SLOTS:
    void onAccept();
    void onReject();

private:
    // 构建界面与信号连接
    void buildUI();
    // 显示弹窗并聚焦编辑区
    void execShow();
};

}  // namespace DA

#endif  // DACHARTTEXTEDITORPOPUP_H
