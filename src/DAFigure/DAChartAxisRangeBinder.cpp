#include "DAChartAxisRangeBinder.h"
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param par 父对象
 */
DAChartAxisRangeBinder::DAChartAxisRangeBinder(QObject* par) : QObject(par)
{
}

/**
 * @brief 构造函数，绑定源图表和跟随图表的坐标轴
 * @param source 源图表
 * @param sourceAxisid 源坐标轴ID
 * @param follower 跟随图表
 * @param followerAxisid 跟随坐标轴ID
 * @param par 父对象
 */
DAChartAxisRangeBinder::DAChartAxisRangeBinder(QwtPlot* source, int sourceAxisid, QwtPlot* follower, int followerAxisid, QObject* par)
    : QObject(par)
{
    setSourceChart(source, sourceAxisid);
    setFollowerChart(follower, followerAxisid);
    bind();
}

/**
 * @brief 设置源图表及坐标轴
 * @param source 源图表
 * @param axisid 源坐标轴ID
 */
void DAChartAxisRangeBinder::setSourceChart(QwtPlot* source, int axisid)
{
    mSourcePlot   = source;
    mSourceAxisid = axisid;
}

/**
 * @brief 获取源图表
 * @return 源图表指针
 */
QwtPlot* DAChartAxisRangeBinder::getSourcePlot() const
{
    return mSourcePlot.data();
}

/**
 * @brief 获取源坐标轴ID
 * @return 源坐标轴ID
 */
int DAChartAxisRangeBinder::getSourceAxis() const
{
    return mSourceAxisid;
}

/**
 * @brief 设置跟随图表及坐标轴
 * @param follower 跟随图表
 * @param axisid 跟随坐标轴ID
 */
void DAChartAxisRangeBinder::setFollowerChart(QwtPlot* follower, int axisid)
{
    mFollowerPlot   = follower;
    mFollowerAxisid = axisid;
}

/**
 * @brief 获取跟随图表
 * @return 跟随图表指针
 */
QwtPlot* DAChartAxisRangeBinder::getFollowerPlot() const
{
    return mFollowerPlot.data();
}

/**
 * @brief 获取跟随坐标轴ID
 * @return 跟随坐标轴ID
 */
int DAChartAxisRangeBinder::getFollowerAxis() const
{
    return mFollowerAxisid;
}

/**
 * @brief 绑定源图表和跟随图表的坐标轴
 * @return 绑定成功返回true，否则返回false
 */
bool DAChartAxisRangeBinder::bind()
{
    if (!isValid()) {
        return false;
    }
    QwtScaleWidget* axisSource = mSourcePlot->axisWidget(mSourceAxisid);

    mCon = QObject::connect(
        axisSource, &QwtScaleWidget::scaleDivChanged, this, &DAChartAxisRangeBinder::onSourcePlotScaleDivChanged);
    return mCon;
}

/**
 * @brief 解除绑定
 * @return 解除成功返回true，否则返回false
 */
bool DAChartAxisRangeBinder::unbind()
{
    if (!isBinded()) {
        return false;
    }
    return QObject::disconnect(mCon);
}

/**
 * @brief 判断绑定是否有效
 * @return 有效返回true，否则返回false
 */
bool DAChartAxisRangeBinder::isValid() const
{
    return (mSourcePlot && mFollowerPlot && QwtAxis::isValid(mSourceAxisid) && QwtAxis::isValid(mFollowerAxisid));
}

/**
 * @brief 判断是否与另一个绑定器相同
 * @param other 另一个绑定器
 * @return 相同返回true，否则返回false
 */
bool DAChartAxisRangeBinder::isSame(const DAChartAxisRangeBinder& other) const
{
    return (mSourcePlot == other.mSourcePlot) && (mSourceAxisid == other.mSourceAxisid)
           && (mFollowerPlot == other.mFollowerPlot) && (mFollowerAxisid == other.mFollowerAxisid);
}

/**
 * @brief 判断是否与指定的源图表、坐标轴和跟随图表、坐标轴相同
 * @param source 源图表
 * @param sourceAxisid 源坐标轴ID
 * @param follower 跟随图表
 * @param followerAxisid 跟随坐标轴ID
 * @return 相同返回true，否则返回false
 */
bool DAChartAxisRangeBinder::isSame(QwtPlot* source, int sourceAxisid, QwtPlot* follower, int followerAxisid) const
{
    return (mSourcePlot == source) && (mSourceAxisid == sourceAxisid) && (mFollowerPlot == follower)
           && (mFollowerAxisid == followerAxisid);
}

/**
 * @brief 等于运算符
 * @param other 另一个绑定器
 * @return 相同返回true，否则返回false
 */
bool DAChartAxisRangeBinder::operator==(const DAChartAxisRangeBinder& other) const
{
    return isSame(other);
}

/**
 * @brief 源图表坐标轴刻度变化时的槽函数
 *
 * 将源图表的坐标轴刻度同步到跟随图表
 */
void DAChartAxisRangeBinder::onSourcePlotScaleDivChanged()
{
    if (!mFollowerPlot || !mSourcePlot || !QwtAxis::isValid(mSourceAxisid) || !QwtAxis::isValid(mFollowerAxisid)) {
        return;
    }
    mFollowerPlot->setAxisScaleDiv(mFollowerAxisid, mSourcePlot->axisScaleDiv(mSourceAxisid));
    if (mReplotImmediately) {
        mFollowerPlot->replotAll();
    }
}

/**
 * @brief 判断是否立即重绘
 * @return 如果立即重绘返回true，否则返回false
 */
bool DAChartAxisRangeBinder::isRplotImmediately() const
{
    return mReplotImmediately;
}

/**
 * @brief 设置是否立即重绘
 * @param v 是否立即重绘
 */
void DAChartAxisRangeBinder::setReplotImmediately(bool v)
{
    mReplotImmediately = v;
}

/**
 * @brief 判断是否已绑定
 * @return 已绑定返回true，否则返回false
 */
bool DAChartAxisRangeBinder::isBinded() const
{
    return mCon;
}
}
