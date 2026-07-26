import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path


def run_checked(args):
    return subprocess.run(args, check=True, capture_output=True, text=True)


def run_expected_failure(args):
    proc = subprocess.run(args, capture_output=True, text=True)
    if proc.returncode == 0:
        raise AssertionError("command unexpectedly succeeded")
    return proc


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vex", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--binary", required=True)
    args = parser.parse_args()

    vex = Path(args.vex)
    manifest = Path(args.manifest)
    binary = Path(args.binary)

    with tempfile.TemporaryDirectory(prefix="vex-pkg-test-") as temp_dir:
        temp_root = Path(temp_dir)
        key_path = temp_root / "signing.key"
        key_path.write_text(
            "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
            encoding="utf-8",
        )

        out_a = temp_root / "demo_a.vex"
        out_b = temp_root / "demo_b.vex"

        run_checked(
            [
                str(vex),
                "package",
                "--manifest",
                str(manifest),
                "--binary",
                str(binary),
                "--out",
                str(out_a),
                "--signing-key",
                str(key_path),
            ]
        )
        run_checked(
            [
                str(vex),
                "package",
                "--manifest",
                str(manifest),
                "--binary",
                str(binary),
                "--out",
                str(out_b),
                "--signing-key",
                str(key_path),
            ]
        )

        bytes_a = out_a.read_bytes()
        bytes_b = out_b.read_bytes()
        if bytes_a != bytes_b:
            raise AssertionError("package output is not deterministic")

        run_checked([str(vex), "verify", "--package", str(out_a)])

        inspect = run_checked([str(vex), "inspect", "--package", str(out_a)])
        meta = json.loads(inspect.stdout)
        if meta["manifest"]["name"] != "demo-shell":
            raise AssertionError("unexpected manifest name")
        if meta["format_version"] != 2:
            raise AssertionError("unexpected package format version")

        tampered = temp_root / "tampered.vex"
        shutil.copyfile(out_a, tampered)
        payload = bytearray(tampered.read_bytes())
        payload[-1] ^= 0x5A
        tampered.write_bytes(payload)

        failure = run_expected_failure([str(vex), "verify", "--package", str(tampered)])
        combined = failure.stdout + failure.stderr
        if "mismatch" not in combined and "signature verify" not in combined:
            raise AssertionError("tamper check failed for the wrong reason")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
