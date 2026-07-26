#!/usr/bin/env python3
"""Vex OS build/test failure analyzer.

Usage:
    python tools/analyze_tests.py --build-dir build
    python tools/analyze_tests.py --build-dir build --qemu-output qemu.log
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Tuple


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = REPO_ROOT / "build"


class Issue:
    def __init__(self, file: str, line: int, severity: str, message: str, hint: str = "") -> None:
        self.file = file
        self.line = line
        self.severity = severity  # ERROR / WARN / FAIL
        self.message = message
        self.hint = hint

    def __str__(self) -> str:
        base = f"{self.file}:{self.line}: {self.severity}: {self.message}"
        if self.hint:
            base += f"\n  hint: {self.hint}"
        return base


def parse_build_output(text: str) -> List[Issue]:
    issues: List[Issue] = []
    for line in text.splitlines():
        m = re.search(r"^(.*?):(\d+):\s*(\d+):\s*(error|warning):\s*(.*)$", line)
        if m:
            issues.append(Issue(
                file=m.group(1),
                line=int(m.group(2)),
                severity="ERROR" if m.group(4) == "error" else "WARN",
                message=m.group(5),
                hint=_hint_for_message(m.group(5)),
            ))
        elif "error:" in line.lower():
            issues.append(Issue(
                file="unknown",
                line=0,
                severity="ERROR",
                message=line.strip(),
            ))
    return issues


def parse_ctest_output(text: str) -> List[Issue]:
    issues: List[Issue] = []
    for line in text.splitlines():
        if "not run" in line.lower() or "failed" in line.lower() or "error" in line.lower():
            issues.append(Issue(
                file="ctest",
                line=0,
                severity="FAIL",
                message=line.strip(),
            ))
    return issues


def parse_qemu_serial(text: str) -> Tuple[List[str], Optional[str]]:
    markers: List[str] = []
    panic_reason: Optional[str] = None
    for line in text.splitlines():
        if "vex:" in line:
            markers.append(line.strip())
        if any(tok in line for tok in ["vex:fault", "status=FAIL", "verify=FAIL", "panic", "exception"]):
            panic_reason = line.strip()
    return markers, panic_reason


def _hint_for_message(message: str) -> str:
    msg = message.lower()
    if "expected identifier or '('" in msg:
        return "Likely a stray '*/' in a comment or macro. Check block comments near this line."
    if "unknown type name 'u32'" in msg:
        return "Missing typedef / stdint include. Add `#include <stdint.h>` or a shared typedef header."
    if "redefinition" in msg:
        return "Duplicate symbol or macro. Search for earlier declaration with the same name."
    if "undeclared" in msg:
        return "Identifier used without declaration. Check includes or forward declarations."
    if "unused" in msg and "-Werror" in msg:
        return "Unused variable/function is fatal under -Werror. Either use it or mark with (void)cast."
    if "too many arguments" in msg or "too few arguments" in msg:
        return "Function call signature mismatch. Verify prototype vs definition."
    return ""


def run_build(build_dir: Path) -> Tuple[str, str]:
    proc = subprocess.run(
        ["cmake", "--build", str(build_dir), "--", "/v"] if sys.platform == "win32" else ["cmake", "--build", str(build_dir), "-v"],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    return proc.stdout, proc.stderr


def run_ctest(build_dir: Path) -> Tuple[str, str]:
    proc = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--output-on-failure"],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    return proc.stdout, proc.stderr


def build_report(build_issues: List[Issue], test_issues: List[Issue], serial_markers: List[str], panic_reason: Optional[str]) -> str:
    lines: List[str] = []
    lines.append("# Vex OS Test Analysis Report\n")
    lines.append(f"- build errors: {sum(1 for i in build_issues if i.severity == 'ERROR')}")
    lines.append(f"- build warnings: {sum(1 for i in build_issues if i.severity == 'WARN')}")
    lines.append(f"- test failures: {len(test_issues)}")
    lines.append(f"- serial markers: {len(serial_markers)}")
    if panic_reason:
        lines.append(f"- panic/fault marker: **{panic_reason}**")
    lines.append("")

    if build_issues:
        lines.append("## Build Issues\n")
        for issue in build_issues[:50]:
            lines.append(f"- {issue.file}:{issue.line}: **{issue.severity}** — {issue.message}")
            if issue.hint:
                lines.append(f"  - hint: {issue.hint}")
        if len(build_issues) > 50:
            lines.append(f"\n... and {len(build_issues) - 50} more issues.")
        lines.append("")

    if test_issues:
        lines.append("## Test Issues\n")
        for issue in test_issues[:50]:
            lines.append(f"- **{issue.severity}** — {issue.message}")
        lines.append("")

    if panic_reason:
        lines.append("## Boot Panic / Fault\n")
        lines.append(f"- {panic_reason}\n")

    if serial_markers:
        lines.append("## Serial Markers (first 40)\n")
        for marker in serial_markers[:40]:
            lines.append(f"- `{marker}`")
        lines.append("")

    lines.append("## Suggested Next Pass\n")
    if not build_issues and not test_issues and not panic_reason:
        lines.append("- Build and tests appear clean. If the screen is still black, capture QEMU serial output and re-run this analyzer with --qemu-output.")
    else:
        lines.append("- Fix build errors in the order listed above, then rerun `cmake --build <dir>`.")
        lines.append("- If the boot never reaches `vex:kernel:enter`, check loader/kernel ELF load path in boot/loader/src/main.c.")
        lines.append("- If `fb.width`/`fb.height` is 0 in serial logs, GOP negotiation failed — inspect capture_framebuffer() in boot/loader/src/main.c.")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Vex OS build/test failure analyzer")
    parser.add_argument("--build-dir", default=str(DEFAULT_BUILD_DIR))
    parser.add_argument("--qemu-output", default=None)
    args = parser.parse_args()
    build_dir = Path(args.build_dir)

    build_stdout, build_stderr = run_build(build_dir) if (build_dir / "build.ninja").exists() else ("", "")
    test_stdout, test_stderr = run_ctest(build_dir) if (build_dir / "CTestTestfile.cmake").exists() else ("", "")

    build_issues = parse_build_output(build_stdout + "\n" + build_stderr)
    test_issues = parse_ctest_output(test_stdout + "\n" + test_stderr)

    serial_markers: List[str] = []
    panic_reason: Optional[str] = None
    if args.qemu_output and Path(args.qemu_output).exists():
        qemu_text = Path(args.qemu_output).read_text(encoding="utf-8", errors="replace")
        serial_markers, panic_reason = parse_qemu_serial(qemu_text)

    report = build_report(build_issues, test_issues, serial_markers, panic_reason)
    print(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
