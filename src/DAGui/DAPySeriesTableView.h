#ifndef DAPYSERIESTABLEVIEW_H
#define DAPYSERIESTABLEVIEW_H
#include "DACacheWindowTableView.h"
#include "DAData.h"
#include "DAAutoincrementSeries.hpp"
namespace DA
{
class DAPySeriesTableModel;
class DAPySeriesTableView : public DACacheWindowTableView
{
public:
    DAPySeriesTableView(QWidget* parent = nullptr);
    DAPySeriesTableModel* getSeriesModel() const;
    // wrapper
    //  追加series
    void appendSeries(const DAPySeries& s);
    void appendSeries(const DAAutoincrementSeries< double >& s);
    // 插入series，index如果超出范围，会append，例如[s0,s1],insertSeries(3,s2),结果是[s0,s1,s2]
    void insertSeries(int c, const DAPySeries& s);
    void insertSeries(int c, const DAAutoincrementSeries< double >& s);
    // 把series设置到对应位置，如果有，则替换
    void setSeriesAt(int c, const DAPySeries& s);
    void setSeriesAt(int c, const DAAutoincrementSeries< double >& s);
    //
    void resizeVerticalHeader();
    // 清除内容
    void clear();
};
}

#endif  // DAPYSERIESTABLEVIEW_H
