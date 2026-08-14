# DAWorkbench Build Script — Auto-detect Qt, VS, CMake
# Usage: .\build.ps1 [options]
# Options: -Target <name>  -Full  -Clean  -Test  -QtPath <path>
#          -VSVersion <2019|2022>  -Config <Release|Debug|...>
#          -Plugins <ON|OFF>
#
# Examples:
#   .\build.ps1                          # Full build (configure + build Release)
#   .\build.ps1 -Target DAPyWorkFlow     # Build one module only
#   .\build.ps1 -Target DAPyWorkFlow -Test  # Build module and run tests
#   .\build.ps1 -Clean                   # Clean rebuild
#   .\build.ps1 -Full -Config Debug      # Full debug build

param(
    [string]$Target = '',
    [switch]$Full,
    [switch]$Clean,
    [switch]$Test,

    [string]$QtPath = '',
    [string]$VSVersion = '',
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Config = 'Release',

    [ValidateSet('ON', 'OFF')]
    [string]$Plugins = 'ON'
)

$ErrorActionPreference = 'Stop'

# ============================================================
# Auto-detect Qt installation
# ============================================================
function Find-Qt {
    param([string]$HintPath)

    # If user provided a path, validate it
    if ($HintPath -and (Test-Path $HintPath)) {
        $cmakeDir = Join-Path $HintPath 'lib/cmake'
        if (Test-Path $cmakeDir) {
            Write-Host "[OK] Qt path (user-specified): $HintPath" -ForegroundColor Green
            return $HintPath
        }
        Write-Host "[WARN] User-specified Qt path exists but has no lib/cmake — will search anyway" -ForegroundColor Yellow
    }

    # Common Qt install locations (ordered by likelihood)
    $searchRoots = @(
        'D:\Qt',
        'C:\Qt',
        "${env:USERPROFILE}\Qt",
        "${env:ProgramFiles}\Qt",
        "${env:ProgramFiles(x86)}\Qt"
    )

    # Also check Qt installer's standard layout: <root>/<version>/<compiler>
    # e.g. D:\Qt\6.7.3\msvc2019_64
    foreach ($root in $searchRoots) {
        if (-not (Test-Path $root)) { continue }

        # Find msvc*_64 directories (64-bit MSVC builds)
        $qtDirs = Get-ChildItem -Path $root -Directory -Recurse -Depth 3 -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^msvc2019_64$|^msvc2022_64$' } |
            Sort-Object -Property { $_.Parent.Name } -Descending

        foreach ($dir in $qtDirs) {
            $candidate = $dir.FullName
            $cmakeDir = Join-Path $candidate 'lib/cmake'
            if (Test-Path $cmakeDir) {
                # Determine Qt version
                $qt6VersionFile = Join-Path $cmakeDir 'Qt6/Qt6Config.cmake'
                $qt5VersionFile = Join-Path $cmakeDir 'Qt5/Qt5Config.cmake'
                $qtVer = ''
                if (Test-Path $qt6VersionFile) { $qtVer = 'Qt6' }
                elseif (Test-Path $qt5VersionFile) { $qtVer = 'Qt5' }

                Write-Host "[OK] Qt path (auto-detected): $candidate ($qtVer)" -ForegroundColor Green
                return $candidate
            }
        }
    }

    Write-Host "[ERROR] Cannot find Qt installation!" -ForegroundColor Red
    Write-Host "  Searched: $($searchRoots -join ', ')" -ForegroundColor Red
    Write-Host "  Please install Qt or specify -QtPath explicitly" -ForegroundColor Red
    exit 1
}

# ============================================================
# Auto-detect Visual Studio & CMake generator
# ============================================================
function Find-VSGenerator {
    param(
        [string]$HintVersion,
        [string]$QtCompilerHint
    )

    # If a hint version is provided, use it directly
    if ($HintVersion) {
        switch ($HintVersion) {
            '2019' { $vsNum = '2019' }
            '2022' { $vsNum = '2022' }
            default {
                Write-Host "[ERROR] Unsupported VS version: $HintVersion" -ForegroundColor Red
                exit 1
            }
        }
    }
    else {
        # Prefer a VS version matching the Qt compiler, if known
        if ($QtCompilerHint -match 'msvc2019_64') {
            $vsNum = '2019'
        }
        elseif ($QtCompilerHint -match 'msvc2022_64') {
            $vsNum = '2022'
        }
        else {
            # Auto-detect newest installed VS
            $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
            if (Test-Path $vsWhere) {
                $installs = & $vsWhere -all -property installationVersion -format value 2>$null |
                    Sort-Object -Descending | Select-Object -First 1
                if ($installs) {
                    $major = $installs.Split('.')[0]
                    switch ($major) {
                        '16' { $vsNum = '2019' }
                        '17' { $vsNum = '2022' }
                        default { $vsNum = '2019' }
                    }
                }
                else {
                    $vsNum = '2019'
                }
            }
            else {
                $vsNum = '2019'
            }
        }
    }

    # Map VS version to CMake generator and default installation path
    switch ($vsNum) {
        '2019' {
            $genName = 'Visual Studio 16 2019'
            $vsPaths = @(
                'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community',
                'C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional',
                'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise'
            )
        }
        '2022' {
            $genName = 'Visual Studio 17 2022'
            $vsPaths = @(
                'C:\Program Files\Microsoft Visual Studio\2022\Community',
                'C:\Program Files\Microsoft Visual Studio\2022\Professional',
                'C:\Program Files\Microsoft Visual Studio\2022\Enterprise'
            )
        }
    }

    $foundPath = ''
    foreach ($p in $vsPaths) {
        if (Test-Path $p) {
            $foundPath = $p
            break
        }
    }

    if ($foundPath) {
        Write-Host "[OK] Visual Studio $vsNum found at: $foundPath" -ForegroundColor Green
    }
    else {
        Write-Host "[WARN] VS $vsNum not found at expected path, CMake may still find it via the generator" -ForegroundColor Yellow
    }

    Write-Host "[OK] CMake generator: $genName, architecture: x64" -ForegroundColor Green
    return @{ Generator = $genName; VSVersion = $vsNum; VSPath = $foundPath }
}

