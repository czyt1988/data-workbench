#ifndef DAAPPLAYOUTMANAGER_H
#define DAAPPLAYOUTMANAGER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

namespace DA
{
class AppMainWindow;

/**
 * @brief 布局方案管理器（D12：预置 + 自定义）
 *
 * 布局 = geometry + QMainWindow state + ADS dock state 三者组合
 * （复用 DAAppWindowStateSerializer），持久化到配置目录独立 JSON 文件。
 *
 * 预置方案（只读，不可删除）：
 * - "Default"：启动时抓取的默认快照（与「恢复默认布局」同一字节流）
 * - "Focus Analysis"：默认布局基础上隐藏左右侧栏（专注分析）
 *
 * 所有操作仅涉及顶层窗口状态（D4），不触碰图表区嵌套布局
 * （DAChartOperateWidget::saveChartLayout/restoreChartLayout 随工程文件管理）
 */
class DAAppLayoutManager : public QObject
{
    Q_OBJECT
public:
    explicit DAAppLayoutManager(AppMainWindow* mw, QObject* par = nullptr);
    ~DAAppLayoutManager() override;

    // 初始化：注册预置方案（默认布局用启动快照），并加载磁盘上的自定义方案
    void initialize(const QByteArray& defaultSnapshot);
    // 保存当前布局为自定义方案（同名覆盖）
    bool saveLayoutAs(const QString& name);
    // 打开（应用）方案，字节流版本不匹配等失败返回 false
    bool openLayout(const QString& name);
    // 删除自定义方案（预置方案不可删，返回 false）
    bool removeLayout(const QString& name);
    // 所有方案名（预置在前）
    QStringList layoutNames() const;
    // 是否预置方案
    bool isPreset(const QString& name) const;

private:
    // 自定义方案持久化文件路径 <config>/dawork-layouts.json
    QString layoutsFilePath() const;
    // 读取/写回自定义方案
    void loadCustomLayouts();
    void saveCustomLayouts() const;

    AppMainWindow* mMainWindow { nullptr };
    QByteArray mDefaultSnapshot;           ///< 启动默认快照（Default 方案的内容）
    QHash< QString, QByteArray > mCustom;  ///< 自定义方案名 → 状态字节流
    QStringList mCustomOrder;              ///< 自定义方案名顺序（持久化顺序稳定）
};

}  // namespace DA

#endif  // DAAPPLAYOUTMANAGER_H
