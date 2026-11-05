#ifndef DASIGNALSOURCE_H
#define DASIGNALSOURCE_H
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>
#include <memory>

namespace DA
{
/**
 * @brief 信号来源抽象基类（方案 §4.3）
 *
 * 三种具体来源：音频/视频抽取结果文件、数据帧列、视频（抽取成临时 WAV 后按文件来源处理）。
 *
 * 取数约定（方案 §4.2）：
 * - 返回值统一为单精度浮点（float32）；
 * - readRange 必须允许跨线程调用，实现不得触碰 Qt 界面对象；
 * - 全量读取不允许在界面线程做。
 */
class DASignalSource
{
public:
    enum SourceType
    {
        FileSource,       ///< 音频文件 / 视频抽取出的临时 WAV
        DataFrameSource,  ///< 数据帧列（引用式，不复制整列到内存）
        VideoSource       ///< 视频文件（抽取音频轨后按文件来源处理）
    };

    virtual ~DASignalSource() = default;

    // 来源类型
    virtual SourceType type() const = 0;
    // 按通道读一段（start/count 以采样点为单位），out 为 float32
    virtual bool readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const = 0;
    // 通道数（≥1）
    virtual int channelCount() const = 0;
    // 通道名
    virtual QStringList channelNames() const = 0;
    // 每通道采样点数
    virtual qint64 sampleCount() const = 0;
    // 采样率（赫兹，用户确认后的值）
    virtual double sampleRate() const = 0;
    // 播放用本地文件路径；数据帧来源返回空串（播放前需临时导出 WAV，M9）
    virtual QString playbackFilePath() const = 0;
    // 来源描述（工程保存与界面上「信号源缺失」提示用）
    virtual QString sourceDescription() const = 0;

    // 设置/覆盖采样率（导入确认后写入）
    virtual void setSampleRate(double fs) = 0;
};

using DASignalSourcePtr = std::shared_ptr< DASignalSource >;
}  // namespace DA

#endif  // DASIGNALSOURCE_H
