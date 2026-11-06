# DAWorkbench 测试运行脚本 —— 按套件 / 按改动范围选择测试
#
# 测试按"套件"分组（套件标签在 src/tst/<Name>/CMakeLists.txt 的 da_add_test(... SUITE x) 里声明）：
#   core     —— 基础层（DAShared/DAUtils/DAMessageHandler/DAPyBindQt/DAPyScripts）与 APP
#   data     —— 数据容器 / DataFrame 绑定 / 惰性表接入
#   table    —— 表格（模型、视图态、列/冻结、排序查找、条件格式、计算列、拆分合并、校验、批注）
#   chart    —— 图表 / Figure / 导出 / 控件样式
#   agent    —— DAAgent 框架、权限、会话、子 Agent
#   workflow —— 工作流与图形视图（**目前无测试**，见 docs/zh/dev-guide/general/testing.md）
#
# 性能/基准用例（目标名含 Perf/Benchmark）带 bench 标签，默认不跑；-Bench 才纳入。
#
# 用法：
#   .\scripts\test.ps1 -Suite table        # 只构建并运行表格套件
#   .\scripts\test.ps1 -Changed            # 按 git 改动自动挑套件（默认对比 dev）
#   .\scripts\test.ps1 -All                # 全量（不含 bench）
#   .\scripts\test.ps1 -All -Bench         # 全量 + 性能用例
#   .\scripts\test.ps1 -List               # 列出套件与测试，不构建不运行
#   .\scripts\test.ps1 -Suite table -NoBuild   # 只跑，不重新构建
#
# 说明：测试可执行文件与 DA DLL 输出到同一个 build/bin/<Config> 目录，
# 因此**不要在测试运行期间构建**（会因 DLL 被占用报 LNK1104）。本脚本总是先构建再运行。

