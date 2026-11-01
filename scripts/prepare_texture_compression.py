#!/usr/bin/env python3
"""Prepare the texture compression teaching assets from a landscape photograph.

Requires Pillow, the optional Texture_Compression_Prepare CMake target, and Arm
astcenc. All required results are included in the repository for normal builds.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from PIL import Image, ImageOps


def prepare(source, directory, dxt_tool, astcenc):
    directory.mkdir(parents=True, exist_ok=True)
    with Image.open(source) as original:
        image = ImageOps.fit(original, (1024, 1024), Image.Resampling.LANCZOS).convert("RGBA")
    # Keep the real RGB photograph, adding a diagnostic alpha ramp at the bottom.
    alpha = Image.new("L", image.size, 255)
    alpha.paste(Image.frombytes("L", (1024, 256), bytes(round(x * 255 / 1023)
                for _ in range(256) for x in range(1024))), (0, 768))
    image.putalpha(alpha)
    image.save(directory / "landscape.png")
    with tempfile.TemporaryDirectory(prefix="texture-compression-") as temporary:
        raw = Path(temporary) / "source.rgba"
        raw.write_bytes(image.tobytes())
        subprocess.run([str(dxt_tool.resolve()), str(raw), "1024", "1024", str(directory)], check=True)
    for size in (4, 8, 12):
        astc = directory / f"landscape_{size}x{size}.astc"
        subprocess.run([str(astcenc.resolve()), "-cl", str(directory / "landscape.png"),
                        str(astc), f"{size}x{size}", "-medium", "-j", "2", "-silent"], check=True)
        subprocess.run([str(astcenc.resolve()), "-dl", str(astc),
                        str(directory / f"preview_{size}x{size}.png"), "-silent"], check=True)
    print(f"Prepared DDS/ASTC assets and CPU-decoded ASTC previews in {directory}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Original landscape photograph")
    parser.add_argument("--output", type=Path, default=Path("content/images/texture_compression"))
    parser.add_argument("--dxt-tool", type=Path, required=True)
    parser.add_argument("--astcenc", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.source, args.output, args.dxt_tool, args.astcenc)
