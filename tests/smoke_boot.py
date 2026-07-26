import argparse
import queue
import subprocess
import sys
import threading
import time


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qemu", required=True)
    parser.add_argument("--ovmf-code", required=True)
    parser.add_argument("--ovmf-vars", required=True)
    parser.add_argument("--efi-root", required=True)
    args = parser.parse_args()

    command = [
        args.qemu,
        "-machine", "q35",
        "-m", "256M",
        "-cpu", "qemu64",
        "-display", "none",
        "-serial", "stdio",
        "-monitor", "none",
        "-no-reboot",
        "-drive", f"if=pflash,format=raw,readonly=on,file={args.ovmf_code}",
        "-drive", f"if=pflash,format=raw,file={args.ovmf_vars}",
        "-drive", f"format=raw,file=fat:rw:{args.efi_root}",
    ]

    proc = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    output_queue: "queue.Queue[str | None]" = queue.Queue()

    def read_output() -> None:
        assert proc.stdout is not None
        while True:
            chunk = proc.stdout.read(1)
            if not chunk:
                break
            output_queue.put(chunk)
        output_queue.put(None)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()

    expected = [
        "vex:kernel:enter",
        "vex:memory:regions=",
        "vex:vm:kernel_cr3=",
        "vex:exec:gdt=",
        "syscall=1",
        "vex:gfx:surfaces=",
        "selftest:PASS",
        "vex:init:domain=",
        "vex:init:cr3=",
        "vex:init:user_ctx cs=",
        "vex:services:count=",
        "status=READY name=diagnostics-view",
        "status=READY name=test-center",
        "status=READY name=explorer-console",
        "status=READY name=terminal-console",
        "status=READY name=compositor-service",
        "status=READY name=gpu-service",
        "surface=",
        "fence=",
        "vex:kernel:ready",
    ]
    output = ""
    deadline = time.time() + 40.0

    try:
        while time.time() < deadline and proc.poll() is None:
            try:
                chunk = output_queue.get(timeout=0.05)
            except queue.Empty:
                continue
            if chunk is None:
                break
            output += chunk
            if "vex:fault" in output or "status=FAIL" in output or "verify=FAIL" in output:
                break
            if all(token in output for token in expected):
                proc.terminate()
                proc.wait(timeout=5)
                return 0
    finally:
        if proc.poll() is None:
            proc.kill()
        reader.join(timeout=1.0)

    sys.stderr.write(output)
    sys.stderr.write("\nboot smoke failed\n")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