param(
    [string]$Suite = '',
    [switch]$Changed,
    [switch]$All,
    [switch]$Bench,
    [switch]$List,
    [switch]$NoBuild,

    [int]$Jobs = 0,
    [string]$BuildDir = '',
    [string]$Base = '',
    [string]$Ctest = '',

    [ValidateSet('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Config = 'Release'
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path $PSScriptRoot -Parent
if (-not $BuildDir) {
    $BuildDir = Join-Path $projectRoot 'build'
}

# 套件清单（与各测试 CMakeLists.txt 的 SUITE 声明一致）
$KnownSuites = @('core', 'data', 'table', 'chart', 'agent', 'workflow')
# 参与 -All / -Changed 兜底的套件（workflow 目前无测试，保留在清单里便于将来启用）
$ActiveSuites = @('core', 'data', 'table', 'chart', 'agent')

# ============================================================
# 工具函数
# ============================================================
function Find-Ctest {
    if ($Ctest) {
        return $Ctest
    }
    $cmd = Get-Command ctest -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }
    $cm = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cm -and (Test-Path -PathType Leaf -LiteralPath $cm.Source)) {
        $dir = Split-Path -LiteralPath $cm.Source -Parent
        foreach ($candidate in @('ctest.exe', 'ctest')) {
            $p = Join-Path $dir $candidate
            if (Test-Path -PathType Leaf -LiteralPath $p) {
                return $p
            }
        }
    }
    throw 'ctest 未找到：请把 CMake/CTest 加入 PATH，或用 -Ctest 指定路径'
}

# 用 ctest -N [-L label] 列出测试名（不执行）
# 多配置生成器（VS）下**必须**带 -C：标签挂在按配置生成的 add_test 上，不指定配置时
# ctest 既解析不到测试也解析不到 LABELS，表现为"套件里 0 个测试"。
function Get-TestNames {
    param([string]$Label = '')

    $ctestArgs = @('-N', '-C', $script:Config)
    if ($Label) {
        $ctestArgs += @('-L', $Label)
    }
    $output = & $script:CtestExe @ctestArgs 2>&1
    $names = @()
    foreach ($line in $output) {
        if ($line -match 'Test\s+#\d+:\s+(\S+)') {
            $names += $Matches[1]
        }
    }
    return $names
}

# 改动文件 → 套件。
# 返回 [pscustomobject]@{ Kind = 'suites' | 'none' | 'unmapped'; Suites = <string[]> }。
# 用对象而不是裸数组返回：PowerShell 会把空数组 unroll 成 $null，调用方无法区分
# "已知无测试覆盖"（可以放心跳过）与"规则未命中"（必须保守地全跑）。
function New-SuiteResult {
    param([string]$Kind, [string[]]$Suites = @())
    return [pscustomobject]@{ Kind = $Kind; Suites = $Suites }
}

function Get-SuitesForFile {
    param([string]$Path)

    $p = $Path -replace '\\', '/'

    # 测试自身 / 构建脚本改动：全跑（保守）
    if ($p -match '^src/tst/' -or $p -match '^cmake/' -or $p -match '^CMakeLists\.txt$' -or $p -match '^src/CMakeLists\.txt$') {
        return New-SuiteResult -Kind 'suites' -Suites $ActiveSuites
    }
    # 信号分析插件有 C++ 测试（SUITE data）与 Python 测试；改动必须选中 data 套件
    if ($p -match '^plugins/DASignalAnalysis/') {
        return New-SuiteResult -Kind 'suites' -Suites @('data')
    }
    # 已知无测试覆盖的区域
    if ($p -match '^(docs|scripts|requirements\.txt|\.github)/' -or $p -match '^plugins/' -or
        $p -match '^src/(DAPyWorkFlow|DAGraphicsView|DAPluginSupport|DAInterface|DAAxOfficeWrapper)/' -or
        $p -match '^src/3rdparty/') {
        return New-SuiteResult -Kind 'none'
    }
    # 翻译资源与配置生成物：不参与测试选择。
    # 工作树里 src/DAConfigs.h 与 src/i18n/backup-da_zh_CN.ts 是**固定噪声**（配置期重新生成 /
    # 换行符差异），若把它们当"未命中"处理，-Changed 会永远回落到全量，套件选择形同虚设。
    if ($p -match '^src/i18n/' -or $p -eq 'src/DAConfigs.h' -or $p -match '\.(ts|qm|po)$') {
        return New-SuiteResult -Kind 'none'
    }
    # 基础层：上层全都依赖，全跑
    if ($p -match '^src/(DAShared|DAUtils|DAMessageHandler|DAPyBindQt|DAPyScripts)/') {
        return New-SuiteResult -Kind 'suites' -Suites $ActiveSuites
    }
    if ($p -match '^src/DAData/') {
        return New-SuiteResult -Kind 'suites' -Suites @('data', 'table', 'chart')
    }
    if ($p -match '^src/DAFigure/') {
        return New-SuiteResult -Kind 'suites' -Suites @('chart', 'table')
    }
    if ($p -match '^src/DAAgent/' -or $p -match '^src/DAGui/Agent/' -or $p -match '^src/PyScripts/DAWorkbench/agent/') {
        return New-SuiteResult -Kind 'suites' -Suites @('agent')
    }
    # DAGui 已按领域收敛：Chart/ 跑图表套件，Table/ 跑表格套件
    if ($p -match '^src/DAGui/Chart/') {
        return New-SuiteResult -Kind 'suites' -Suites @('chart')
    }
    if ($p -match '^src/DAGui/Table/') {
        return New-SuiteResult -Kind 'suites' -Suites @('table')
    }
    # 数据管理面板是独立窗口，同时服务表格页与图表添加向导
    if ($p -match '^src/DAGui/DataManager/') {
        return New-SuiteResult -Kind 'suites' -Suites @('table', 'chart')
    }
    # 其余 DAGui 代码（Workflow/Common/Archive 等）：三个上层套件都可能用到，全部跑（保守兜底）
    if ($p -match '^src/DAGui/') {
        return New-SuiteResult -Kind 'suites' -Suites @('table', 'chart', 'agent')
    }
    return New-SuiteResult -Kind 'unmapped'
}

# 调用 git 并返回 @{ Ok = <bool>; Lines = <string[]> }。
# 用对象而不是裸数组返回：PowerShell 会把空数组 unroll 成 $null，调用方无法区分
# "命令失败" 和 "命令成功但没有输出"（-Changed 的基线探测正需要这个区分）。
# 另外 Windows PowerShell 5.1 下 $ErrorActionPreference='Stop' 会把原生命令的 stderr
# （例如 git 的 CRLF 警告）当成终止性错误，这里必须临时降级为 Continue。
function Invoke-GitLines {
    param([string[]]$GitArgs)

    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $out = & git -C $projectRoot @GitArgs 2>$null
        $ok = ($LASTEXITCODE -eq 0)
        $lines = @($out | Where-Object { $_ })
        return [pscustomobject]@{ Ok = $ok; Lines = $lines }
    }
    finally {
        $ErrorActionPreference = $prev
    }
}

