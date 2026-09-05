# run.ps1 — Agent 聊天历史重放性能回归测试（一键）
#
# 用真实的会话 jsonl 驱动 headless Chromium 加载项目内真实的 chat.js，
# 输出分段计时（初始渲染 / 分段加载 / 稳态重放 / scrollToBottom reflow 开销）
# 与正确性断言（分段全加载后 DOM 与一次性全量渲染等价、顺序正确、哨兵状态机正确）。
#
# 用法：
#   .\run.ps1 -SessionJsonl "C:\Users\<user>\AppData\Roaming\DAWorkBench\sessions\<id>.jsonl"
#
# 依赖：node（PATH 中）、Edge 或 Chrome（常用安装路径自动探测）。
# 背景：修复"切换大会话 UI 冻结半分钟"时建立的反馈回路（根因为 loadHistory
# 循环内逐事件 scrollToBottom 强制 reflow，O(n²)），保留用于防止回归。
param(
    [Parameter(Mandatory = $true)]
    [string]$SessionJsonl
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not (Test-Path $SessionJsonl)) {
    throw "session file not found: $SessionJsonl"
}

# 1) jsonl -> UI events（忠实复刻 DAAgentWebChannel::loadHistory 的 C++ 合并逻辑）
$eventsJs = Join-Path $here 'events.js'
node (Join-Path $here 'jsonl-to-events.js') $SessionJsonl $eventsJs
if ($LASTEXITCODE -ne 0) { throw 'jsonl-to-events.js failed' }

# 2) headless Chromium 加载 harness（引用 ../../src/DAGui/Agent/resources/chat.js）
$edgeCandidates = @(
    'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',
    'C:\Program Files\Microsoft\Edge\Application\msedge.exe',
    'C:\Program Files\Google\Chrome\Application\chrome.exe',
    'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe'
)
$browser = $edgeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $browser) { throw 'no Chromium-based browser (Edge/Chrome) found' }
$prof = Join-Path $env:TEMP 'da-chat-perf-profile'
if (Test-Path $prof) { Remove-Item $prof -Recurse -Force }
$domOut = Join-Path $here 'dom.html'
$harnessUrl = 'file:///' + ($here -replace '\\', '/') + '/harness.html'
& $browser --headless=new --disable-gpu --no-first-run --no-default-browser-check `
    "--user-data-dir=$prof" --window-size=480,800 --dump-dom $harnessUrl 2>$null |
    Out-File -FilePath $domOut -Encoding utf8

# 3) 解析结果
$raw = Get-Content $domOut -Raw
$idx = $raw.IndexOf('<pre id="perf-result">PERFRESULT')
if ($idx -lt 0) { $idx = $raw.LastIndexOf('PERFRESULT') }
if ($idx -lt 0) { throw 'perf result not found in dumped DOM (harness error?)' }
$s = $raw.IndexOf('{', $idx)
$e = $raw.IndexOf('</pre>', $s)
$result = (($raw.Substring($s, $e - $s).Trim()) -replace '\s+', ' ') | ConvertFrom-Json

# 4) 断言判定
$failures = @()
foreach ($check in @('EQUIV_bubbles', 'EQUIV_domNodes', 'EQUIV_users', 'EQUIV_questions', 'firstUserMatches')) {
    if ($result.$check -ne $true) { $failures += $check }
}
if ($result.sentinelAfterDrain) { $failures += 'sentinelAfterDrain' }
if ($result.pendingAfterDrain -ne 0) { $failures += 'pendingAfterDrain' }
if ($result.initialLoadMs -gt 2000) { $failures += 'initialLoadMs>2000' }
if ($result.steadyReplayMs -gt 500) { $failures += 'steadyReplayMs>500' }

Write-Host '=== Agent chat history replay perf ===' -ForegroundColor Cyan
$result | Format-List
if ($failures.Count -gt 0) {
    Write-Host "FAILED: $($failures -join ', ')" -ForegroundColor Red
    exit 1
}
Write-Host 'ALL CHECKS PASSED' -ForegroundColor Green