# ============================================================
# Auto-detect CMake executable
# ============================================================
function Find-CMake {
    # 1. Check PATH
    $pathCmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($pathCmake) {
        Write-Host "[OK] CMake (PATH): $($pathCmake.Source)" -ForegroundColor Green
        return 'cmake'
    }

    # 2. VS-embedded CMake locations
    $vsSearchPaths = @(
        'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    )

    foreach ($p in $vsSearchPaths) {
        if (Test-Path $p) {
            Write-Host "[OK] CMake (VS-embedded): $p" -ForegroundColor Green
            return $p
        }
    }

    # 3. Standalone CMake
    $standalonePaths = @(
        'C:\Program Files\CMake\bin\cmake.exe',
        'C:\Program Files (x86)\CMake\bin\cmake.exe'
    )
    foreach ($p in $standalonePaths) {
        if (Test-Path $p) {
            Write-Host "[OK] CMake (standalone): $p" -ForegroundColor Green
            return $p
        }
    }

    Write-Host "[ERROR] Cannot find CMake!" -ForegroundColor Red
    Write-Host "  Install CMake or add it to PATH" -ForegroundColor Red
    exit 1
}

# ============================================================
# Help
# ============================================================
if ($Target -eq 'help' -or $args -contains '-help' -or $args -contains '--help') {
    Write-Host @'
DAWorkbench Build Script
========================

Usage: .\build.ps1 [options]

Options:
  -Target <name>          Build a specific CMake target (e.g. DAPyWorkFlow)
  -Full                   Build all targets (default if no target is specified)
  -Clean                  Remove the build directory and rebuild from scratch
  -Test                   Run tests after the build (via ctest)
  -QtPath <path>          Qt installation path (auto-detect if omitted)
  -VSVersion <2019|2022>  Visual Studio version (auto-detect if omitted)
  -Config <Release|Debug|RelWithDebInfo|MinSizeRel>  Build configuration
  -Plugins <ON|OFF>       Build plugins (default: ON)

Examples:
  .\build.ps1                           # Full build (configure + build Release)
  .\build.ps1 -Target DAPyWorkFlow      # Build one module only
  .\build.ps1 -Target DAPyWorkFlow -Test   # Build module and run tests
  .\build.ps1 -Clean                    # Clean rebuild
  .\build.ps1 -Full -Config Debug       # Full debug build

Auto-detection:
  Qt:     Searches D:\Qt, C:\Qt, ~\Qt, Program Files\Qt for msvc*_64 dirs
  VS:     Uses the Qt compiler version as a hint, then vswhere.exe
  CMake:  Checks PATH, then VS-embedded, then standalone locations

Important:
  - This script MUST use the Visual Studio generator (not Ninja) because
    vcvars64.bat cannot inject the MSVC environment into PowerShell.
  - Qt compiler version must match VS version (e.g. Qt msvc2019 with VS2019).
'@
    exit 0
}

# ============================================================
# Detect environment
# ============================================================
Write-Host "`n=== DAWorkbench Build Environment Detection ===" -ForegroundColor Cyan

$qtDir = Find-Qt -HintPath $QtPath
$qtCompiler = Split-Path $qtDir -Leaf
$vsInfo = Find-VSGenerator -HintVersion $VSVersion -QtCompilerHint $qtCompiler
$cmakeExe = Find-CMake

# Determine Qt major version for summary
$qt6Cmake = Join-Path $qtDir 'lib/cmake/Qt6/Qt6Config.cmake'
$isQt6 = Test-Path $qt6Cmake
$qtMajor = if ($isQt6) { '6' } else { '5' }

Write-Host "`n=== Build Configuration ===" -ForegroundColor Cyan
Write-Host "  Qt:          $qtDir (Qt$qtMajor)"
Write-Host "  VS:          $($vsInfo.VSVersion)"
Write-Host "  Generator:   $($vsInfo.Generator)"
Write-Host "  CMake:       $cmakeExe"
Write-Host "  Config:      $Config"
Write-Host "  Plugins:     $Plugins"
if ($Target) {
    Write-Host "  Target:      $Target"
}
else {
    Write-Host "  Target:      ALL (default)"
}
Write-Host "  Run tests:   $Test"
Write-Host ""

# ============================================================
# Execute actions
# ============================================================
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $projectRoot 'build'

# --- CLEAN ---
if ($Clean) {
    if (Test-Path $buildDir) {
        # Check for running executables in build/bin that might lock files
        $binDir = Join-Path $buildDir 'bin'
        if (Test-Path $binDir) {
            $runningExes = Get-ChildItem -Path $binDir -Filter '*.exe' -Recurse -ErrorAction SilentlyContinue |
                Where-Object {
                    try {
                        [System.IO.File]::Open($_.FullName, 'Open', 'ReadWrite').Close()
                        $false
                    }
                    catch {
                        $true
                    }
                }
            if ($runningExes) {
                Write-Host "[WARN] Locked executables detected (processes still running):" -ForegroundColor Yellow
                $runningExes | ForEach-Object { Write-Host "  $($_.Name)" -ForegroundColor Yellow }
                Write-Host "  Kill them first, or skip clean" -ForegroundColor Yellow
                $cont = Read-Host "Continue with clean? (Y/N)"
                if ($cont -ne 'Y') {
                    Write-Host "Clean skipped." -ForegroundColor Yellow
                    exit 0
                }
            }
        }
        Write-Host "[CLEAN] Removing build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force $buildDir
        Write-Host "[OK] Build directory removed" -ForegroundColor Green
    }
    else {
        Write-Host "[OK] No build directory to clean" -ForegroundColor Green
    }
}

# --- CONFIGURE ---
$needsConfigure = $Clean -or -not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))
if ($needsConfigure) {
    Write-Host "[CONFIGURE] Running cmake configure..." -ForegroundColor Cyan

    $cmakeArgs = @(
        '-S', $projectRoot,
        '-B', $buildDir,
        '-G', $vsInfo.Generator,
        '-A', 'x64',
        "-DCMAKE_PREFIX_PATH=$qtDir",
        "-DDA_BUILD_PLUGINS=$Plugins"
    )

    # Print the command for debugging
    Write-Host "  Command: $cmakeExe $($cmakeArgs -join ' ')" -ForegroundColor Gray

    & $cmakeExe $cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] CMake configure failed (exit code $LASTEXITCODE)" -ForegroundColor Red
        Write-Host "  Common causes:" -ForegroundColor Red
        Write-Host "  - Qt path wrong: check -QtPath, current=$qtDir" -ForegroundColor Red
        Write-Host "  - Qt compiler mismatch: Qt msvc version must match VS version" -ForegroundColor Red
        Write-Host "  - Missing 3rdparty libraries: compile src/3rdparty first (see build.md)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Host "[OK] CMake configure succeeded" -ForegroundColor Green
}

