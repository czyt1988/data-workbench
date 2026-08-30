#ifndef DACHARTTEXTMARKEREDITOR_H
#define DACHARTTEXTMARKEREDITOR_H
#include "DAAbstractChartEditor.h"
#include <QPointer>

class QTimer;

namespace DA
{
class DAChartTextMarker;
class DAChartTextEditorPopup;

/**
 * @brief 文本标注编辑器
 *
 * 单击创建型编辑器，交互流程：
 * 1. 鼠标按下：在点击位置（数据坐标）创建 DAChartTextMarker 并渲染占位文本
 * 2. 鼠标释放：延迟（事件循环下一轮）弹出 DAChartTextEditorPopup，
 *    避免与 overlay 的 grabMouse 冲突
 * 3. 编辑期间：popup 的 textChanged 实时同步到 marker 的 label
 * 4. popup accepted（含点击外部）：保留标注，finishedEdit(false) 走 undo 入栈
 * 5. popup rejected / Esc：删除标注，finishedEdit(true)
 */
class DAFIGURE_API DAChartTextMarkerEditor : public DAAbstractChartEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartTextMarkerEditor)
public:
    explicit DAChartTextMarkerEditor(QwtPlot* parent = nullptr);
    virtual ~DAChartTextMarkerEditor();

    virtual int rtti() const override;
    virtual QwtPlotItem* takeItem() override;
    virtual bool cancel() override;

protected:
    virtual bool mousePressEvent(const QMouseEvent* e) override;
    virtual bool mouseReleaseEvent(const QMouseEvent* e) override;

private Q_SLOTS:
    void onPopupTextChagned(const QString& html);
    void onPopupAccepted();
    void onPopupRejected();

private:
    void finishEdit(bool isCancel);
    void releaseMarker();
};

}  // namespace DA

#endif  // DACHARTTEXTMARKEREDITOR_H