function Get-ChangedFiles {
    param([string]$BaseRef)

    $files = New-Object System.Collections.Generic.List[string]

    if ($BaseRef) {
        $diffRes = Invoke-GitLines -GitArgs @('diff', '--name-only', "$BaseRef...HEAD")
        if (-not $diffRes.Ok) {
            Write-Host "[WARN] 基线 '$BaseRef' 不可用，只比较工作区改动" -ForegroundColor Yellow
        }
        else {
            foreach ($f in $diffRes.Lines) { $files.Add($f) }
        }
    }
    foreach ($f in (Invoke-GitLines -GitArgs @('diff', '--name-only')).Lines) { $files.Add($f) }
    foreach ($f in (Invoke-GitLines -GitArgs @('ls-files', '--others', '--exclude-standard')).Lines) { $files.Add($f) }

    return ($files | Select-Object -Unique)
}

# ============================================================
# 初始化：检查构建目录 + 读取测试清单
# ============================================================
$testFile = Join-Path $BuildDir 'CTestTestfile.cmake'
if (-not (Test-Path -LiteralPath $testFile)) {
    throw "构建目录未启用测试：$BuildDir 下没有 CTestTestfile.cmake。请用 -DDA_ENABLE_TESTING=ON 重新配置。"
}

$script:CtestExe = Find-Ctest

Push-Location $BuildDir
try {
    $allTests = @(Get-TestNames)
    $benchTests = @(Get-TestNames -Label 'bench')
    $suiteTests = @{}
    foreach ($s in $KnownSuites) {
        $suiteTests[$s] = @(Get-TestNames -Label "suite:$s")
    }
}
finally {
    Pop-Location
}

if ($allTests.Count -eq 0) {
    throw "构建目录里没有任何已注册的测试：$BuildDir（先构建测试目标，或 scripts\build.ps1 -Target run_all_tests）"
}

# --- LIST ---
if ($List) {
    Write-Host "`n测试套件（* = bench，默认不跑）" -ForegroundColor Cyan
    foreach ($s in $KnownSuites) {
        $names = $suiteTests[$s]
        if ($names.Count -eq 0) {
            Write-Host ("  {0,-10} （无测试）" -f $s) -ForegroundColor DarkYellow
            continue
        }
        Write-Host ("  {0,-10} {1} 个" -f $s, $names.Count) -ForegroundColor Green
        foreach ($n in $names) {
            $mark = ''
            if ($benchTests -contains $n) { $mark = ' *' }
            Write-Host ("      - {0}{1}" -f $n, $mark)
        }
    }
    $assigned = @()
    foreach ($s in $KnownSuites) { $assigned += $suiteTests[$s] }
    $noSuite = $allTests | Where-Object { $assigned -notcontains $_ }
    if ($noSuite) {
        Write-Host "`n未归入任何套件的测试（配置期应已告警）：" -ForegroundColor Yellow
        $noSuite | ForEach-Object { Write-Host ("      - {0}" -f $_) -ForegroundColor Yellow }
    }
    Write-Host ''
    exit 0
}

