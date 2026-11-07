# ==============================================================================
# 发布包完整性校验
#
# 为什么需要它：install + windeployqt 跑完不代表包是对的。已经踩过的坑：
#   - windeployqt 不加 --compiler-runtime，MSVC 运行库不会进包（用户机器 0xc000007b）
#   - qwt 的 quazip 依赖 zd.dll，而白名单里原来写的是 zlib —— 文件根本不会落盘
#   - QtWebEngine 的 resources / QtWebEngineProcess.exe 漏了，WebEngine 打不开
#   - python 目录没进包或 python-config.json 没写，启动时初始化 Python 直接失败
#
# 用法：
#   .\scripts\verify_package.ps1 -Stage <安装目录>
#   .\scripts\verify_package.ps1 -Stage <安装目录> -RunSmoke
#
# 退出码非 0 表示包不完整，CI 直接判失败，避免把残包发出去。
# ==============================================================================
param(
    [Parameter(Mandatory = $true)]
    [string]$Stage,

    # 期望的 Python DLL 名（python312.dll 等），默认按 3.12
    [string]$PythonDll = 'python312.dll',

    # 是否执行启动冒烟（会真的跑一次 DAWorkBench.exe --version）
    [switch]$RunSmoke
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Stage)) {
    Write-Host "::error::安装目录不存在: $Stage"
    exit 1
}
$Stage = (Resolve-Path $Stage).Path
Write-Host "校验目录: $Stage"

$missing = @()

function Test-Needed {
    param([string[]]$RelativePaths)
    foreach ($rel in $RelativePaths) {
        $full = Join-Path $Stage $rel
        if (-not (Test-Path $full)) {
            $script:missing += $rel
        }
    }
}

# --- 主程序 ---------------------------------------------------------------
Test-Needed @('DAWorkBench.exe')

# --- Qt 运行时（Core/Gui/Widgets 之外，WebEngine 那串是最大的坑）------------
Test-Needed @(
    'Qt6Core.dll',
    'Qt6Gui.dll',
    'Qt6Widgets.dll',
    'Qt6Svg.dll',
    'Qt6Xml.dll',
    'Qt6Network.dll',
    'Qt6PrintSupport.dll',
    'Qt6Concurrent.dll',
    'Qt6OpenGL.dll',
    'Qt6OpenGLWidgets.dll',
    'Qt6Core5Compat.dll',
    'Qt6Multimedia.dll',
    'Qt6WebEngineCore.dll',
    'Qt6WebEngineWidgets.dll',
    'Qt6WebChannel.dll',
    'QtWebEngineProcess.exe'
)

# --- MSVC 运行库（windeployqt --compiler-runtime） -------------------------
Test-Needed @(
    'msvcp140.dll',
    'vcruntime140.dll',
    'vcruntime140_1.dll'
)

# --- Python 运行时 --------------------------------------------------------
Test-Needed @(
    $PythonDll,
    'python-config.json',
    'python\python.exe',
    'python\Lib\site-packages\pandas\__init__.py',
    'python\Lib\site-packages\numpy\__init__.py'
)

# --- 第三方库（windeployqt 不认识，必须自己拷） ---------------------------
Test-Needed @(
    'SARibbonBar.dll',
    'DAWidgets.dll',
    'qwtcore.dll',
    'qwtplot.dll',
    'qwtplot3d.dll',
    'qtadvanceddocking-qt6.dll',
    'quazip1-qt6.dll',
    'zlib1.dll',
    'zd.dll',
    'spdlog.dll'
)

# --- 随包 Python 脚本与翻译 ----------------------------------------------
Test-Needed @('PyScripts\DAWorkbench\__init__.py')

$pluginCount = @(Get-ChildItem (Join-Path $Stage 'plugins') -Filter *.dll -ErrorAction SilentlyContinue).Count
if ($pluginCount -lt 1) { $missing += 'plugins\*.dll（一个都没有）' }
$qmCount = @(Get-ChildItem (Join-Path $Stage 'translations') -Filter *.qm -ErrorAction SilentlyContinue).Count
if ($qmCount -lt 1) { $missing += 'translations\*.qm（一个都没有）' }

# --- WebEngine 的资源目录（windeployqt 会生成，漏了 WebEngine 直接白屏）----
$qtwebengineRes = @(Get-ChildItem (Join-Path $Stage 'resources') -Filter *.pak -ErrorAction SilentlyContinue).Count
if ($qtwebengineRes -lt 1) { $missing += 'resources\*.pak（QtWebEngine 资源缺失）' }

# --- 汇总 ----------------------------------------------------------------
if ($missing.Count -gt 0) {
    Write-Host ''
    Write-Host "::error::发布包缺少 $($missing.Count) 个必需项："
    foreach ($m in $missing) { Write-Host "  - $m" }
    exit 1
}

$totalMB = [math]::Round(((Get-ChildItem $Stage -Recurse -File | Measure-Object Length -Sum).Sum / 1MB), 1)
Write-Host ''
Write-Host "必需项齐全（插件 $pluginCount 个，翻译 $qmCount 个），目录总计 $totalMB MB"

# --- 启动冒烟 ------------------------------------------------------------
if ($RunSmoke) {
    Write-Host ''
    Write-Host '--- 启动冒烟：DAWorkBench.exe --version ---'
    $exe = Join-Path $Stage 'DAWorkBench.exe'
    $outFile = Join-Path $env:RUNNER_TEMP 'pkg-smoke.out'
    $errFile = Join-Path $env:RUNNER_TEMP 'pkg-smoke.err'
    # 关键：工作目录设成包目录，让 Windows 从包内解析 DLL
    $proc = Start-Process -FilePath $exe -ArgumentList '--version' -WorkingDirectory $Stage -PassThru `
        -NoNewWindow -RedirectStandardOutput $outFile -RedirectStandardError $errFile

    if (-not $proc.WaitForExit(60000)) {
        $proc.Kill()
        Write-Host '::error::--version 60 秒未退出（可能弹窗或事件循环卡住）'
        exit 1
    }

    $stdout = if (Test-Path $outFile) { Get-Content $outFile -Raw } else { '' }
    $stderr = if (Test-Path $errFile) { Get-Content $errFile -Raw } else { '' }
    Write-Host "exit=$($proc.ExitCode)"
    if ($stdout) { Write-Host $stdout.Trim() }
    if ($stderr) { Write-Host $stderr.Trim() }

    # 0xC0000135 = STATUS_DLL_NOT_FOUND，说明包内依赖没齐，必须拦下
    if ($proc.ExitCode -eq -1073741515) {
        Write-Host '::error::启动失败：STATUS_DLL_NOT_FOUND（包内还有缺失的 DLL）'
        exit 1
    }
    if ($proc.ExitCode -ne 0) {
        Write-Host "::warning::--version 退出码非 0（$($proc.ExitCode)），请人工确认"
    } elseif (-not $stdout) {
        Write-Host '::warning::--version 没有输出（GUI 子系统程序在 CI 下可能拿不到 stdout，仅供参考）'
    } else {
        Write-Host '启动冒烟通过'
    }
}
