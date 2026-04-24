#ifndef DAPYNODEPALETTE_H
#define DAPYNODEPALETTE_H
#include <QtCore/qglobal.h>
#include <QObject>
#include <QColor>
#include "DAPyWorkFlowAPI.h"
#include "DAPyNodeState.h"

namespace DA
{

/**
 * @brief Python节点调色板
 * 
 * 定义了Python工作流节点在不同状态下的颜色配置
 * 通过Q_PROPERTY暴露颜色属性，支持主题切换和样式定制
 */
class DAPYWORKFLOW_API DAPyNodePalette : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyNodePalette)
public:
    explicit DAPyNodePalette(QObject* parent = nullptr);
    ~DAPyNodePalette();

    // 获取全局调色板实例
    static DAPyNodePalette& getGlobalPalette();

    // 颜色属性
    Q_PROPERTY(QColor idleColor READ getIdleColor WRITE setIdleColor NOTIFY idleColorChanged)
    Q_PROPERTY(QColor waitingColor READ getWaitingColor WRITE setWaitingColor NOTIFY waitingColorChanged)
    Q_PROPERTY(QColor runningColor READ getRunningColor WRITE setRunningColor NOTIFY runningColorChanged)
    Q_PROPERTY(QColor successColor READ getSuccessColor WRITE setSuccessColor NOTIFY successColorChanged)
    Q_PROPERTY(QColor errorColor READ getErrorColor WRITE setErrorColor NOTIFY errorColorChanged)
    Q_PROPERTY(QColor skippedColor READ getSkippedColor WRITE setSkippedColor NOTIFY skippedColorChanged)

    // 获取颜色
    QColor getIdleColor() const;
    QColor getWaitingColor() const;
    QColor getRunningColor() const;
    QColor getSuccessColor() const;
    QColor getErrorColor() const;
    QColor getSkippedColor() const;

    // 设置颜色
    void setIdleColor(const QColor& color);
    void setWaitingColor(const QColor& color);
    void setRunningColor(const QColor& color);
    void setSuccessColor(const QColor& color);
    void setErrorColor(const QColor& color);
    void setSkippedColor(const QColor& color);

    // 根据节点状态获取颜色
    QColor getColorForState(DAPyNodeState state) const;

    // 全局颜色获取（静态方法）
    static QColor getGlobalIdleColor();
    static QColor getGlobalWaitingColor();
    static QColor getGlobalRunningColor();
    static QColor getGlobalSuccessColor();
    static QColor getGlobalErrorColor();
    static QColor getGlobalSkippedColor();
    static QColor getGlobalColorForState(DAPyNodeState state);

Q_SIGNALS:
    void idleColorChanged(const QColor& color);
    void waitingColorChanged(const QColor& color);
    void runningColorChanged(const QColor& color);
    void successColorChanged(const QColor& color);
    void errorColorChanged(const QColor& color);
    void skippedColorChanged(const QColor& color);

private:
    Q_DISABLE_COPY(DAPyNodePalette)
};

}  // end of namespace DA

#endif  // DAPYNODEPALETTE_H