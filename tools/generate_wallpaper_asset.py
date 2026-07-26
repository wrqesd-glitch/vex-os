import argparse
import struct
from pathlib import Path

from PIL import Image


def render_wallpaper(input_path: Path, output_path: Path, width: int, height: int) -> None:
    with Image.open(input_path) as image:
        source = image.convert("RGB")

        scale = max(width / source.width, height / source.height)
        resized = source.resize(
            (max(1, int(round(source.width * scale))), max(1, int(round(source.height * scale)))),
            Image.Resampling.LANCZOS,
        )

        offset_x = max(0, (resized.width - width) // 2)
        offset_y = max(0, (resized.height - height) // 2)
        cropped = resized.crop((offset_x, offset_y, offset_x + width, offset_y + height))

        pixels = bytearray()
        raster = cropped.load()
        for y in range(height):
            for x in range(width):
                red, green, blue = raster[x, y]
                pixels.extend(struct.pack("<I", (red << 16) | (green << 8) | blue))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(pixels)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--width", required=True, type=int)
    parser.add_argument("--height", required=True, type=int)
    args = parser.parse_args()

    render_wallpaper(Path(args.input), Path(args.output), args.width, args.height)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
