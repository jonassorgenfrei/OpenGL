# Texture compression

A real **1024 x 1024 landscape photograph** is displayed beside the same image
in DXT1/BC1, DXT3/BC2, DXT5/BC3, or ASTC. The bottom panel shows payload size,
compression ratio, saved bytes/percentage, measured file-read time, completed
GPU-upload time, and total loading time. This follows the compressed-texture,
S3TC, DDS, alpha, and ASTC topics in `02_Materialmodelle/Texturen.tex`.

The reference is an opaque photograph with an **added alpha ramp in its bottom
quarter**. RGB detail is photographic throughout; the artificial transparency
band makes alpha loss, DXT3 quantization, and DXT5 interpolation easy to compare.
The left panel always supplies the corresponding uncompressed reference.

## Formats and memory

| Key | Format | Block | Bits/texel | Base payload | Reduction vs RGBA8 | Alpha |
| --- | --- | --- | --- | --- | --- | --- |
| `1` | RGBA8 | None | 32 | 4096 KiB | Reference | 8-bit |
| `2` | DXT1 / BC1 (RGB) | 4 x 4, 64 bits | 4 | 512 KiB | 8:1, 87.5% saved | Opaque |
| `3` | DXT3 / BC2 | 4 x 4, 128 bits | 8 | 1024 KiB | 4:1, 75% saved | Explicit 4-bit values |
| `4` | DXT5 / BC3 | 4 x 4, 128 bits | 8 | 1024 KiB | 4:1, 75% saved | Interpolated endpoint palette |
| `5` | ASTC 4 x 4 | 128 bits | 8 | 1024 KiB | 4:1, 75% saved | RGBA |
| `6` | ASTC 8 x 8 | 128 bits | 2 | 256 KiB | 16:1, 93.75% saved | RGBA |
| `7` | ASTC 12 x 12 | 128 bits | ~0.89 | 115.6 KiB | ~35.4:1, ~97.2% saved | RGBA |

DXT1 uses two RGB565 endpoints and sixteen 2-bit palette indices per block.
This demo uses `GL_COMPRESSED_RGB_S3TC_DXT1_EXT`, so alpha is always one. The
separate binary-alpha BC1 variant discussed in the slides is not selected here.
Against RGB8 instead of this RGBA8 reference, BC1 would have a 6:1 ratio.
DXT3 and DXT5 reuse the RGB block and add 64 bits of alpha information: sixteen
4-bit alpha values for DXT3, or two 8-bit endpoints plus sixteen 3-bit indices
for DXT5. Both BC3 alpha endpoint modes are considered by the offline encoder.

ASTC always uses 128-bit blocks. Increasing the footprint reduces storage, while
potentially losing more fine detail. The 1024-pixel edge is not divisible by 12:
ASTC 12 x 12 uses `ceil(1024/12)^2 = 86^2` blocks, not an idealized 36:1 ratio.

All formats have the same resolution, **one base level, and linear filtering**.
The payload figures exclude container headers, mip chains, and driver overhead.
They describe each image independently; the comparison application intentionally
keeps several variants resident at once. PNG/JPEG source-file compression is a
separate concept: GPU-ready block data stays compressed on a supported GPU and
is decoded by texture hardware as it is sampled.

## Try this walkthrough

1. Start with **DXT5**. Compare the photo detail against the reference. Check the
   bottom panel: 4096 KiB becomes 1024 KiB, saving 3072 KiB. The measured total
   load-time difference is shown independently of that fixed storage saving.
2. Press **2** for DXT1. It saves more space, but the transparency ramp disappears:
   this RGB format cannot preserve the source's continuous alpha.
3. Press **A** to display alpha. Switch between **3** and **4**: DXT3 shows its
   sixteen explicit opacity levels; DXT5 preserves this smooth ramp much better
   using a local interpolated palette. Neither is universally best for all masks.
4. Toggle **D** for amplified error: RGB differences are shown at 8x, alpha at
   16x when alpha view is enabled. The reference stays visible on the left.
5. Reset with **R**, then compare **5**, **6**, and **7**. Increase zoom with the
   wheel or **Z**, pan with arrow keys, and use **B** to expose block boundaries.
   Compare mountain ridges, clouds, and small foliage as ASTC payloads shrink.
6. Press **T** to repeat the loading measurements and observe normal variation.

## What do the loading measurements mean?

The application reads **already prepared DDS/ASTC assets**, then uploads their
payloads with `glTexImage2D` (RGBA8) or `glCompressedTexImage2D` (block formats).
There is **no runtime encoding** in the comparison. This matches the slide
pipeline: offline encoding -> file/container -> compressed upload -> compressed
GPU storage -> block decoding during sampling.

