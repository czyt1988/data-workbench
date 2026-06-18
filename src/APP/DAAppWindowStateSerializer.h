#ifndef DAAPPWINDOWSTATESERIALIZER_H
#define DAAPPWINDOWSTATESERIALIZER_H

#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QVector>

namespace DA
{
/**
 * @brief APP 主窗口状态快照
 */
struct DAAppWindowStateSnapshot
{
    QByteArray geometry;
    QByteArray mainWindowState;
    QByteArray dockingState;
};

/**
 * @brief 序列化窗口状态
 * @param state 状态快照
 * @return 序列化后的字节数组
 */
inline QByteArray serializeAppWindowState(const DAAppWindowStateSnapshot& state)
{
    QVector< QByteArray > uiStateArr;
    uiStateArr << state.geometry << state.mainWindowState;
    if (!state.dockingState.isEmpty()) {
        uiStateArr << state.dockingState;
    }

    QByteArray res;
    QBuffer buffer(&res);
    buffer.open(QIODevice::WriteOnly);
    QDataStream st(&buffer);
    st << uiStateArr;
    return res;
}

/**
 * @brief 反序列化窗口状态
 * @param data 序列化后的字节数组
 * @param state 输出状态快照
 * @return 成功返回true
 */
inline bool deserializeAppWindowState(const QByteArray& data, DAAppWindowStateSnapshot* state)
{
    if (nullptr == state) {
        return false;
    }

    QVector< QByteArray > uiStateArr;
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream st(&buffer);
    st >> uiStateArr;
    if (QDataStream::Ok != st.status()) {
        return false;
    }

    state->geometry        = uiStateArr.value(0);
    state->mainWindowState = uiStateArr.value(1);
    state->dockingState    = uiStateArr.value(2);
    return 2 <= uiStateArr.size();
}

}  // namespace DA

#endif  // DAAPPWINDOWSTATESERIALIZER_H
