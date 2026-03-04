#ifndef DAABSTRACTCHARTEDITOR_H
#define DAABSTRACTCHARTEDITOR_H
#include "DAFigureAPI.h"
#include <QObject>
#include "qwt_plot.h"
namespace DA
{

/**
 * @brief 绘图编辑器
 *
 * 此编辑器会屏蔽长按空格的keyPressEvent和keyReleaseEvent
 */
class DAFIGURE_API DAAbstractChartEditor : public QObject
{
    Q_OBJECT
    friend class DAFigureChartEditorWidgetOverlay;

public:
    DAAbstractChartEditor(QwtPlot* parent);
    virtual ~DAAbstractChartEditor();
    enum RTTI
    {
        RTTIRectRegionSelectEditor,     ///< 用于给图标添加矩形选框
        RTTIEllipseRegionSelectEditor,  ///< 用于给图标添加椭圆选框
        RTTIPolygonRegionSelectEditor,  ///< 用于给图标添加多边形选框
        RTTIUserDefine
    };

    const QwtPlot* plot() const;
    QwtPlot* plot();
    // 是否启用编辑器
    virtual void setEnabled(bool on);
    virtual bool isEnabled() const;
    virtual int rtti() const = 0;
    // 取消编辑
    virtual bool cancel();
    // 设置屏蔽按键，屏蔽按键将不触发keyPressEvent和keyReleaseEvent
    void setBlockKeys(const QList< int >& keys);
    const QList< int >& getBlockKeys() const;
Q_SIGNALS:
    /**
     * @brief 开始编辑
     */
    void beginEdit();

    /**
     * @brief 结束编辑
     * @param isCancel
     */
    void finishedEdit(bool isCancel);

protected:
    virtual bool eventFilter(QObject* object, QEvent* event);
    virtual bool mousePressEvent(const QMouseEvent* e);
    virtual bool mouseMoveEvent(const QMouseEvent* e);
    virtual bool mouseReleaseEvent(const QMouseEvent* e);
    virtual bool keyPressEvent(const QKeyEvent* e);
    virtual bool keyReleaseEvent(const QKeyEvent* e);

private:
    bool m_isEnable;           ///< 是否生效
    QList< int > m_blockKeys;  ///< 屏蔽按键
};
}  // End Of Namespace DA
#endif  // DAABSTRACTPLOTEDITOR_H
