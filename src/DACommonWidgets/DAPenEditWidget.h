#ifndef DAPENEDITWIDGET_H
#define DAPENEDITWIDGET_H
#include "DACommonWidgetsAPI.h"
#include <QWidget>
#include <QPen>

namespace DA
{
/**
 * @brief 画笔编辑窗口
 * 颜色按钮+线型选择+线宽
 * ┏┓┍━━━━┓┍━━┓
 * ┗┛┗━━━━┛┗━━┛
 */
class DACOMMONWIDGETS_API DAPenEditWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPenEditWidget)
public:
    explicit DAPenEditWidget(QWidget* parent = nullptr);
    explicit DAPenEditWidget(const QPen& p, QWidget* parent = nullptr);
    ~DAPenEditWidget();
    void retranslateUi();
    //获取当前的画笔
    QPen getCurrentPen() const;
public slots:
    //设置画笔,设置画笔会触发penChanged信号
    void setCurrentPen(const QPen& p);
signals:
    /**
     * @brief 画笔改变信号
     * @param p
     */
    void penChanged(const QPen& p);

protected slots:
    void onColorChanged(const QColor& c);
    void onPenWidthValueChanged(int w);
    void onPenStyleChanged(Qt::PenStyle s);

private:
    void init();
};
}  // namespace DA
#endif  // DAPENEDITWIDGET_H