if ($Changed -and $Suite) {
    throw '-Changed 与 -Suite 不能同时使用（-Changed 会自动推出套件）'
}

$selectedSuites = New-Object System.Collections.Generic.List[string]
$reason = ''

# --- CHANGED ---
if ($Changed) {
    $baseRef = $Base
    if (-not $baseRef) { $baseRef = 'dev' }
    $changedFiles = @(Get-ChangedFiles -BaseRef $baseRef)
    if ($changedFiles.Count -eq 0) {
        Write-Host "[INFO] 没有检测到改动（基线 $baseRef），按全量处理" -ForegroundColor Yellow
        foreach ($s in $ActiveSuites) { $selectedSuites.Add($s) }
        $reason = '-Changed（无改动 → 全量）'
    }
    else {
        $unmapped = @()
        $uncovered = @()
        foreach ($f in $changedFiles) {
            $res = Get-SuitesForFile -Path $f
            if ($res.Kind -eq 'unmapped') {
                $unmapped += $f
                continue
            }
            if ($res.Kind -eq 'none') {
                $uncovered += $f
                continue
            }
            foreach ($s in $res.Suites) {
                if (-not $selectedSuites.Contains($s)) { $selectedSuites.Add($s) }
            }
        }
        if ($unmapped.Count -gt 0) {
            Write-Host "[WARN] 以下改动没有匹配到套件，按全量处理（宁可多跑）：" -ForegroundColor Yellow
            $unmapped | ForEach-Object { Write-Host ("        {0}" -f $_) -ForegroundColor Yellow }
            foreach ($s in $ActiveSuites) {
                if (-not $selectedSuites.Contains($s)) { $selectedSuites.Add($s) }
            }
        }
        if ($uncovered.Count -gt 0) {
            Write-Host "[INFO] 以下改动所在模块没有测试覆盖（不影响本次选择）：" -ForegroundColor DarkYellow
            $uncovered | Select-Object -First 20 | ForEach-Object { Write-Host ("        {0}" -f $_) -ForegroundColor DarkYellow }
            if ($uncovered.Count -gt 20) {
                Write-Host ("        ... 其余 {0} 个" -f ($uncovered.Count - 20)) -ForegroundColor DarkYellow
            }
        }
        $reason = ("-Changed（基线 {0}，{1} 个文件）" -f $baseRef, $changedFiles.Count)
    }
}
# --- 显式套件 ---
elseif ($Suite -and -not $All) {
    if ($Suite -eq 'all') {
        foreach ($s in $ActiveSuites) { $selectedSuites.Add($s) }
        $reason = '-Suite all'
    }
    else {
        if ($KnownSuites -notcontains $Suite) {
            throw ("未知套件 '{0}'，可用：{1}" -f $Suite, ($KnownSuites -join ', '))
        }
        $selectedSuites.Add($Suite)
        $reason = "-Suite $Suite"
    }
}
# --- 默认：全量 ---
else {
    foreach ($s in $ActiveSuites) { $selectedSuites.Add($s) }
    $reason = '-All'
}

# --- 汇总选中的测试 ---
$selected = @()
$skippedBench = @()
foreach ($s in $selectedSuites) {
    foreach ($n in $suiteTests[$s]) {
        if ($benchTests -contains $n) {
            if ($skippedBench -notcontains $n) { $skippedBench += $n }
            if (-not $Bench) { continue }
        }
        if ($selected -notcontains $n) { $selected += $n }
    }
}

if ($selected.Count -eq 0) {
    if ($skippedBench.Count -gt 0) {
        Write-Host "[OK] 选中的套件里只有 bench 测试，加 -Bench 才运行：$($skippedBench -join ', ')" -ForegroundColor Green
    }
    else {
        Write-Host '[OK] 没有需要运行的测试' -ForegroundColor Green
    }
    exit 0
}

