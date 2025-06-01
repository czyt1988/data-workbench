#ifndef DADIALOGDATAFRAMEDATASEARCH_H
#define DADIALOGDATAFRAMEDATASEARCH_H

#include <QDialog>

namespace Ui
{
class DADialogDataFrameDataSearch;
}

namespace DA
{
class DADialogDataFrameDataSearch : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameDataSearch(QWidget* parent = nullptr);
	~DADialogDataFrameDataSearch();
    // 获取查找内容
    QString getFindItem() const;

    //获取内容坐标
    void setItemCoor(const QList< QPair< int, int > >& matches);
    QList< QPair< int, int > > getItemCoor() const;

    void setCoorIndex(const int idx);
    int getCoorIndex() const;

private slots:
    void on_pushButtonNext_clicked();

private:
	Ui::DADialogDataFrameDataSearch* ui;
    QList< QPair< int, int > > mMatches{};
    int mIndex{ 0 };
};
}

#endif  // DADIALOGDATAFRAMEDATASEARCH_H
