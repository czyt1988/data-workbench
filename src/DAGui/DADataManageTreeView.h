#ifndef DADATAMANAGETREEVIEW_H
#define DADATAMANAGETREEVIEW_H
#include <QTreeView>
#include "DAGuiAPI.h"
#include "DAData.h"
class QMenu;
class QAction;

namespace DA
{
class DADataManagerTreeModel;
class DADataManagerTreeItem;
class DADataManager;
/**
 * @brief 变量树控件
 */
class DAGUI_API DADataManageTreeView : public QTreeView
{
    Q_OBJECT
public:
    DADataManageTreeView(QWidget* parent = nullptr);
    ~DADataManageTreeView();
    //设置datamager
    void setDataManager(DADataManager* mgr);
    DADataManager* getDataManager() const;
    //多语言翻译
    void retranslateUi();
    //添加数据文件夹
    DADataManagerTreeItem* addDataFolder(const QString& name = QString());
    //移除文件夹
    void removeCurrentSelectDataFloder();
    //获取当前选中的文件夹,如果没有选中，返回nullptr
    DADataManagerTreeItem* getCurrentSelectFolder() const;
    //获取选中的数据
    QList< DADataManagerTreeItem* > getCurrentSelectDatas() const;

protected:
    virtual void dragEnterEvent(QDragEnterEvent* ev) override;
    virtual void dragMoveEvent(QDragMoveEvent* ev) override;
    virtual void dropEvent(QDropEvent* ev) override;
    virtual void changeEvent(QEvent* e) override;

private slots:
    //鼠标右键
    void onCustomContextMenuRequested(const QPoint& pos);
    //
    void onActionAddDataFolderTriggered();
signals:
    // TODO 暂时未实现
    /**
     * @brief 变量双击
     * @param data
     */
    void dataDbClicked(const DA::DAData& data);

private:
    QAction* createAction(const char* objname, const char* iconpath);

private:
    DADataManagerTreeModel* _model;
    QMenu* _menu;
    QAction* actionAddDataFolder;  ///< 添加文件夹
};
}  // end of namespace DA
#endif  // DADATAMANAGETREEVIEW_H
