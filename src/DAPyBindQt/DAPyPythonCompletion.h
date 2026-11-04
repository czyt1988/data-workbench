#ifndef DAPYPYTHONCOMPLETION_H
#define DAPYPYTHONCOMPLETION_H
#include <QString>
#include <QStringList>
#include "DAPyBindQtGlobal.h"

namespace DA
{
/**
 * @brief Python 自动补全数据源（通用）
 *
 * 提供 Python 关键字/内置名，以及常用对象成员（pandas / numpy / DataFrame /
 * Series / Series.str / Series.dt / Index）。成员列表在首次使用时从当前 Python
 * 解释器 dir() 读取并缓存，之后为纯内存查询；Python 未初始化时返回空列表，
 * 调用方按“无补全”降级处理。
 *
 * 本类只负责“有哪些候选词”，不包含弹窗与文本编辑逻辑；弹窗在
 * DAGui 的 DAPythonCodeEdit 中，列名等上下文词由调用方注入。
 */
class DAPYBINDQT_API DAPyPythonCompletion
{
public:
    // Python 关键字（与高亮器共用词表）
    static QStringList keywords();
    // Python 内置名
    static QStringList builtins();
    // 对象成员：pd / np / DataFrame / Series / Series.str / Series.dt / Index
    static QStringList membersOf(const QString& objectKey);
    // 当前是否处于可用的 Python 环境
    static bool isPythonReady();
};
}  // namespace DA
#endif  // DAPYPYTHONCOMPLETION_H
