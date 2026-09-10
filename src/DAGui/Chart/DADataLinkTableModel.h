#ifndef DADATALINKTABLEMODEL_H
#define DADATALINKTABLEMODEL_H
#include <QStandardItemModel>
#include <QColor>
#include <QList>
#include "DAGuiAPI.h"
class QwtPlot;
class QwtPlotItem;

namespace DA
{
/**
 * @brief 数据联动表的树形数据模型
 *
 * 行为两层树：绘图节点行（加粗）+ 曲线子行；
 * 第 0 列固定为曲线名，之后每列对应一组同名探针（一次探针模式点击创建的跨子图探针组）
 */
class DAGUI_API DADataLinkTableModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum CustomRoles
    {
        RolePlotPointer = Qt::UserRole + 1,
        RolePlotItem    = Qt::UserRole + 2,
        RoleIsPlotNode  = Qt::UserRole + 3,
        RoleCurveColor  = Qt::UserRole + 4,
        RoleRowHidden   = Qt::UserRole + 5
    };

    explicit DADataLinkTableModel(QObject* parent = nullptr);
    ~DADataLinkTableModel() override;

    // 添加一列（对应一组探针），headerText 一般为 x 值的格式化文本，letter 为探针名
    int addProbeColumn(double xValue, const QString& headerText = QString(), const QString& letter = QString());
    void removeProbeColumn(int col);
    void renameColumn(int col, const QString& name);
    // 列头显示文本
    QString columnLetter(int col) const;

    QStandardItem* ensurePlotRow(QwtPlot* plot, const QString& title);
    QStandardItem* ensureCurveRow(QStandardItem* plotRow, QwtPlotItem* curveItem,
                                  const QString& title, const QColor& color);
    void setCellData(QStandardItem* curveRow, int col, const QString& value);
    void removeCurveRow(QwtPlotItem* item);
    void updateVisibility();
    double columnXValue(int col) const;

private:
    QStandardItem* findPlotRow(QwtPlot* plot) const;
    QStandardItem* findCurveRowByTitle(QStandardItem* plotRow, const QString& title) const;
    static QString formatXValue(double x);
    QList< double > m_columnXValues;
};
}  // namespace DA
#endif  // DADATALINKTABLEMODEL_H
