#include "DAPyNodePalette.h"
#include "DAPyNodeState.h"
#include "DAGlobals.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================
namespace DA
{

class DAPyNodePalettePrivate
{
    DA_DECLARE_PUBLIC(DAPyNodePalette)
public:
    DAPyNodePalettePrivate(DAPyNodePalette* p);
    ~DAPyNodePalettePrivate();

public:
    QColor mIdleColor;      ///< 空闲状态颜色
    QColor mWaitingColor;   ///< 等待状态颜色
    QColor mRunningColor;   ///< 运行状态颜色
    QColor mSuccessColor;   ///< 成功状态颜色
    QColor mErrorColor;     ///< 错误状态颜色
    QColor mSkippedColor;   ///< 跳过状态颜色
};

DAPyNodePalettePrivate::DAPyNodePalettePrivate(DAPyNodePalette* p) : q_ptr(p)
{
    // 设置默认颜色
    mIdleColor    = QColor();                     // 默认颜色（透明）
    mWaitingColor = QColor(255, 255, 0, 100);     // 黄色半透明边框
    mRunningColor = QColor(0, 255, 0, 100);       // 绿色半透明边框
    mSuccessColor = QColor(0, 255, 0, 200);       // 绿色填充
    mErrorColor   = QColor(255, 0, 0, 200);       // 红色填充
    mSkippedColor = QColor(128, 128, 128, 200);   // 灰色填充
}

DAPyNodePalettePrivate::~DAPyNodePalettePrivate()
{
}

// ===================================================
// DAPyNodePalette 实现
// ===================================================

/**
 * @brief 构造函数
 * @param parent 父对象
 */
DAPyNodePalette::DAPyNodePalette(QObject* parent) : QObject(parent)
{
    DA_PIMPL_CONSTRUCT(DAPyNodePalette);
}

/**
 * @brief 析构函数
 */
DAPyNodePalette::~DAPyNodePalette()
{
}

/**
 * @brief 获取全局调色板实例
 * @return 全局调色板引用
 */
DAPyNodePalette& DAPyNodePalette::getGlobalPalette()
{
    static DAPyNodePalette globalPalette;
    return globalPalette;
}

/**
 * @brief 获取空闲状态颜色
 * @return 空闲状态颜色
 */
QColor DAPyNodePalette::getIdleColor() const
{
    return (DA_DC->mIdleColor);
}

/**
 * @brief 获取等待状态颜色
 * @return 等待状态颜色
 */
QColor DAPyNodePalette::getWaitingColor() const
{
    return (DA_DC->mWaitingColor);
}

/**
 * @brief 获取运行状态颜色
 * @return 运行状态颜色
 */
QColor DAPyNodePalette::getRunningColor() const
{
    return (DA_DC->mRunningColor);
}

/**
 * @brief 获取成功状态颜色
 * @return 成功状态颜色
 */
QColor DAPyNodePalette::getSuccessColor() const
{
    return (DA_DC->mSuccessColor);
}

/**
 * @brief 获取错误状态颜色
 * @return 错误状态颜色
 */
QColor DAPyNodePalette::getErrorColor() const
{
    return (DA_DC->mErrorColor);
}

/**
 * @brief 获取跳过状态颜色
 * @return 跳过状态颜色
 */
QColor DAPyNodePalette::getSkippedColor() const
{
    return (DA_DC->mSkippedColor);
}

/**
 * @brief 设置空闲状态颜色
 * @param color 颜色值
 */
void DAPyNodePalette::setIdleColor(const QColor& color)
{
    if (DA_D->mIdleColor != color) {
        DA_D->mIdleColor = color;
        Q_EMIT idleColorChanged(color);
    }
}

/**
 * @brief 设置等待状态颜色
 * @param color 颜色值
 */
void DAPyNodePalette::setWaitingColor(const QColor& color)
{
    if (DA_D->mWaitingColor != color) {
        DA_D->mWaitingColor = color;
        Q_EMIT waitingColorChanged(color);
    }
}

/**
 * @brief 设置运行状态颜色
 * @param color 颜色值
 */
void DAPyNodePalette::setRunningColor(const QColor& color)
{
    if (DA_D->mRunningColor != color) {
        DA_D->mRunningColor = color;
        Q_EMIT runningColorChanged(color);
    }
}

/**
 * @brief 设置成功状态颜色
 * @param color 颜色值
 */
void DAPyNodePalette::setSuccessColor(const QColor& color)
{
    if (DA_D->mSuccessColor != color) {
        DA_D->mSuccessColor = color;
        Q_EMIT successColorChanged(color);
    }
}

/**
 * @brief 设置错误状态颜色
 * @param color 颜色值
 */
void DAPyNodePalette::setErrorColor(const QColor& color)
{
    if (DA_D->mErrorColor != color) {
        DA_D->mErrorColor = color;
        Q_EMIT errorColorChanged(color);
    }
}

/**
 * @brief 设置跳过状态颜色
 * @param color 颜色值
 */
void DAPyNodePalette::setSkippedColor(const QColor& color)
{
    if (DA_D->mSkippedColor != color) {
        DA_D->mSkippedColor = color;
        Q_EMIT skippedColorChanged(color);
    }
}

/**
 * @brief 根据节点状态获取对应的颜色
 * @param state 节点状态
 * @return 对应状态的颜色
 */
QColor DAPyNodePalette::getColorForState(DAPyNodeState state) const
{
    switch (state) {
    case Idle:
        return getIdleColor();
    case Waiting:
        return getWaitingColor();
    case Running:
        return getRunningColor();
    case Success:
        return getSuccessColor();
    case Error:
        return getErrorColor();
    case Skipped:
        return getSkippedColor();
    default:
        return QColor();
    }
}

/**
 * @brief 获取全局空闲状态颜色
 * @return 全局空闲状态颜色
 */
QColor DAPyNodePalette::getGlobalIdleColor()
{
    return getGlobalPalette().getIdleColor();
}

/**
 * @brief 获取全局等待状态颜色
 * @return 全局等待状态颜色
 */
QColor DAPyNodePalette::getGlobalWaitingColor()
{
    return getGlobalPalette().getWaitingColor();
}

/**
 * @brief 获取全局运行状态颜色
 * @return 全局运行状态颜色
 */
QColor DAPyNodePalette::getGlobalRunningColor()
{
    return getGlobalPalette().getRunningColor();
}

/**
 * @brief 获取全局成功状态颜色
 * @return 全局成功状态颜色
 */
QColor DAPyNodePalette::getGlobalSuccessColor()
{
    return getGlobalPalette().getSuccessColor();
}

/**
 * @brief 获取全局错误状态颜色
 * @return 全局错误状态颜色
 */
QColor DAPyNodePalette::getGlobalErrorColor()
{
    return getGlobalPalette().getErrorColor();
}

/**
 * @brief 获取全局跳过状态颜色
 * @return 全局跳过状态颜色
 */
QColor DAPyNodePalette::getGlobalSkippedColor()
{
    return getGlobalPalette().getSkippedColor();
}

/**
 * @brief 根据节点状态获取全局颜色
 * @param state 节点状态
 * @return 对应状态的全局颜色
 */
QColor DAPyNodePalette::getGlobalColorForState(DAPyNodeState state)
{
    return getGlobalPalette().getColorForState(state);
}

}  // end DA