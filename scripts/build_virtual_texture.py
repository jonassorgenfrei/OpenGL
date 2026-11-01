#!/usr/bin/env python3
"""Offline preparation of the virtual-texturing sample's indexed JPEG tile pack.

Requires Pillow (runtime samples only need the existing stb_image dependency).
Run with a high-resolution square source; see the sample README for provenance.
"""
import argparse
import io
from pathlib import Path
import struct

from PIL import Image

VIRTUAL_SIZE = 16384
PAGE_SIZE = 64
MIP_LEVELS = 9
TILE_SIZE = 256
GUTTER = 16  # JPEG block aligned; pages have separate one-texel GPU gutters.
MAGIC = b"VTPACK01"


def padded_tile(image, x, y, size):
    """Include neighboring pixels, replicating the image's outermost edges."""
    tile = image.crop((x - GUTTER, y - GUTTER, x + size + GUTTER, y + size + GUTTER))
    extent = size + 2 * GUTTER
    if x == 0:
        tile.paste(tile.crop((GUTTER, 0, GUTTER + 1, extent)).resize((GUTTER, extent)), (0, 0))
    if x + size == image.width:
        tile.paste(tile.crop((extent - GUTTER - 1, 0, extent - GUTTER, extent)).resize((GUTTER, extent)), (extent - GUTTER, 0))
    if y == 0:
        tile.paste(tile.crop((0, GUTTER, extent, GUTTER + 1)).resize((extent, GUTTER)), (0, 0))
    if y + size == image.height:
        tile.paste(tile.crop((0, extent - GUTTER - 1, extent, extent - GUTTER)).resize((extent, GUTTER)), (0, extent - GUTTER))
    return tile


def build(source, output):
    # This trusted offline source exceeds Pillow's default image-size safeguard.
    # Runtime loads only small individual tiles, never this full source image.
    Image.MAX_IMAGE_PIXELS = None
    with Image.open(source) as original:
        if original.width != original.height or original.width < VIRTUAL_SIZE:
            raise ValueError("Expected a square source at least 16384 pixels wide")
        image = original.resize((VIRTUAL_SIZE, VIRTUAL_SIZE), Image.Resampling.LANCZOS).convert("RGB")
    image = image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    counts = [max(1, (VIRTUAL_SIZE >> mip) // TILE_SIZE) for mip in range(MIP_LEVELS)]
    count = sum(n * n for n in counts)
    output.parent.mkdir(parents=True, exist_ok=True)
    entries = []
    with output.open("wb") as pack:
        # Explicit little-endian fields; no compiler struct padding on disk.
        pack.write(MAGIC)
        pack.write(struct.pack("<6I", VIRTUAL_SIZE, PAGE_SIZE, MIP_LEVELS, TILE_SIZE, GUTTER, count))
        index_offset = pack.tell()
        pack.write(bytes(count * 12))  # uint64 offset, uint32 JPEG byte length
        for mip, dimension in enumerate(counts):
            size = min(TILE_SIZE, image.width)
            for y in range(dimension):
                for x in range(dimension):
                    tile = padded_tile(image, x * TILE_SIZE, y * TILE_SIZE, size)
                    encoded = io.BytesIO()
                    tile.save(encoded, format="JPEG", quality=85, subsampling=2, optimize=True)
                    payload = encoded.getvalue()
                    entries.append((pack.tell(), len(payload)))
                    pack.write(payload)
            print(f"Mip {mip}: {image.width} x {image.height}, {dimension * dimension} disk tiles", flush=True)
            if mip + 1 < MIP_LEVELS:
                image = image.resize((image.width // 2, image.height // 2), Image.Resampling.BOX)
        pack.seek(index_offset)
        for offset, length in entries:
            pack.write(struct.pack("<QI", offset, length))
    image.close()
    print(f"Saved {output}: {output.stat().st_size / (1024 * 1024):.2f} MiB, {count} tiles")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="High-resolution square source image")
    parser.add_argument("output", type=Path, help="Output .vtpack file")
    args = parser.parse_args()
    build(args.source, args.output)