Each row has one warm-up followed by seven trials, using `steady_clock`:

- **Read ms:** file open/read, header validation, and CPU buffer allocations/copy.
- **Upload ms:** texture creation, parameters, allocation, upload, driver
  validation, and `glFinish` so submitted transfers are complete.
- **Load ms:** read + upload from the median total-time trial. Its read and
  upload components come from that same trial and therefore sum to the total.

`glFinish` before each trial excludes earlier rendering work. Texture deletion
happens after timing. Measurements use **warm OS file caches**; they are not
cold-storage bandwidth or pure GPU timer-query measurements. They exclude
PNG/JPEG decoding, offline encoding, fallback-preview preparation, scene
rendering, and application startup. DDS files add 128 bytes of header; ASTC
files add 16 bytes. Payload columns exclude those headers, but file reads include
them. The original PNG is retained for inspection/preparation, not timed loading.

Smaller transfers can improve loading, but fixed driver/allocation costs and
cache state can dominate. The bottom panel reports the actual faster/slower
percentage for the selected format on this run; it does not derive a fictitious
speedup from the compression ratio. Results depend on hardware and build mode.

## GPU support and previews

The sample checks S3TC and ASTC LDR extensions, then verifies that uploads retain
the requested internal format, compression flag, and expected payload size.
On unsupported/rejected formats, it displays a **CPU-decoded RGBA8 preview**:
DXT is decoded by the sample; ASTC previews were decoded offline by Arm astcenc.
The image title marks the preview and the table reports its actual 4096 KiB GPU
payload. Compressed **upload/load times are N/A**; reading the compressed file is
still measured. Preview decoding/upload is excluded from those measurements.
This keeps ASTC quality comparison usable on desktop GPUs without ASTC support.

## Controls

| Key | Action |
| --- | --- |
| `1`-`7` | Select reference, DXT1, DXT3, DXT5, ASTC 4 x 4 / 8 x 8 / 12 x 12 |
| `A` | Toggle alpha-channel view |
| `D` | Toggle amplified error on the comparison panel |
| `B` | Toggle block boundaries |
| Wheel / `Z` | Zoom / cycle 1x, 4x, 16x |
| Arrow keys | Pan the zoomed image |
| `R` | Reset the view |
| `T` | Repeat loading benchmark |
| `H` / `Esc` | Keyboard help / quit |

## Image source and offline preparation

The photo is [Fronalpstock by Hannes Roest](https://commons.wikimedia.org/wiki/File:Fronalpstock_big.jpg),
licensed CC BY-SA 3.0. The crop, alpha-band derivative, and all prepared image
variants retain that license; see the
[asset notice](../../../content/images/texture_compression/landscape.LICENSE.txt).
No network access, image encoder, or Python installation is required to build
or run the demo: all prepared assets are bundled under
`content/images/texture_compression/`.

The small BC encoders in `src/compression_encoders.h` favor readable teaching
code (RGB bounding-box endpoints and simple alpha endpoints). Their quality is
not a benchmark of production BC encoders. ASTC assets use the real
[Arm astcenc](https://github.com/ARM-software/astc-encoder) 5.7.0 encoder, medium
preset, rather than constant-color teaching blocks. Visual quality also depends
on encoder choices, not just the format.

To reproduce the assets from the original photo linked in the notice:

```sh
cmake --build build/windows --config Release --target Texture_Compression_Prepare
python -m pip install Pillow==12.0.0
python scripts/prepare_texture_compression.py Fronalpstock_big.jpg --dxt-tool bin/Release/Texture_Compression_Prepare.exe --astcenc /path/to/astcenc
```

`Texture_Compression_Prepare` is an optional CMake target excluded from normal
builds. On Linux, its executable is in the configured build directory. Arm's
encoder is an offline tool, not a linked runtime dependency. Source rows remain
top-to-bottom in DDS/ASTC; the shader applies `v = 1 - v` consistently, matching
the DDS orientation discussion in the slides. The loader deliberately accepts
only these prepared single-level DDS/ASTC files, not arbitrary texture containers.

Build/run the graphical example using the [repository setup](../../../README.md):

```sh
cmake --build build/windows --config Release --target Texture_Compression
```

On Windows, run `bin/Basics/Release/Texture_Compression.exe`.
The original sample implemented [issue #7](https://github.com/jonassorgenfrei/OpenGL/issues/7).
