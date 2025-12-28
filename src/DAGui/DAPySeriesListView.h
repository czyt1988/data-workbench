#ifndef DAPYSERIESLISTVIEW_H
#define DAPYSERIESLISTVIEW_H
#include "DAGuiAPI.h"
#include <QListView>
#include <QStringList>
#include "DAData.h"
class QDragEnterEvent;
class QDropEvent;
class QDragMoveEvent;
namespace DA
{
class DAMimeDataForMultDataSeries;


class DAGUI_API DAPySeriesListView : public QListView
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPySeriesListView)
public:
    enum AcceptMode
    {
        AcceptOneSeries,               ///< 仅仅接受一个dataframe的一个series
        AcceptOneDataframeMultSeries,  ///< 接受1个dataframe以及它下面的多个series
        AcceptMultDataframeMultSeries  ///< 接受多个dataframe的多个series
    };

public:
    DAPySeriesListView(QWidget* par = nullptr);
    ~DAPySeriesListView();
    // 设置数据管理器
    void setDataManager(DADataManager* dataMgr);
    DADataManager* getDataManager() const;
    // 模式
    void setAcceptMode(AcceptMode mode);
    AcceptMode getAcceptMode() const;

    // 添加序列
    void addSeries(const DAData& dataframeData, const QString& name);
    void removeSeries(const DAData& dataframeData, const QString& name);
    // 移除当前选中
    void removeCurrentSelect();

    // 获取数据
    QList< QPair< DAData, QStringList > > getSeries() const;

Q_SIGNALS:
    void seriesChanged();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    bool acceptMimeData(const QMimeData* mime) const;
    bool acceptSeries(const DAMimeDataForMultDataSeries* mime) const;
};
}  // end namespace DA


#endif  // DAPYSERIESLISTVIEW_H