# --- BUILD ---
Write-Host "[BUILD] Running cmake build ($Config)..." -ForegroundColor Cyan

$buildArgs = @(
    '--build', $buildDir,
    '--config', $Config
)
if ($Target) {
    $buildArgs += @('--target', $Target)
}

Write-Host "  Command: $cmakeExe $($buildArgs -join ' ')" -ForegroundColor Gray
& $cmakeExe $buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed (exit code $LASTEXITCODE)" -ForegroundColor Red
    Write-Host "  Try: .\build.ps1 -Clean (clean + configure + build)" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "[OK] Build succeeded" -ForegroundColor Green

# --- TEST ---
if ($Test) {
    Write-Host "[TEST] Running tests ($Config)..." -ForegroundColor Cyan

    $testArgs = @(
        '-C', $Config,
        '--output-on-failure'
    )
    if ($Target) {
        # Map a target to a CTest regex when possible; otherwise run all
        $testArgs += @('-R', $Target)
    }

    Write-Host "  Command: ctest $($testArgs -join ' ')" -ForegroundColor Gray
    & ctest $testArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Tests failed (exit code $LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Host "[OK] Tests passed" -ForegroundColor Green
}

# Show output artifacts
$binOutDir = Join-Path $buildDir "bin/$Config"
if (Test-Path $binOutDir) {
    Write-Host "`n=== Build Artifacts ===" -ForegroundColor Cyan
    Get-ChildItem -Path $binOutDir -Filter '*.dll' -ErrorAction SilentlyContinue |
        ForEach-Object { Write-Host "  DLL: $($_.Name)" }
    Get-ChildItem -Path $binOutDir -Filter '*.exe' -ErrorAction SilentlyContinue |
        ForEach-Object { Write-Host "  EXE: $($_.Name)" }
}

Write-Host "`n=== Done ===" -ForegroundColor Green
