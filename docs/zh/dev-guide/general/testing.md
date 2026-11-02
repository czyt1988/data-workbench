# 测试体系与按套件运行

> 本文说明 `src/tst/` 下测试如何分组、"改动后该跑哪些测试"怎么定，以及新增测试时必须遵守的约定。
> 命令入口是 [scripts/test.ps1](../../../../scripts/test.ps1)。

## 1. 为什么按套件跑

测试目录里现在有近 30 个独立测试可执行文件（一个 `src/tst/<Name>/` 一个 exe、一个 Qt Test 类）。全量跑一遍并不慢，**慢的是历史遗留的两个坑**：

- `DATableDataTest` 一个文件里塞了 125 个用例，其中 7 个是 10 万/100 万行的性能用例。其中一个（`testSelectAllLargeSelectionPerf`）会撞穿 Qt Test 的 5 分钟看门狗 —— 实测该测试跑一次 **331 秒**，其中 300 秒耗在一个用例上，且**必然为红**。
- `DATableUndoBenchmark` 同样是基准测试，运行 312 秒后超时。

后果是"跑一遍测试"变成 5 分钟起步且必红，于是没人跑，回归保护形同虚设。

现在的做法：**把性能/基准用例摘出默认套件 + 给测试打套件标签 + 按改动选套件**。实测表格相关改动从"6 分钟以上"降到"约 1 分钟"。

## 2. 两分钟上手

```powershell
# 只跑表格套件（会先构建选中的测试目标，再运行）
.\scripts\test.ps1 -Suite table

# 按 git 改动自动挑套件（默认对比 dev；含工作区未提交改动）
.\scripts\test.ps1 -Changed

# 全量（不含 bench）
.\scripts\test.ps1 -All

# 全量 + 性能/基准用例
.\scripts\test.ps1 -All -Bench

# 只看套件与测试清单，不构建不运行
.\scripts\test.ps1 -List

# 只运行、不重新构建（陈旧 exe 会以 0xc0000139/崩溃形式假红，谨慎使用）
.\scripts\test.ps1 -Suite table -NoBuild
```

`scripts/build.ps1 -Test` 仍然可用，它等价于"构建完成后执行 `test.ps1 -All`"（不含 bench）。注意 `build.ps1 -Target <模块>` 只影响**构建阶段**，不影响测试选择 —— 想只跑某个领域请用 `test.ps1 -Suite`。

## 3. 套件清单

套件标签由各测试 `CMakeLists.txt` 里的 `da_add_test(... SUITE <name>)` 声明，脚本用 `ctest -L suite:<name>` 读取。

| 套件 | 覆盖范围 | 包含的测试 |
|------|----------|------------|
| `core` | 基础层与 APP | DASharedTest、DAShortcutManagerTest、DASlowLogTest、DAAppConfigTest、MessageHandleTest、APPRefactorTest |
| `data` | 数据容器 / DataFrame 绑定 / 惰性表接入 | DADataFrameTest、DADataLinkTableTest |
| `table` | 表格（模型、视图态、列与冻结、排序查找、条件格式、计算列、拆分合并、校验、批注） | DATableDataTest、DATableNavigationTest、DATableExporterTest、DATableQuickPlotTest（+ bench：DATablePerfTest、DATableUndoBenchmark） |
| `chart` | 图表 / Figure / 导出 / 控件样式 | Chart3DIntegrationTest、DAChartSeriesDataTest、DAStatsGuideTest、DAFigurePointerToolTest、DARichTextEditTest、DAMarkdownExporterTest、DASpinBoxStyleTest、Qwt3DRTTITest |
| `agent` | DAAgent 框架、权限、会话、子 Agent | DAAgentBridgeTest、DAAgentConfigTest、DAAgentModuleTest、DAAgentPermissionManagerTest、DAAgentSessionStoreTest、DAAgentSubagentManagerTest、DAAgentToolSpecTest |
| `workflow` | 工作流与图形视图 | **目前为空** —— 没有任何测试链接 `DAPyWorkFlow` / `DAGraphicsView`，这是已知覆盖空洞 |

> **⚠️ 新增测试必须写 `SUITE`**
> 漏写不会导致构建失败，但该测试不会出现在任何套件里，按套件跑测试时会被静默漏掉。配置期会对未声明 `SUITE` 的测试打印 `WARNING`（见 `src/tst/CMakeLists.txt`）。

## 4. bench：性能与基准用例

- **判定规则**：测试目标名包含 `Perf` 或 `Benchmark` → 自动打上 `bench` 标签，并把 ctest `TIMEOUT` 放宽到 600 秒。规则在 `cmake/DAWorkbench.cmake` 的 `da_add_test` 里，改规则要同步改 `test.ps1`。
- **默认不跑**：`test.ps1` 的 `-All` / `-Suite` / `-Changed` 都会排除 bench；`-Bench` 才纳入。裸 `ctest` 仍会跑全部（便于发现问题）。
- **超时兜底**：所有测试统一带 ctest `TIMEOUT`（普通 120 秒、bench 600 秒）。将来再写出挂死的用例，代价是 2 分钟而不是 5 分钟。

