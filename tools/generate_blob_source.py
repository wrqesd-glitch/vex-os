import argparse
from pathlib import Path


def emit_blob(blob: bytes, symbol: str) -> str:
    lines = []
    lines.append("#include <stddef.h>")
    lines.append("")
    lines.append(f"__attribute__((section(\".data\"))) unsigned char {symbol}[] = {{")
    for offset in range(0, len(blob), 12):
        chunk = blob[offset:offset + 12]
        encoded = ", ".join(f"0x{byte:02X}" for byte in chunk)
        lines.append(f"    {encoded},")
    lines.append("};")
    lines.append("")
    lines.append(f"__attribute__((section(\".data\"))) size_t {symbol}_size = sizeof({symbol});")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--symbol", required=True)
    args = parser.parse_args()

    source_path = Path(args.input)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    payload = source_path.read_bytes()
    output_path.write_text(emit_blob(payload, args.symbol), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
