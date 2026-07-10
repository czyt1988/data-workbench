# -*- coding: utf-8 -*-
"""Manual translation table for the remaining unfinished entries.

Keys are tuples (context, source) — but to keep this file editable we store a
dict mapping "context|||source" -> translation. Source text stored AS IT
APPEARS in the .ts <source> element (i.e. with XML entities still encoded as
their literal characters like &gt;, &lt;, &amp;, &quot; — NO, we store the
DECODED source and the applier re-encodes). We use decoded form (plain >, <, &, ")
for matching because that's what lupdate produces internally.

Note: the .ts source elements contain XML-escaped text (e.g. &gt;&gt;). When
matching, decode those to plain text. We decode in the applier.
"""

# Decoded source strings (plain text, not XML-escaped)
TRANSLATIONS = {
    # DA::DADataManagerTreeModel
    ("DA::DADataManagerTreeModel", "Properties"): "属性",
    # DA::DANodeLinkItemSettingWidget
    ("DA::DANodeLinkItemSettingWidget", "Knuckle"): "折线",
    ("DA::DANodeLinkItemSettingWidget", "Straight"): "直线",
    ("DA::DANodeLinkItemSettingWidget", "Bezier"): "贝塞尔",
    # DA::DAPluginManagerDialog
    ("DA::DAPluginManagerDialog", "Version"): "版本",
    ("DA::DAPluginManagerDialog", "Description"): "描述",

    # ---- DAChartAddCurveWidget (UI) ----
    ("DAChartAddCurveWidget", "Add XY Series"): "添加 XY 序列",
    ("DAChartAddCurveWidget", "X"): "X",
    ("DAChartAddCurveWidget", "Autoincrement series"): "自增序列",
    ("DAChartAddCurveWidget", "Initial value"): "初始值",
    ("DAChartAddCurveWidget", "1"): "1",
    ("DAChartAddCurveWidget", "Self increasing step size"): "自增步长",
    ("DAChartAddCurveWidget", "Y"): "Y",

    # ---- DAChartAddOHLCSeriesWidget (UI) ----
    ("DAChartAddOHLCSeriesWidget", "Add XY Series"): "添加 XY 序列",
    ("DAChartAddOHLCSeriesWidget", "X"): "X",
    ("DAChartAddOHLCSeriesWidget", "Autoincrement series"): "自增序列",
    ("DAChartAddOHLCSeriesWidget", "Initial value"): "初始值",
    ("DAChartAddOHLCSeriesWidget", "1"): "1",
    ("DAChartAddOHLCSeriesWidget", "Self increasing step size"): "自增步长",
    ("DAChartAddOHLCSeriesWidget", "Hight"): "最高",

    # ---- DAChartAddSpectrogramWidget (UI) ----
    ("DAChartAddSpectrogramWidget", "Add Curve"): "添加曲线",
    ("DAChartAddSpectrogramWidget", "1.Data"): "1.数据",
    ("DAChartAddSpectrogramWidget", ">>"): ">>",
    ("DAChartAddSpectrogramWidget", "2.Plot"): "2.绘图",

    # ---- DAChartAddXYESeriesWidget (UI) ----
    ("DAChartAddXYESeriesWidget", "Add XY Series"): "添加 XY 序列",
    ("DAChartAddXYESeriesWidget", "X"): "X",
    ("DAChartAddXYESeriesWidget", "Autoincrement series"): "自增序列",
    ("DAChartAddXYESeriesWidget", "Initial value"): "初始值",
    ("DAChartAddXYESeriesWidget", "1"): "1",
    ("DAChartAddXYESeriesWidget", "Self increasing step size"): "自增步长",
    ("DAChartAddXYESeriesWidget", "Y"): "Y",

    # ---- DAChartAddXYSeriesWidget (UI) ----
    ("DAChartAddXYSeriesWidget", "Add XY Series"): "添加 XY 序列",
    ("DAChartAddXYSeriesWidget", "X"): "X",
    ("DAChartAddXYSeriesWidget", "Autoincrement series"): "自增序列",
    ("DAChartAddXYSeriesWidget", "Initial value"): "初始值",
    ("DAChartAddXYSeriesWidget", "1"): "1",
    ("DAChartAddXYSeriesWidget", "Self increasing step size"): "自增步长",
    ("DAChartAddXYSeriesWidget", "Y"): "Y",
    ("DAChartAddXYSeriesWidget", "Drag the data into the corresponding list"): "把数据拖入对应的列表",

    # ---- DAChartAddtGridRasterDataWidget (UI) ----
    ("DAChartAddtGridRasterDataWidget", "Add XY Series"): "添加 XY 序列",
    ("DAChartAddtGridRasterDataWidget", "X"): "X",
    ("DAChartAddtGridRasterDataWidget", "Y"): "Y",
    ("DAChartAddtGridRasterDataWidget", "Matrics"): "矩阵",

    # ---- DAChartCommonItemsSettingWidget ----
    ("DAChartCommonItemsSettingWidget", "Common Item Setting"): "通用图元设置",
    # DAChartManageWidget
    ("DAChartManageWidget", "Chart Manage"): "图表管理",
    # DAChartSymbolEditWidget
    ("DAChartSymbolEditWidget", "Chart Symbol Edit"): "图表符号编辑",
    ("DAChartSymbolEditWidget", "Outline"): "轮廓",
    # DADataManageWidget
    ("DADataManageWidget", "Data Manage"): "数据管理",
    # DADataManagerTreeWidget (Qt Designer default "Form")
    ("DADataManagerTreeWidget", "Form"): "窗体",
    ("DADataManagerTreeWidget", "-"): "-",
    # DADataOperateOfDataFrameWidget
    ("DADataOperateOfDataFrameWidget", "DataFrame Operate"): "DataFrame 操作",
    # DADataOperateWidget
    ("DADataOperateWidget", "Data Operator"): "数据操作",
    ("DADataOperateWidget", "This is the data operation window, which is specially responsible for data operation and display"): "这是数据操作窗口，专门负责数据的操作和展示",
    # DADataframeToVectorPointWidget
    ("DADataframeToVectorPointWidget", "Dataframe To Vector Point"): "DataFrame 转矢量点",
    ("DADataframeToVectorPointWidget", "x:"): "x:",
    ("DADataframeToVectorPointWidget", "y:"): "y:",
    ("DADataframeToVectorPointWidget", "data view"): "数据视图",
    # DADialogChartGuide
    ("DADialogChartGuide", "Chart Guide"): "图表向导",
    # DADialogDataFrameSeriesSelector
    ("DADialogDataFrameSeriesSelector", "Dialog"): "对话框",
    ("DADialogDataFrameSeriesSelector", "Data preview"): "数据预览",

    # ---- DADialogDataframeColumnCastToDatetime (UI, rich text tooltips) ----
    ("DADialogDataframeColumnCastToDatetime", "Cast To Datetime"): "转换为日期时间",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>If True and no format is given, attempt to infer the format of the datetime strings, and if it can be inferred, switch to a faster method of parsing them. In some cases this can increase the parsing speed by ~5-10x.</p></body></html>"):
        "<html><head/><body><p>若为 True 且未指定 format，将尝试推断日期时间字符串的格式，若可推断则切换到更快的解析方式。某些情况下可将解析速度提升约 5-10 倍。</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "infer datetime format "): "推断日期时间格式",
    ("DADialogDataframeColumnCastToDatetime", "format"): "格式",
    ("DADialogDataframeColumnCastToDatetime", "%d/%m/%Y"): "%d/%m/%Y",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>If True, require an exact format match.</p><p>If False, allow the format to match anywhere in the target string.</p></body></html>"):
        "<html><head/><body><p>若为 True，要求格式精确匹配。</p><p>若为 False，允许格式匹配目标字符串的任意位置。</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "exact "): "精确匹配",
    ("DADialogDataframeColumnCastToDatetime", "errors:"): "错误处理:",
    ("DADialogDataframeColumnCastToDatetime", "invalid parsing will raise an exception"): "无效解析将抛出异常",
    ("DADialogDataframeColumnCastToDatetime", "invalid parsing will be set as NaN"): "无效解析将设为 NaN",
    ("DADialogDataframeColumnCastToDatetime", "invalid parsing will return the input"): "无效解析将返回原输入",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>Define the reference date. The numeric values would be parsed as number of units (defined by unit) since this reference date.</p></body></html>"):
        "<html><head/><body><p>定义参考日期。数值将被解析为自该参考日期起以指定单位（由 unit 定义）计数的值。</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "origin"): "参考日期",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>origin is set to 1970-01-01</p></body></html>"):
        "<html><head/><body><p>参考日期设为 1970-01-01</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "unix"): "unix",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>unit must be ‘D’, and origin is set to beginning of Julian Calendar. Julian day number 0 is assigned to the day starting at noon on January 1, 4713 BC</p></body></html>"):
        "<html><head/><body><p>单位必须为 ‘D’，参考日期设为儒略历的起始。儒略日 0 对应公元前 4713 年 1 月 1 日正午开始的那一天。</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "julian"): "julian",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>unit of the arg (D,s,ms,us,ns) denote the unit, which is an integer or float number. This will be based off the origin. Example, with unit=’ms’ and origin=’unix’ (the default), this would calculate the number of milliseconds to the unix epoch start.</p></body></html>"):
        "<html><head/><body><p>参数的单位（D,s,ms,us,ns）表示数值的单位，可为整数或浮点数。该值基于参考日期计算。例如，当 unit=’ms’ 且 origin=’unix’（默认）时，将计算距离 Unix 纪元起点的毫秒数。</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "unit"): "单位",
    ("DADialogDataframeColumnCastToDatetime", "D"): "D",
    ("DADialogDataframeColumnCastToDatetime", "s"): "s",
    ("DADialogDataframeColumnCastToDatetime", "ms"): "ms",
    ("DADialogDataframeColumnCastToDatetime", "us"): "us",
    ("DADialogDataframeColumnCastToDatetime", "ns"): "ns",
    ("DADialogDataframeColumnCastToDatetime", "parse set:"): "解析设置:",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>Specify a date parse order</p><p>If True, parses dates with the day first, eg 10/11/12 is parsed as 2012-11-10. </p><p><span style=\" font-weight:600; color:#afaf00;\">Warning</span>:<span style=\" font-style:italic;\"> dayfirst=True is not strict, but will prefer to parse with day first (this is a known bug, based on dateutil behavior)</span></p></body></html>"):
        "<html><head/><body><p>指定日期解析顺序</p><p>若为 True，优先按日解析，例如 10/11/12 将解析为 2012-11-10。</p><p><span style=\" font-weight:600; color:#afaf00;\">警告</span>:<span style=\" font-style:italic;\"> dayfirst=True 并非严格规则，只是优先按日解析（这是基于 dateutil 行为的已知问题）。</span></p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "day first"): "日优先",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>Specify a date parse order</p><p>- If True parses dates with the year first, eg 10/11/12 is parsed as 2010-11-12.</p><p>- If both dayfirst and yearfirst are True, yearfirst is preceded (same as dateutil).</p><p><span style=\" font-weight:600; color:#b6a80b;\">Warning</span>:<span style=\" font-style:italic;\"> yearfirst=True is not strict, but will prefer to parse with year first (this is a known bug, based on dateutil behavior).</span></p></body></html>"):
        "<html><head/><body><p>指定日期解析顺序</p><p>- 若为 True，优先按年解析，例如 10/11/12 将解析为 2010-11-12。</p><p>- 若 dayfirst 和 yearfirst 同时为 True，yearfirst 优先（与 dateutil 一致）。</p><p><span style=\" font-weight:600; color:#b6a80b;\">警告</span>:<span style=\" font-style:italic;\"> yearfirst=True 并非严格规则，只是优先按年解析（这是基于 dateutil 行为的已知问题）。</span></p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "year first"): "年优先",
    ("DADialogDataframeColumnCastToDatetime", "utc"): "UTC",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>Return UTC DatetimeIndex if True (converting any tz-aware datetime.datetime objects as well).</p></body></html>"):
        "<html><head/><body><p>若为 True，返回 UTC DatetimeIndex（同时转换带时区的 datetime.datetime 对象）。</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime",
     "<html><head/><body><p>If True, use a cache of unique, converted dates to apply the datetime conversion. May produce significant speed-up when parsing duplicate date strings, especially ones with timezone offsets.</p></body></html>"):
        "<html><head/><body><p>若为 True，使用唯一已转换日期的缓存来执行日期时间转换。在解析重复日期字符串（尤其是带时区偏移的）时可显著提升速度。</p></body></html>",
    ("DADialogDataframeColumnCastToDatetime", "cache"): "缓存",
    ("DADialogDataframeColumnCastToDatetime", "OK"): "确定",

    # ---- DADialogDataframeColumnCastToNumeric (UI) ----
    ("DADialogDataframeColumnCastToNumeric", "Cast To Numeric"): "转换为数值",
    ("DADialogDataframeColumnCastToNumeric", "errors:"): "错误处理:",
    ("DADialogDataframeColumnCastToNumeric", "invalid parsing will raise an exception"): "无效解析将抛出异常",
    ("DADialogDataframeColumnCastToNumeric", "invalid parsing will be set as NaN"): "无效解析将设为 NaN",
    ("DADialogDataframeColumnCastToNumeric", "invalid parsing will return the input"): "无效解析将返回原输入",
    ("DADialogDataframeColumnCastToNumeric", "downcast:"): "向下转换:",
    ("DADialogDataframeColumnCastToNumeric",
     "<html><head/><body><p>smallest signed int dtype (min.: np.int8)</p></body></html>"):
        "<html><head/><body><p>最小的有符号整数类型（最小为 np.int8）</p></body></html>",
    ("DADialogDataframeColumnCastToNumeric", "integer"): "整数",
    ("DADialogDataframeColumnCastToNumeric", "signed"): "有符号",
    ("DADialogDataframeColumnCastToNumeric",
     "<html><head/><body><p>smallest unsigned int dtype (min.: np.uint8)</p></body></html>"):
        "<html><head/><body><p>最小的无符号整数类型（最小为 np.uint8）</p></body></html>",
    ("DADialogDataframeColumnCastToNumeric", "unsigned"): "无符号",
    ("DADialogDataframeColumnCastToNumeric",
     "<html><head/><body><p>smallest float dtype (min.: np.float32)</p></body></html>"):
        "<html><head/><body><p>最小的浮点数类型（最小为 np.float32）</p></body></html>",
    ("DADialogDataframeColumnCastToNumeric", "float"): "浮点数",
    ("DADialogDataframeColumnCastToNumeric", "OK"): "确定",

    # ---- DADialogInsertNewColumn (UI) ----
    ("DADialogInsertNewColumn", "Insert New Column"): "插入新列",
    ("DADialogInsertNewColumn", "dtype"): "数据类型",
    ("DADialogInsertNewColumn", "Fill Setting"): "填充设置",
    ("DADialogInsertNewColumn", "Fill in the same value"): "填充相同值",
    ("DADialogInsertNewColumn", "Generate growth value"): "生成递增值",
    ("DADialogInsertNewColumn", "default value"): "默认值",
    ("DADialogInsertNewColumn", "start"): "起始",
    ("DADialogInsertNewColumn", "stop"): "结束",
    ("DADialogInsertNewColumn", "yyyy-MM-dd HH:mm:ss"): "yyyy-MM-dd HH:mm:ss",
    ("DADialogInsertNewColumn", "OK"): "确定",

    # ---- DAGraphicsPixmapItemSettingWidget ----
    ("DAGraphicsPixmapItemSettingWidget", "Pixmap Item Setting"): "图像图元设置",
    ("DAGraphicsPixmapItemSettingWidget", "Alpha"): "透明度",

    # ---- DAMessageLogViewWidget ----
    ("DAMessageLogViewWidget", "Message View"): "消息视图",
    ("DAMessageLogViewWidget", "..."): "...",

    # ---- DANodeItemSettingWidget ----
    ("DANodeItemSettingWidget", "Node Item Setting"): "节点图元设置",
    ("DANodeItemSettingWidget", "Width"): "宽度",
    ("DANodeItemSettingWidget", "Height"): "高度",
    ("DANodeItemSettingWidget", "Lock Aspect Ratio"): "锁定宽高比",
    ("DANodeItemSettingWidget", "rotation"): "旋转",
    ("DANodeItemSettingWidget", "Link Point Location"): "连接点位置",
    ("DANodeItemSettingWidget", "Input Location"): "输入位置",
    ("DANodeItemSettingWidget", "Output Location"): "输出位置",
    ("DANodeItemSettingWidget", "Property"): "属性",
    ("DANodeItemSettingWidget", "movable"): "可移动",
    ("DANodeItemSettingWidget", "resizable"): "可缩放",
    ("DANodeItemSettingWidget", "tooltip"): "工具提示",

    # ---- DANodeLinkItemSettingWidget ----
    ("DANodeLinkItemSettingWidget", "Node Link Item Setting"): "节点连线图元设置",
    ("DANodeLinkItemSettingWidget", "pen:"): "画笔:",
    ("DANodeLinkItemSettingWidget", "link style:"): "连线样式:",

    # ---- DANodeSettingWidget ----
    ("DANodeSettingWidget", "Node Setting"): "节点设置",
    ("DANodeSettingWidget", "Meta Data"): "元数据",
    ("DANodeSettingWidget", "Name:"): "名称:",

    # ---- DAPyWorkFlowNodeItemSettingWidget ----
    ("DAPyWorkFlowNodeItemSettingWidget", "Node Setting"): "节点设置",
    ("DAPyWorkFlowNodeItemSettingWidget", "Node"): "节点",
    ("DAPyWorkFlowNodeItemSettingWidget", "Picture"): "图片",

    # ---- DAPyWorkFlowNodeListWidget ----
    ("DAPyWorkFlowNodeListWidget", "Node List"): "节点列表",

    # ---- DARenameColumnsNameDialog ----
    ("DARenameColumnsNameDialog", "Rename Table"): "重命名表",
    ("DARenameColumnsNameDialog", "Table Name:"): "表名:",
    ("DARenameColumnsNameDialog", "OK"): "确定",

    # ---- DASettingDialog ----
    ("DASettingDialog", "OK"): "确定",
    ("DASettingDialog", "Apply"): "应用",

    # ---- DASettingPageCommon ----
    ("DASettingPageCommon", "UI"): "界面",

    # ---- DATxtFileImportDialog (UI) ----
    ("DATxtFileImportDialog", "Txt Import"): "Txt 导入",
    ("DATxtFileImportDialog", "Text File Path"): "文本文件路径",
    ("DATxtFileImportDialog", "The maximum number of rows to read"): "读取的最大行数",
    ("DATxtFileImportDialog", "The number of lines to skip at the end of the file"): "文件末尾跳过的行数",
    ("DATxtFileImportDialog", "Auto"): "自动",
    ("DATxtFileImportDialog", "max rows"): "最大行数",
    ("DATxtFileImportDialog", "The number of lines to skip at the beginning of the file"): "文件开头跳过的行数",
    ("DATxtFileImportDialog", "delimiter"): "分隔符",
    ("DATxtFileImportDialog", "skip rows"): "跳过行数",
    ("DATxtFileImportDialog", "encoding"): "编码",
    ("DATxtFileImportDialog",
     "<html><head/><body><p>Character or regex pattern to treat as the delimiter</p></body></html>"):
        "<html><head/><body><p>作为分隔符的字符或正则表达式模式</p></body></html>",
    ("DATxtFileImportDialog", "skip over blank lines rather than interpreting as NaN values"): "跳过空行而不是将其解析为 NaN 值",
    ("DATxtFileImportDialog", "skip blank lines"): "跳过空行",
    ("DATxtFileImportDialog", "skip footer"): "跳过末尾",
    ("DATxtFileImportDialog", "Row number(s) containing column labels and marking the start of the data"): "包含列标签并标记数据起点的行号",
    ("DATxtFileImportDialog", "header row"): "表头行",
    ("DATxtFileImportDialog", "No Error"): "无错误",
    ("DATxtFileImportDialog", "Preview"): "预览",
    ("DATxtFileImportDialog",
     "<html><head/><body><p><span style=\" font-family:'-apple-system','BlinkMacSystemFont','Segoe UI','Roboto','Ubuntu','Helvetica Neue','Helvetica','Arial','PingFang SC','Hiragino Sans GB','Microsoft YaHei UI','Microsoft YaHei','Source Han Sans CN','sans-serif','Apple Color Emoji','Segoe UI Emoji'; font-size:15px; color:#05073b; background-color:#fdfdfe;\">In order to avoid the interface stalling due to loading of large texts, the maximum number of words in this preview is limited to 100,000 characters</span></p></body></html>"):
        "<html><head/><body><p><span style=\" font-family:'-apple-system','BlinkMacSystemFont','Segoe UI','Roboto','Ubuntu','Helvetica Neue','Helvetica','Arial','PingFang SC','Hiragino Sans GB','Microsoft YaHei UI','Microsoft YaHei','Source Han Sans CN','sans-serif','Apple Color Emoji','Segoe UI Emoji'; font-size:15px; color:#05073b; background-color:#fdfdfe;\">为避免加载大段文本导致界面卡顿，此预览的最大字数限制为 100,000 字符。</span></p></body></html>",
    ("DATxtFileImportDialog", "Refresh"): "刷新",
    ("DATxtFileImportDialog", "preview max row:"): "预览最大行数:",

    # ---- DataFrameCreatePivotTableDialog (UI) ----
    ("DataFrameCreatePivotTableDialog", "Pivot Table Guide"): "数据透视表向导",
    ("DataFrameCreatePivotTableDialog", "Aggregate function"): "聚合函数",
    ("DataFrameCreatePivotTableDialog", "Special All columns and rows will be added with partial group aggregates across the categories on the rows and columns"): "特殊：所有列和行都会添加按行/列类别的部分分组聚合值",
    ("DataFrameCreatePivotTableDialog", "Margins"): "边缘汇总",
    ("DataFrameCreatePivotTableDialog", "Specifies if the result should be sorted."): "指定结果是否需要排序。",
    ("DataFrameCreatePivotTableDialog", "Name of the row / column that will contain the totals"): "包含合计的行/列名称",
    ("DataFrameCreatePivotTableDialog", "All"): "全部",
    ("DataFrameCreatePivotTableDialog", "OK"): "确定",
    ("DataFrameCreatePivotTableDialog", "Value"): "值",
    ("DataFrameCreatePivotTableDialog", "Index"): "索引",

    # ---- DataFrameDataSearchDialog (UI) ----
    ("DataFrameDataSearchDialog", "Dataframe Search"): "DataFrame 搜索",
    ("DataFrameDataSearchDialog", "Seacrch"): "搜索",
    ("DataFrameDataSearchDialog", "From Begin"): "从头开始",
    ("DataFrameDataSearchDialog", "Find item:"): "查找内容:",
    ("DataFrameDataSearchDialog", "Next"): "下一个",

    # ---- DataFrameDataSelectDialog (UI) ----
    ("DataFrameDataSelectDialog", "Data Filter"): "数据筛选",
    ("DataFrameDataSelectDialog", "Range"): "范围",
    ("DataFrameDataSelectDialog", "-"): "-",
    ("DataFrameDataSelectDialog", "OK"): "确定",

    # ---- DataFrameEvalDatasDialog (UI) ----
    ("DataFrameEvalDatasDialog", "Enter an expression, for example: new_col = age * 2. Column names containing spaces or punctuations (besides underscores) or starting with digits must be surrounded by backticks. (For example, a column named “Area (cm^2)” would be referenced as `Area (cm^2)`). Column names which are Python keywords (like “list”, “for”, “import”, etc) cannot be used. For example, if one of your columns is called a a and you want to sum it with b, your eval should be `a a` + b."):
        "输入一个表达式，例如：new_col = age * 2。包含空格或下划线以外标点、或以数字开头的列名必须用反引号包围。（例如名为 “Area (cm^2)” 的列应写为 `Area (cm^2)`）。Python 关键字（如 “list”、“for”、“import” 等）不能作为列名使用。例如，如果某列名为 a a，想与 b 相加，eval 表达式应写为 `a a` + b。",
    ("DataFrameEvalDatasDialog", "The string to evaluate"): "要求值的字符串",
    ("DataFrameEvalDatasDialog", "OK"): "确定",
    ("DataFrameEvalDatasDialog", "Explanation："): "说明：",
    ("DataFrameEvalDatasDialog", """# I. Basic Syntax

You can write expressions using the following elements:

- **Column names**: Use column names directly in calculations (e.g., `age`, `salary`)
- **Constants**: Numbers, strings, and boolean values (e.g., `10`, `&quot;male&quot;`, `True`)
- **Operators**:
  - Mathematical operations: `+`, `-`, `*`, `/`, `**` (power), `%` (modulus)
  - Comparison operations: `==`, `!=`, `&gt;`, `&lt;`, `&gt;=`, `&lt;=`
  - Logical operations: `and`, `or`, `not`
- **Function calls** (partially supported):
  - Common math functions: `abs()`, `sin()`, `cos()`, `log()`, `exp()`, etc.
  - Conditional logic: `where(condition, x, y)`
  - String operations: `str.contains()`, `str.startswith()`, etc. (to be used with columns)

| Goal | Example Expression |
|------|--------------------|
| Add a new column | `new_col = col1 + col2` |
| Modify an existing column | `col = col * 2` |
| Conditional assignment | `col = where(col &gt; 10, 1, 0)` |
| Filter rows (returns boolean) | `col1 &gt; 5 and col2 &lt; 10` |

---

## Example 1: Add or Modify a Column

```python
age + 10
```

This adds 10 to each value in the `age` column and either updates the original column or writes to a new column.

---

## Example 2: Create a New Column and Assign Values

```python
new_column = salary * 1.1
```

This creates a new column named `new_column`, whose values are 1.1 times those of the `salary` column.

---

## Example 3: Conditional Filtering and Assignment

```python
bonus = where(age &gt; 30, salary * 0.2, salary * 0.1)
```

This means: if age is greater than 30, the bonus is 20% of the salary; otherwise, it&apos;s 10%.

---

## Example 4: String Matching (for filtering)

```python
name.str.contains(&quot;John&quot;)
```

This can be used to filter rows where the name contains &quot;John&quot;."""):
        """# 一、基本语法

可以使用以下元素编写表达式：

- **列名**：直接在计算中使用列名（如 `age`、`salary`）
- **常量**：数字、字符串和布尔值（如 `10`、`&quot;male&quot;`、`True`）
- **运算符**：
  - 数学运算：`+`、`-`、`*`、`/`、`**`（幂）、`%`（取模）
  - 比较运算：`==`、`!=`、`&gt;`、`&lt;`、`&gt;=`、`&lt;=`
  - 逻辑运算：`and`、`or`、`not`
- **函数调用**（部分支持）：
  - 常用数学函数：`abs()`、`sin()`、`cos()`、`log()`、`exp()` 等
  - 条件逻辑：`where(condition, x, y)`
  - 字符串操作：`str.contains()`、`str.startswith()` 等（需配合列使用）

| 目标 | 表达式示例 |
|------|--------------------|
| 新增列 | `new_col = col1 + col2` |
| 修改现有列 | `col = col * 2` |
| 条件赋值 | `col = where(col &gt; 10, 1, 0)` |
| 筛选行（返回布尔值） | `col1 &gt; 5 and col2 &lt; 10` |

---

## 示例 1：新增或修改列

```python
age + 10
```

将 `age` 列的每个值加 10，结果可更新原列或写入新列。

---

## 示例 2：创建新列并赋值

```python
new_column = salary * 1.1
```

创建名为 `new_column` 的新列，其值为 `salary` 列的 1.1 倍。

---

## 示例 3：条件筛选与赋值

```python
bonus = where(age &gt; 30, salary * 0.2, salary * 0.1)
```

含义：若 age 大于 30，则 bonus 为 salary 的 20%；否则为 10%。

---

## 示例 4：字符串匹配（用于筛选）

```python
name.str.contains(&quot;John&quot;)
```

可用于筛选 name 中包含 &quot;John&quot; 的行。""",

    # ---- DataFrameExportRangeSelectDialog (UI) ----
    ("DataFrameExportRangeSelectDialog", "Export Setting"): "导出设置",
    ("DataFrameExportRangeSelectDialog", "Select Export Range"): "选择导出范围",
    ("DataFrameExportRangeSelectDialog", "Export All"): "导出全部",
    ("DataFrameExportRangeSelectDialog", "Export Selected"): "导出所选",

    # ---- DataFrameQueryDatasDialog (UI) ----
    ("DataFrameQueryDatasDialog", "You can refer to column names that are not valid Python variable names by surrounding them in backticks. Column names containing spaces or punctuations (besides underscores) or starting with digits must be surrounded by backticks. (For example, a column named “Area (cm^2)” would be referenced as `Area (cm^2)`). Column names which are Python keywords (like “list”, “for”, “import”, etc) cannot be used. For example, if one of your columns is called a a and you want to compare it with b, your query should be `a a` &gt; b."):
        "可以用反引号包围不合法的 Python 列名。包含空格或下划线以外标点、或以数字开头的列名必须用反引号包围。（例如名为 “Area (cm^2)” 的列应写为 `Area (cm^2)`）。Python 关键字（如 “list”、“for”、“import” 等）不能作为列名使用。例如，如果某列名为 a a，想与 b 比较，query 表达式应写为 `a a` &gt; b。",
    ("DataFrameQueryDatasDialog", "The query string to evaluate"): "要求值的查询字符串",
    ("DataFrameQueryDatasDialog", "OK"): "确定",
    ("DataFrameQueryDatasDialog", "Explanation："): "说明：",
    ("DataFrameQueryDatasDialog", """Using the **Query Data** feature, you can filter data using expressions:

1  **Comparison Operators**: Supports `==`, `&gt;`, `&lt;`, `&gt;=`, `&lt;=`, `!=` for direct comparison of column names and values.  
   **Example**:  
   `A &gt; 2 &amp; B &lt; 8` filters rows where the value in column **A** is greater than **2** and the value in column **B** is less than **8**.

2  **Inter-Column Comparisons**: Directly compare values between columns.  
   **Example**:  
   `A &gt; B` filters rows where the value in column **A** is greater than the value in column **B**.

3  **Logical Operators**: Supports `and`, `or`, `not`, `in`, and `not in` for simplified multi-condition filtering.
   **Examples**:  
   - `A &gt; 2 and B &lt; 8` filters rows where **A** &gt; 2 and **B** &lt; 8.  
   - `A in (&quot;S&quot;, &quot;C&quot;)` filters rows where **A** is either &quot;S&quot; or &quot;C&quot;.

4  **Arithmetic and Complex Logic**: Allows arithmetic operations and complex logical expressions.  
   **Example**:  
   `(A * 3 &gt; 1) | ((B + 12.5) &lt; 5)`.

5  **Range Filtering with `between`**: Use `between` to filter numeric ranges.  
   **Example**:  
   `A.between(2, 8)` filters values in column **A** between **2** and **8**.

6  **String Operations with `str` Methods**: Supports string column processing (e.g., length, prefix matching).  
   **Example**:  
   `Ticket.str.startswith(&quot;A&quot;)` filters rows where the **Ticket** column starts with &quot;A&quot;.

**Note**:  
If a column name contains spaces or special characters, enclose it in backticks (`` ` ``), e.g., `` `Embarked On` ``."""):
        """使用 **数据查询** 功能，可以通过表达式筛选数据：

1  **比较运算符**：支持 `==`、`&gt;`、`&lt;`、`&gt;=`、`&lt;=`、`!=`，用于直接比较列名和值。  
   **示例**：  
   `A &gt; 2 &amp; B &lt; 8` 筛选 **A** 列值大于 **2** 且 **B** 列值小于 **8** 的行。

2  **列间比较**：直接比较不同列之间的值。  
   **示例**：  
   `A &gt; B` 筛选 **A** 列值大于 **B** 列值的行。

3  **逻辑运算符**：支持 `and`、`or`、`not`、`in`、`not in`，简化多条件筛选。  
   **示例**：  
   - `A &gt; 2 and B &lt; 8` 筛选 **A** &gt; 2 且 **B** &lt; 8 的行。  
   - `A in (&quot;S&quot;, &quot;C&quot;)` 筛选 **A** 为 &quot;S&quot; 或 &quot;C&quot; 的行。

4  **算术与复杂逻辑**：支持算术运算和复杂逻辑表达式。  
   **示例**：  
   `(A * 3 &gt; 1) | ((B + 12.5) &lt; 5)`。

5  **使用 `between` 进行范围筛选**：用 `between` 筛选数值范围。  
   **示例**：  
   `A.between(2, 8)` 筛选 **A** 列值介于 **2** 和 **8** 之间的行。

6  **使用 `str` 方法的字符串操作**：支持字符串列处理（如长度、前缀匹配）。  
   **示例**：  
   `Ticket.str.startswith(&quot;A&quot;)` 筛选 **Ticket** 列以 &quot;A&quot; 开头的行。

**注意**：  
若列名包含空格或特殊字符，需用反引号包围（`` ` ``），如 `` `Embarked On` ``。""",

    # ---- DataframeExportSettingsDialog (UI) ----
    ("DataframeExportSettingsDialog", "Export Data Setting"): "导出数据设置",
    ("DataframeExportSettingsDialog", "Select Folder To Export"): "选择导出文件夹",
    ("DataframeExportSettingsDialog", "Export All"): "导出全部",
    ("DataframeExportSettingsDialog", "Export Selected"): "导出所选",
    ("DataframeExportSettingsDialog", "Feather"): "Feather",
    ("DataframeExportSettingsDialog", "Comma-separated values - Universal text format. Best for data exchange and basic analysis."): "逗号分隔值 - 通用文本格式。最适合数据交换和基础分析。",
    ("DataframeExportSettingsDialog", "Column-oriented binary format. High compression and fast querying. Perfect for big data and analytics."): "列式二进制格式。高压缩比，查询速度快。适合大数据和分析场景。",
    ("DataframeExportSettingsDialog", "JSON"): "JSON",
    ("DataframeExportSettingsDialog", "Microsoft Excel format. Supports multiple sheets and formatting."): "Microsoft Excel 格式。支持多表和格式设置。",
    ("DataframeExportSettingsDialog", "Python-specific binary format. Preserves complete object structure. Best for temporary storage within Python applications."): "Python 专用二进制格式。保留完整的对象结构。最适合在 Python 应用内临时存储。",
    ("DataframeExportSettingsDialog", "Lightweight binary format. Extremely fast read/write speeds. Great for intermediate data storage and Python/R interoperability."): "轻量级二进制格式。读写速度极快。适合中间数据存储以及 Python/R 互操作。",
    ("DataframeExportSettingsDialog", "Pickle"): "Pickle",
    ("DataframeExportSettingsDialog", "HTML"): "HTML",
    ("DataframeExportSettingsDialog", "csv"): "csv",
    ("DataframeExportSettingsDialog", "xlsx"): "xlsx",
    ("DataframeExportSettingsDialog", "Parquet"): "Parquet",
    ("DataframeExportSettingsDialog", "JavaScript Object Notation. Human-readable, web-friendly format. Excellent for web APIs and configuration files."): "JavaScript 对象表示法。人类可读、Web 友好的格式。非常适合 Web API 和配置文件。",
    ("DataframeExportSettingsDialog", "Web page format. Preserves table styling. Ideal for embedding data in reports or emails."): "网页格式。保留表格样式。适合在报告或邮件中嵌入数据。",

    # ---- main ----
    ("main", "version:%1,compile datetime:%2,enable python:%3"): "版本:%1,编译时间:%2,启用 Python:%3",
}