表格性能用例集中在新拆出的 `src/tst/DATablePerfTest/`：7 个 10 万/100 万行用例，从 `DATableDataTest` 迁出。这样默认套件里的 118 个常规用例只需约 30 秒。

## 5. `-Changed` 的映射规则

脚本把改动文件路径映射到套件，规则在 `scripts/test.ps1` 的 `Get-SuitesForFile`：

| 改动位置 | 选中套件 |
|----------|----------|
| `src/DAShared`、`DAUtils`、`DAMessageHandler`、`DAPyBindQt`、`DAPyScripts`（基础层） | 全部套件 |
| `src/DAData` | `data` + `table` + `chart` |
| `src/DAFigure` | `chart` + `table` |
| `src/DAAgent`、`src/DAGui/Agent`、`src/PyScripts/DAWorkbench/agent` | `agent` |
| `src/DAGui/Chart*`、`Chart3DSetting`、`ChartAddItem`、`ChartSetting` | `chart` |
| `src/DAGui/TableChart`、`DATable*`、`DADataTableModel`、表格相关对话框/命令 | `table` |
| 其余 `src/DAGui` 代码 | `table` + `chart` + `agent`（保守兜底） |
| `src/tst`、`cmake/`、顶层 `CMakeLists.txt` | 全部套件 |
| `docs/`、`scripts/`、`plugins/`、`src/3rdparty`、`src/DAPyWorkFlow`、`src/DAGraphicsView`、`src/DAInterface` 等 | 无测试覆盖，跳过（打印 INFO） |
| 规则未命中 | 全部套件 + `WARN`（宁可多跑，不能漏跑） |

默认基线与 `dev` 比较（`git diff dev...HEAD` + 工作区改动 + 未跟踪文件），可用 `-Base <ref>` 指定别的基线。

> **💡 设计取向**
> 所有"拿不准"的情况都倒向多跑：规则未命中 → 全量；`DAGui` 里认不出归属的文件 → 三个上层套件都跑。漏跑一次（该跑没跑）的代价远大于多跑十次。

## 6. 已知约束与坑

- **测试 exe 与库 DLL 在同一个输出目录**（`build/bin/<Config>`）。测试运行期间 DLL 被占用，此时构建会报 `LNK1104`。`test.ps1` 总是先构建再运行，不要在测试跑着的时候手工构建。
- **陈旧 exe 会假红**：`0xc0000139`（找不到入口点）或启动即崩溃，通常是 exe 比 DLL 旧。重新构建即可；`-NoBuild` 会放大这个问题。
- **嵌入式 Python 的 teardown 崩溃**：链接 Python 的测试不能在 `qExec` 返回后正常走进程退出，否则会在结果已打印后触发访问冲突、被 ctest 记为 SEGFAULT。规范做法见 `DADataFrameTest` / `DATableDataTest` / `DATableUndoBenchmark` / `DATablePerfTest` 里的自定义 `main()`：`qExec` 返回后 `std::fflush(nullptr)`，Windows 上用 `TerminateProcess` 直接结束进程。
- **环境依赖**：没有安装 pandas 时，相关用例会 `QSKIP`（不是失败）。CI 目前**只构建、不跑测试**（`.github/workflows/build.yml`），所以回归保护只在本机生效。
- **已知失败隔离**：`DATableDataTest::testContextMenuComposition` 目前以 `QSKIP` 形式隔离（列头菜单 action 数量与期望不一致，原因待定位）。修复任务单独跟踪，不要把它当作"测试本来就是红的"。

## 7. 新增测试检查清单

1. 新建目录 `src/tst/<Name>/`，用 `da_add_test(NAME <Name> SUITE <suite> ...)` 声明；目标名与目录名保持一致。
2. 性能/基准用例 → 目标名带上 `Perf` 或 `Benchmark`，让它自动进 `bench`。
3. 需要 Python 的用例 → 套用上面第 6 节的 `main()` 模板，否则退出码不可信。
4. 跑一遍 `.\scripts\test.ps1 -List`，确认新测试出现在预期套件里。

## 8. 参考数据（2026-11，本机 Release / Qt 5.15.16）

| 场景 | 改造前 | 改造后 |
|------|--------|--------|
| 表格相关改动 → 跑相关测试 | 6 分钟以上（`DATableDataTest` 单项 331 s） | 约 70–80 秒 |
| 全量默认套件 | 331 s + 312 s（benchmark 超时） | 约 55 秒（不含 bench） |
| 默认套件是否可信 | 3 个红（1 真失败 + 1 段错误 + 1 超时） | 全绿 |
| 按模块跑测试 | 不存在（`build.ps1 -Target DAGui -Test` 直接报错） | `test.ps1 -Suite table` / `-Changed` |
