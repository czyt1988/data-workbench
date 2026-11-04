#ifndef DAAPPDATAACTIONHANDLER_H
#define DAAPPDATAACTIONHANDLER_H
#include <QObject>
#include <QList>
#include <memory>
#include "DAAppActionHandlerDeps.h"
#include "Table/DATableExporter.h"
#include "Table/Views/DADataHeaderView.h"

class QAction;
class QMenu;

namespace DA
{
class DADataOperateOfDataFrameWidget;
class DADataManagerTreeWidget;
class DAData;

/**
 * @brief 数据域动作分发器：数据集增删改、导入、导出与复制为表格
 *
 * 从 DAAppController 抽出的分发器，只负责把动作转成对当前活动对象的调用；
 * 翻译上下文沿用 DA::DAAppController（.ts 译文已存在，拆类不改上下文）。
 */
class DAAppDataActionHandler : public QObject
{
    Q_OBJECT
public:
    explicit DAAppDataActionHandler(const DAAppActionHandlerDeps& deps, QObject* par = nullptr);
    // 建立本域的动作与信号绑定
    void initConnections();

public Q_SLOTS:
    // 把选中的 dataframe 数据集复制到剪贴板（数据管理树右键「复制为」与表头右键
    // 「复制选中列为」共用）：fmt 决定目标形态（Tab 文本/Markdown/HTML），经
    // DATableExporter 构建；超大表先确认（isLargeForCopy 阈值），构建期间进度
    // 对话框可取消。columns 非空时只复制这些列（表头右键入口），空 = 整表；
    // sourcePage 非空时列子集取数固定用该页面（表头右键菜单是独立窗口，
    // 触发时停靠区焦点尚未归还，不能走焦点检查取当前页）
    void copySelectedDataAsTable(DA::DATableExporter::Format fmt,
                                 const QList< int >& columns = QList< int >(),
                                 DADataOperateOfDataFrameWidget* sourcePage = nullptr);
    // 导出选中数据集为 Markdown/HTML（右键导出子菜单、主导出按钮与表头右键
    // 「导出选中列」共用）：取数/渲染与复制共用 DATableExporter，增量写入文件；
    // path 非空时跳过保存对话框（主导出按钮已选好路径）。columns/sourcePage 语义同上
    void exportSelectedDataAsDocument(DA::DATableExporter::Format fmt,
                                      const QString& path = QString(),
                                      const QList< int >& columns = QList< int >(),
                                      DADataOperateOfDataFrameWidget* sourcePage = nullptr);
    // 表头右键「复制选中列为/导出选中列」信号处理器：page 固定为发送信号表头
    // 所在页面（菜单窗口抢焦点，不能经焦点检查取当前页），从页面解析选中列
    // （无选中回退 fallbackColumn），映射格式枚举后走共用复制/导出链路
    void onHeaderColumnDataCopyRequested(DADataOperateOfDataFrameWidget* page,
                                         int fallbackColumn,
                                         DA::DADataHeaderView::ColumnDataFormat format);
    void onHeaderColumnDataExportRequested(DADataOperateOfDataFrameWidget* page,
                                           int fallbackColumn,
                                           DA::DADataHeaderView::ColumnDataFormat format);

private Q_SLOTS:
    // 添加数据
    void onActionAddDataTriggered();
    // 移除数据
    void onActionRemoveDataTriggered();
    // 重命名数据集（树视图选中项进入编辑）
    void onActionRenameDataTriggered();
    // 导出数据（大按钮，format 为空则按保存对话框后缀判定）
    void onActionExportDataTriggered();
    // 按指定格式导出（csv/excel/pickle/parquet）
    void onActionExportDataAsTriggered();
    // 执行导出：取选中数据集 → 保存对话框 → exportToFile → daInfo/daWarning 反馈
    // format 非空时直接使用（子菜单路径），为空时由文件后缀判定（大按钮路径）
    void exportSelectedData(const QString& format);
    // 取要操作的数据集（优先数据管理树选中，回退当前操作数据）与对应表元数据束，
    // 成功构造 DATableExporter 返回 true（exporter/dataOut 为出参；束可为空=无样式）；
    // columns 非空时构造列子集导出器（表头右键「按选中列」入口）；
    // sourcePage 非空时列子集取数固定用该页面（空 = 焦点检查取当前页）
    bool makeExporterForSelection(std::unique_ptr< DA::DATableExporter >* exporter,
                                  DA::DAData* dataOut = nullptr,
                                  const QList< int >& columns = QList< int >(),
                                  DADataOperateOfDataFrameWidget* sourcePage = nullptr);

private:
    DAAppActionHandlerDeps mDeps;
};
}  // namespace DA

#endif  // DAAPPDATAACTIONHANDLER_H