# ============================================================
# 构建
# ============================================================
Write-Host "`n=== DAWorkbench 测试 ===" -ForegroundColor Cyan
Write-Host ("  选择依据: {0}" -f $reason)
Write-Host ("  套件:     {0}" -f ($selectedSuites -join ', '))
Write-Host ("  测试:     {0} 个" -f $selected.Count)
if (-not $Bench -and $skippedBench.Count -gt 0) {
    Write-Host ("  已跳过:   {0} 个 bench（-Bench 可运行）: {1}" -f $skippedBench.Count, ($skippedBench -join ', ')) -ForegroundColor DarkGray
}
Write-Host ''

if (-not $NoBuild) {
    Write-Host '[BUILD] 构建选中的测试目标...' -ForegroundColor Cyan
    $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmakeCmd) {
        throw 'cmake 未找到：请加入 PATH'
    }
    $buildArgs = @('--build', $BuildDir, '--config', $Config, '--target') + $selected
    & $cmakeCmd.Source @buildArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] 构建失败（退出码 $LASTEXITCODE）" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Host '[OK] 构建完成' -ForegroundColor Green
}
else {
    Write-Host '[SKIP] -NoBuild：直接使用现有可执行文件（陈旧时可能 0xc0000139/崩溃，去掉该参数即可）' -ForegroundColor Yellow
}

# ============================================================
# 运行
# ============================================================
if ($Jobs -le 0) {
    $Jobs = [Math]::Min(8, [Environment]::ProcessorCount)
    if ($Jobs -lt 1) { $Jobs = 1 }
}

# 基准用例与普通测试分两阶段跑，基准阶段强制串行：
# 1) 并行会让测量值失真（同一台机器上并排跑 1M 行夹具，耗时没有可比性）；
# 2) 基准的峰值内存 1 GB 起步，和别的测试并行会被内存压力拖成"进程活着但几乎不占 CPU"
#    的假死（实测 -j8 时 DATableUndoBenchmark 卡住十分钟以上，单跑 4.6 秒）。
$benchSelected  = @($selected | Where-Object { $benchTests -contains $_ })
$normalSelected = @($selected | Where-Object { $benchTests -notcontains $_ })

function Invoke-CtestSelection {
    param([string[]]$Names, [int]$Parallelism, [string]$Title)

    if ($Names.Count -eq 0) {
        return 0
    }
    $pat = '^(' + (($Names | ForEach-Object { [regex]::Escape($_) }) -join '|') + ')$'
    Write-Host "`n[TEST] $Title：ctest -j$Parallelism（$($Names.Count) 个）" -ForegroundColor Cyan
    Push-Location $BuildDir
    try {
        # Out-Host：把 ctest 的输出直接送到控制台，不要让它进入本函数的返回值
        # （原生命令的 stdout 属于成功流，会被一并当成函数返回值收集，导致 $LASTEXITCODE
        # 变成一个「输出行 + 退出码」的数组，后续 -ne 0 判断永远为真）
        & $script:CtestExe -C $Config -j $Parallelism --output-on-failure --no-tests=error -R $pat | Out-Host
        return $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
}

$testExitCode = 0

$code = Invoke-CtestSelection -Names $normalSelected -Parallelism $Jobs -Title '普通测试'
if ($code -ne 0) { $testExitCode = $code }

if ($testExitCode -eq 0 -and $benchSelected.Count -gt 0) {
    $code = Invoke-CtestSelection -Names $benchSelected -Parallelism 1 -Title '基准测试（串行）'
    if ($code -ne 0) { $testExitCode = $code }
}

if ($testExitCode -ne 0) {
    Write-Host "`n[ERROR] 测试失败（退出码 $testExitCode）" -ForegroundColor Red
    exit $testExitCode
}

Write-Host "`n[OK] 测试通过：$($selected -join ', ')" -ForegroundColor Green
