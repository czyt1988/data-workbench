#ifndef DAPYSERIESLISTVIEW_H
#define DAPYSERIESLISTVIEW_H
#include "DAGuiAPI.h"
#include <QListView>
#include <QStringList>
#include <functional>
#include "DAData.h"
class QDragEnterEvent;
class QDropEvent;
class QDragMoveEvent;
namespace DA
{
class DAMimeDataForData;


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
    // 添加序列前的验证器：返回 false 拒绝添加（data 为 dataframe 数据，name 为列名）
    typedef std::function< bool(const DAData&, const QString&) > AddSeriesValidator;

public:
    DAPySeriesListView(QWidget* par = nullptr);
    ~DAPySeriesListView();
    // 设置数据管理器
    void setDataManager(DADataManager* dataMgr);
    DADataManager* getDataManager() const;
    // 模式
    void setAcceptMode(AcceptMode mode);
    AcceptMode getAcceptMode() const;
    // 设置添加验证器（可为空），在 addSeries 实际添加前调用，返回 false 则拒绝
    void setAddSeriesValidator(AddSeriesValidator validator);

    // 添加序列
    void addSeries(const DAData& dataframeData, const QString& name);
    void removeSeries(const DAData& dataframeData, const QString& name);
    // 移除当前选中
    void removeCurrentSelect();

    // 获取数据
    QList< QPair< DAData, QStringList > > getSeries() const;

Q_SIGNALS:
    void seriesChanged();
    // AcceptOneSeries 模式下一次拖入多个序列时，只保留第一个，其余被忽略
    void excessSeriesIgnored(int ignoredCount);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    bool acceptMimeData(const QMimeData* mime) const;
    bool acceptSeries(const DAMimeDataForData* mime) const;
};
}  // end namespace DA


#endif  // DAPYSERIESLISTVIEW_H
