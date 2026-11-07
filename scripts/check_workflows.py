#!/usr/bin/env python3
"""本地静态校验 GitHub Actions 工作流。

做两件事：
  1. YAML 能否解析、jobs/steps 结构是否齐全
  2. 把每个 pwsh 的 run 块导出成临时 .ps1，调用 pwsh 的官方 parser 检查语法

为什么需要：workflow 里的 PowerShell 语法错误只有在 CI 上才会暴露，
一次 30 分钟的编译跑完之后才发现"第 3 步引号少了一个"非常浪费。
（真实踩过：here-string 结束符顶格把整份 YAML 写坏）

用法：
    python scripts/check_workflows.py .github/workflows
退出码非 0 表示有问题。
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml

PS_CHECK = r"""
$bad = 0
$dir = $env:WF_CHECK_DIR
if (-not (Test-Path $dir)) { Write-Output "目录不存在: $dir"; exit 2 }
Get-ChildItem -Path $dir -Filter *.ps1 | ForEach-Object {
    $errors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($_.FullName, [ref]$null, [ref]$errors) | Out-Null
    if ($errors -and $errors.Count -gt 0) {
        $bad++
        Write-Output "FAIL $($_.Name)"
        $errors | Select-Object -First 5 | ForEach-Object {
            Write-Output ("   行 {0}: {1}" -f $_.Extent.StartLineNumber, $_.Message)
        }
    }
}
if ($bad -gt 0) { exit 1 } else { Write-Output "PS_OK" }
"""


def iter_pwsh_steps(doc: dict):
    for job_name, job in (doc.get("jobs") or {}).items():
        for idx, step in enumerate(job.get("steps") or []):
            shell = step.get("shell", "pwsh")
            run = step.get("run")
            if not run or not str(shell).startswith("pwsh"):
                continue
            label = step.get("name") or step.get("uses") or f"step#{idx}"
            yield f"{job_name}-{idx}", f"{job_name} / {label}", run


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".github/workflows")
    if not root.is_dir():
        print(f"目录不存在: {root}")
        return 2

    failures = 0
    fragments: list[tuple[str, str, str]] = []

    for path in sorted(root.glob("*.yml")):
        try:
            doc = yaml.safe_load(path.read_text(encoding="utf-8"))
        except Exception as exc:  # noqa: BLE001
            print(f"[FAIL] {path.name}: YAML 解析失败 -> {exc}")
            failures += 1
            continue
        if not isinstance(doc, dict) or "jobs" not in doc:
            print(f"[FAIL] {path.name}: 缺少 jobs 段")
            failures += 1
            continue
        steps = list(iter_pwsh_steps(doc))
        for slug, label, code in steps:
            fragments.append((f"{path.stem}-{slug}", label, code))
        print(f"[OK]   {path.name}: {len(doc['jobs'])} 个 job，{len(steps)} 个 PowerShell 片段")

    if failures:
        return 1

    pwsh = shutil.which("pwsh")
    if not pwsh:
        print("[SKIP] 没找到 pwsh，跳过 PowerShell 语法校验")
        return 0

    with tempfile.TemporaryDirectory(prefix="wf-check-") as tmp:
        tmp_dir = Path(tmp)
        for slug, label, code in fragments:
            (tmp_dir / f"{slug}.ps1").write_text(code, encoding="utf-8")
        result = subprocess.run(
            [pwsh, "-NoProfile", "-NonInteractive", "-Command", PS_CHECK],
            capture_output=True,
            text=True,
            env={**__import__("os").environ, "WF_CHECK_DIR": str(tmp_dir)},
        )
        out = (result.stdout or "").strip()
        if result.returncode != 0:
            print(out or result.stderr)
            print(f"[FAIL] {len(fragments)} 个片段中存在 PowerShell 语法错误")
            return 1
        print(f"[OK]   {len(fragments)} 个 PowerShell 片段语法全部通过")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
