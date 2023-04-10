#ifndef DADATAMANAGETABLEVIEW_H
#define DADATAMANAGETABLEVIEW_H
#include <QtCore/qglobal.h>
#include <QTableView>
#include "DAGuiAPI.h"
#include "DAData.h"
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
namespace DA
{
class DADataManager;
class DADataManagerTableModel;

/**
 * @brief 用于显示数据列表的TableView
 */
class DAGUI_API DADataManageTableView : public QTableView
{
    Q_OBJECT
public:
    DADataManageTableView(QWidget* parent = nullptr);
    DADataManageTableView(DADataManager* dmgr, QWidget* parent = nullptr);
    //设置datamanager
    void setDataManager(DADataManager* dmgr);
    //获取一个选中的数据
    DAData getOneSelectData() const;
    //获取所有选中的数据
    QList< DAData > getSelectDatas() const;

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void startDrag(Qt::DropActions supportedActions) override;
signals:
    /**
     * @brief 变量双击
     * @param data
     */
    void dataDbClicked(const DA::DAData& data);
private slots:
    void onTableViewDoubleClicked(const QModelIndex& index);

private:
    void init();
};
}  // end of namespace DA
#endif  // DADATAMANAGETABLEVIEW_H
