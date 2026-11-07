# ==============================================================================
# 发布包完整性校验
#
# 设计原则：**按实际导入表推导**，而不是手写"应该有这些文件"的清单。
# 手写清单会照抄开发机的现状（我第一版就把本机 Debug 版 zlib 的 zd.dll 写了进去），
# 换个构建环境立刻误报或漏报。这里改为：
#   1. 用 dumpbin 扫包里每个 exe/dll 的导入表
#   2. 任何导入项必须能在包里找到，或在 C:\Windows\System32 里找到（系统 DLL）
#   3. VC++ 运行库例外：必须随包（不能指望用户机器装了 redist）
# 另外补几类导入表看不出来的东西：Python 运行时、WebEngine 资源、插件、翻译、脚本。
#
# 用法：
#   .\scripts\verify_package.ps1 -Stage <安装目录> [-RunSmoke]
# 退出码非 0 表示包不完整。
# ==============================================================================
param(
    [Parameter(Mandatory = $true)]
    [string]$Stage,

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

$problems = @()

function Add-Missing {
    param([string]$Item)
    if ($script:problems -notcontains $Item) { $script:problems += $Item }
}

# ---------------------------------------------------------------------------
# 1. dumpbin
# ---------------------------------------------------------------------------
$dumpbin = $null
$cmd = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if ($cmd) {
    $dumpbin = $cmd.Source
} else {
    $fi = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($fi) { $dumpbin = $fi.FullName }
}
if (-not $dumpbin) {
    Write-Host '::error::找不到 dumpbin，无法校验导入表'
    exit 1
}

# ---------------------------------------------------------------------------
# 2. 必需的非 DLL 内容（导入表看不出来的东西）
# ---------------------------------------------------------------------------
$fixed = @(
    'DAWorkBench.exe',
    'python-config.json',
    'python\python.exe',
    'python\Lib\site-packages\pandas\__init__.py',
    'python\Lib\site-packages\numpy\__init__.py',
    'PyScripts\DAWorkbench\__init__.py'
)
foreach ($rel in $fixed) {
    if (-not (Test-Path (Join-Path $Stage $rel))) { Add-Missing $rel }
}

foreach ($sub in 'plugins', 'translations') {
    $count = @(Get-ChildItem (Join-Path $Stage $sub) -File -Recurse -ErrorAction SilentlyContinue).Count
    if ($count -lt 1) { Add-Missing "$sub\（目录为空或不存在）" }
}
$pakCount = @(Get-ChildItem (Join-Path $Stage 'resources') -Filter *.pak -ErrorAction SilentlyContinue).Count
if ($pakCount -lt 1) { Add-Missing 'resources\*.pak（QtWebEngine 资源）' }
if (-not (Test-Path (Join-Path $Stage 'QtWebEngineProcess.exe'))) { Add-Missing 'QtWebEngineProcess.exe' }

# ---------------------------------------------------------------------------
# 3. 按导入表校验 DLL
# ---------------------------------------------------------------------------
# VC++ 运行库必须随包：目标用户机器不一定装了 redist，靠系统目录兜不住
$mustBundle = @('msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')

$present = @(Get-ChildItem $Stage -Filter *.dll | ForEach-Object { $_.Name.ToLowerInvariant() })
$system32 = Join-Path $env:SystemRoot 'System32'

$peFiles = Get-ChildItem $Stage -Include *.exe, *.dll -File -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.DirectoryName -eq $Stage }
Write-Host "扫描 $($peFiles.Count) 个 PE 文件的导入表..."

$imports = @{}
foreach ($pe in $peFiles) {
    $out = & $dumpbin /nologo /dependents $pe.FullName 2>$null
    if ($LASTEXITCODE -ne 0) { continue }
    foreach ($line in $out) {
        $t = $line.Trim()
        if ($t -match '^([\w\-\.]+\.(dll|DLL))$') {
            $name = $Matches[1].ToLowerInvariant()
            if (-not $imports.ContainsKey($name)) { $imports[$name] = $pe.Name }
        }
    }
}

foreach ($dep in ($imports.Keys | Sort-Object)) {
    if ($present -contains $dep) { continue }                    # 包里已有
    if ($dep -like 'api-ms-win-*') { continue }                  # API set 转发
    if ($mustBundle -contains $dep) {
        # VC++ 运行库必须随包，不能指望目标机器装过 redist
        Add-Missing "$dep（VC++ 运行库必须随包，被 $($imports[$dep]) 依赖）"
        continue
    }
    if (Test-Path (Join-Path $system32 $dep)) { continue }       # 系统自带
    Add-Missing "$dep（被 $($imports[$dep]) 依赖，包内和系统里都没有）"
}

# ---------------------------------------------------------------------------
# 4. 汇总
# ---------------------------------------------------------------------------
if ($problems.Count -gt 0) {
    Write-Host ''
    Write-Host "::error::发布包存在 $($problems.Count) 个问题："
    foreach ($p in $problems) { Write-Host "  - $p" }
    exit 1
}

$totalMB = [math]::Round(((Get-ChildItem $Stage -Recurse -File | Measure-Object Length -Sum).Sum / 1MB), 1)
Write-Host ''
Write-Host "导入表校验通过（共 $($imports.Count) 个外部依赖全部可解析），目录总计 $totalMB MB"

# ---------------------------------------------------------------------------
# 5. 启动冒烟
# ---------------------------------------------------------------------------
if ($RunSmoke) {
    Write-Host ''
    Write-Host '--- 启动冒烟：DAWorkBench.exe --version ---'
    $exe = Join-Path $Stage 'DAWorkBench.exe'
    $outFile = Join-Path $env:RUNNER_TEMP 'pkg-smoke.out'
    $errFile = Join-Path $env:RUNNER_TEMP 'pkg-smoke.err'
    $proc = Start-Process -FilePath $exe -ArgumentList '--version' -WorkingDirectory $Stage -PassThru `
        -NoNewWindow -RedirectStandardOutput $outFile -RedirectStandardError $errFile

    if (-not $proc.WaitForExit(90000)) {
        $proc.Kill()
        Write-Host '::error::--version 90 秒未退出（可能弹窗或事件循环卡住）'
        exit 1
    }

    $stdout = if (Test-Path $outFile) { Get-Content $outFile -Raw } else { '' }
    $stderr = if (Test-Path $errFile) { Get-Content $errFile -Raw } else { '' }
    Write-Host "exit=$($proc.ExitCode)"
    if ($stdout) { Write-Host $stdout.Trim() }
    if ($stderr) { Write-Host $stderr.Trim() }

    if ($proc.ExitCode -eq -1073741515) {
        Write-Host '::error::启动失败：STATUS_DLL_NOT_FOUND（导入表校验没覆盖到的动态加载）'
        exit 1
    }
    if ($proc.ExitCode -ne 0) {
        Write-Host "::warning::--version 退出码非 0（$($proc.ExitCode)），请人工确认"
    } else {
        Write-Host '启动冒烟通过'
    }
}
