#ifndef DATAFRAMEDATASEARCHDIALOG_H
#define DATAFRAMEDATASEARCHDIALOG_H

#include <QDialog>
#include "pandas/DAPyDataFrame.h"
namespace Ui
{
class DataFrameDataSearchDialog;
}

namespace DA
{
class DADataTableView;
class DAPyScripts;
}

class DataFrameDataSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataFrameDataSearchDialog(DA::DAPyScripts* script, QWidget* parent = nullptr);
    ~DataFrameDataSearchDialog();
    // 获取查找内容
    QString getSearchText() const;
    // 获取内容坐标
    QList< QPair< int, int > > getItemCoor() const;

    DA::DADataTableView* getDataTableView() const;
    void setDataTableView(DA::DADataDataTableView* v);
    // 搜索
    void searchData();
private slots:
    void onPushButtonNextClicked();
    void onLineEditTextChanged(const QString& t);

private:
    Ui::DADialogDataFrameDataSearch* ui;
    DA::DAPyScripts* mPyScript { nullptr };
    DA::DADataTableView* mDataTableView { nullptr };
    QList< QPair< int, int > > mMatches {};
    bool mIsNeedResearch { true };  ///< 需要重新搜索，这个在重新设置了dataframe后触发
    int mIndex { -1 };              ///< -1代表全新的搜索，需要重新匹配一下mMatches
};

#endif  // DADIALOGDATAFRAMEDATASEARCH_H
