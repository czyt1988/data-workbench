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
# 通过 vswhere 定位任意版本的 VS（本机 CI 与本机既可能是 2019 也可能是 2022），
# 不再写死 ProgramFiles\...\2022\* —— 写死会在 VS2019 环境直接误判「找不到 dumpbin」。
$dumpbin = $null
$cmd = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if ($cmd) {
    $dumpbin = $cmd.Source
} else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -property installationPath 2>$null
        if ($vsPath) {
            $fi = Get-ChildItem (Join-Path $vsPath 'VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe') -ErrorAction SilentlyContinue |
                Sort-Object FullName | Select-Object -Last 1
            if ($fi) { $dumpbin = $fi.FullName }
        }
    }
}
if (-not $dumpbin) {
    $fi = @(
        Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" -ErrorAction SilentlyContinue
        Get-ChildItem "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" -ErrorAction SilentlyContinue
    ) | Sort-Object FullName | Select-Object -Last 1
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

# ---------------------------------------------------------------------------
# 2b. Python 运行时依赖（Q4①：soundfile 为强制依赖，缺失即发布包不合格）
# ---------------------------------------------------------------------------
# soundfile 是音频导入的首选解码器（requirements-runtime.txt 已列出）。Q4 拍板①后
# 它不再是「可降级项」：发布包缺 soundfile / libsndfile 时直接判失败，避免发出去的包
# 在用户机器上才发现音频导入不可用（之前用 ::warning:: 只告警，等于没测）。
# soundfile 依赖 libsndfile：新版 wheel 放在 _soundfile_data 子包，旧版放在 soundfile/ 同目录；
# soundfile 0.12 及以前是单文件 soundfile.py，新版可能是包目录，两种布局都要认。
$sitePackages = Join-Path $Stage 'python\Lib\site-packages'
if (-not (Test-Path $sitePackages)) {
    Add-Missing 'python\Lib\site-packages（随包 Python 运行时缺失）'
}
else {
    $soundfileFound = @(
        (Join-Path $sitePackages 'soundfile.py'),
        (Join-Path $sitePackages 'soundfile\__init__.py')
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $soundfileFound) {
        Add-Missing 'soundfile（音频解码强制依赖缺失，requirements-runtime.txt 已列出；Q4①）'
    }
    else {
        Write-Host "soundfile 包存在: $soundfileFound"
    }
    $libsndfile = @(Get-ChildItem $sitePackages -Recurse -File -Filter 'libsndfile*.dll' -ErrorAction SilentlyContinue)
    if ($libsndfile.Count -lt 1) {
        Add-Missing 'libsndfile*.dll（soundfile 的本地解码库缺失，音频导入不可用）'
    }
    else {
        Write-Host ("libsndfile 检查通过（{0}）" -f $libsndfile[0].Name)
    }
    # 真导入冒烟：只查文件不够，DLL 架构/ABI 不匹配、_soundfile_data 缺失只有 import 才会暴露。
    # 用 cmd /c 包裹，避免 $ErrorActionPreference='Stop' 把 python 的 stderr 当终止错误。
    $py = Join-Path $Stage 'python\python.exe'
    if (Test-Path $py) {
        $probeOut = Join-Path $env:TEMP 'dawb-soundfile-probe.txt'
        cmd /c "`"$py`" -c `"import soundfile, sys; fmts = soundfile.available_formats(); sys.exit(0 if fmts else 3)`" > `"$probeOut`" 2>&1"
        $probeCode = $LASTEXITCODE
        if ($probeCode -ne 0) {
            $probeMsg = if (Test-Path $probeOut) { (Get-Content $probeOut -Raw).Trim() } else { '' }
            Add-Missing ("soundfile 导入/解码能力自检失败（exit {0}）：{1}" -f $probeCode, $probeMsg)
        }
        else {
            Write-Host 'soundfile 导入与 libsndfile 可用性自检通过'
        }
    }
    else {
        Write-Host "::warning::未找到 $py，跳过 soundfile 导入冒烟（无法确认随包 Python 能否解码音频）"
    }
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
    # 用 .NET Process 直接重定向：PowerShell 5.1 的 Start-Process -PassThru + RedirectStandardOutput
    # 对 GUI 子系统程序会拿不到 ExitCode（本机实测为 $null，导致「退出码非 0（）」误报），
    # .NET 路径在 PS5.1/PS7 上都能稳定拿到退出码与输出。
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = '--version'
    $psi.WorkingDirectory = $Stage
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
    $stderrTask = $proc.StandardError.ReadToEndAsync()

    if (-not $proc.WaitForExit(90000)) {
        $proc.Kill()
        Write-Host '::error::--version 90 秒未退出（可能弹窗或事件循环卡住）'
        exit 1
    }

    $exitCode = $proc.ExitCode
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    Write-Host "exit=$exitCode"
    if ($stdout) { Write-Host $stdout.Trim() }
    if ($stderr) { Write-Host $stderr.Trim() }

    if ($exitCode -eq -1073741515) {
        Write-Host '::error::启动失败：STATUS_DLL_NOT_FOUND（导入表校验没覆盖到的动态加载）'
        exit 1
    }
    if ($exitCode -ne 0) {
        Write-Host "::warning::--version 退出码非 0（$exitCode），请人工确认"
    } else {
        Write-Host '启动冒烟通过'
    }
}
