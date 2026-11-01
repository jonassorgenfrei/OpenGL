# Virtual texturing

Explore a real **16384 x 16384 satellite map** from NASA's Blue Marble:
Next Generation (July 2004, with topography). It covers Europe, northern Africa,
and western/central Asia. The sample starts on Italy's Adriatic coast, where
mountain ridges and coastlines make the difference between coarse and detailed
pages easy to recognize. The full-resolution image is never loaded on the GPU
or decoded into CPU memory at runtime. The main view shows streamed detail,
the upper-right overview marks your position in yellow, and the lower-right panel shows the actual
11 x 11 physical page cache. An on-screen panel explains memory use and reports
residency, pending requests, upload activity, evictions, and the selected mip.
Hover a slot in the physical cache to outline it and mark the corresponding
image region in **cyan** in both the main view and overview. The status line
identifies the slot, logical page, mip level, and full-resolution texel coverage.
Empty slots are labeled and have no image highlight. Cached regions outside the
current close-up remain visible in the overview. The mark denotes the page's
logical footprint; finer resident pages may currently supply some of its texels.
Requires OpenGL 3.3. The prepared image pack is included in the repository;
no downloads, Python installation, or sparse-texture extension are needed to run.

## What is the benefit?

| Storage | Texture payload |
| --- | --- |
| Full logical image, base level only | 1024 MiB |
| Physical cache: 726 x 726 RGBA8 | 2.011 MiB |
| Page table: 256 x 511 RGBA8 | 0.499 MiB |
| Cache + page table, allocated total | **2.51 MiB** |

Only pages needed near the current view are read from the image pack and
uploaded. Panning reuses cache slots instead of growing GPU pixel storage. The full 1 GiB image
is a calculated baseline; it is never allocated for comparison. Figures exclude
CPU storage, temporary upload buffers, driver overhead, and the UI. The JPEG
tile pack occupies about **51 MiB on disk** (including mip levels). A separate
CPU LRU cache holds at most four decoded disk tiles, about **1.27 MiB**, plus
the tile index and temporary decoder buffers. This disk compression is distinct
from virtual-texturing residency: uploaded pages use uncompressed RGBA8. A complete
conventional mip chain would increase the baseline further. The physical cache
has a fixed size; page-table storage grows with the logical page count.

This trades memory for an extra page-table lookup, CPU work, transfers, and
briefly coarser detail while pages arrive. It is useful when the working set is
much smaller than the whole image. It does not increase detail beyond what the
cache can hold simultaneously.

## Try this walkthrough

1. Start in the untinted close-up of Italy's coast. Watch the landscape sharpen
   and the cache slots fill. Press **V**: **green** means the requested mip is
   resident; **orange** means a coarser page is supplying the image. The coloring
   and page borders are optional so they do not obscure the actual image.
2. Press **Space**, then **C**. Streaming pauses and detailed pages are cleared.
   The complete image remains visible through the five permanently resident
   coarse pages, although it loses detail. The cache preview now shows only
   these fallback pages.
3. Press **B** to select one upload per frame, then **Space** to resume. Detail
   returns incrementally. Pending requests include a one-page prefetch border.
4. Hover the **physical-cache slots**. Match each cyan-outlined cache tile with
   its cyan footprint in the main image and overview. Compare a detailed page
   with one of the five pinned coarse pages: higher mips cover larger regions.
   Pause with **Space** to inspect a stable cache while moving the pointer.
5. Click a different land region in the **upper-right overview**, such as the
   Alps, the Nile, or the Himalayas, or pan with **WASD** to fill the cache.
   Its allocation stays fixed while the eviction counter increases and slots acquire new image regions.
6. Press **O** to see the whole image. The sample chooses a coarser mip whose
   requested pages fit the cache. Zoom back in or press **R** to restore the
   close-up over Italy. Press **V** again to hide residency colors.

## Controls

| Key | Action |
| --- | --- |
| `WASD` / arrow keys | Pan |
| Mouse wheel / `Q` / `E` | Zoom in / out |
| `Space` | Pause / resume detailed page streaming |
| `C` | Clear detailed pages; keep coarse fallback |
| `B` | Switch upload budget between 1 and 4 pages per frame |
| Hover physical cache | Mark the slot and its logical page footprint in cyan |
| Click overview | Jump to another region while keeping the current zoom |
| `V` | Toggle green/orange residency visualization |
| `O` / `R` | Whole-image view / reset close-up over Italy |
| `H` | Show keyboard help |
| `Esc` | Quit |

## How it works

The CPU computes the visible page rectangle, adds a one-page prefetch border,
and selects the finest mip that fits the 116 streaming slots. Missing pages
are requested near the view center first. LRU replacement protects the current
requested set and the five pinned pages (2 x 2 plus 1 x 1 at the coarsest levels).
The initial pinned uploads happen outside the per-frame detailed upload budget.
The page table is uploaded only when mappings change. A paused or settled view
therefore performs no redundant table transfer. If the cache size is reduced
so far that detailed pages cannot fit, the already pinned coarse mip is used.

Each 64 x 64 page has a one-pixel gutter on every side, occupying a 66 x 66 cache
slot. A vertically stacked mip page table maps logical pages to physical slots.
The fragment shader walks toward coarser levels when a mapping is missing,
then samples the cache with bilinear filtering inside the page and its gutters.
The cache itself has only a base level; the shader uses `textureLod(..., 0)`
because virtual mip selection has already happened in the page-table walk.

The source is an indexed pack of 5,463 JPEG disk tiles across nine mip levels.
Disk tiles cover up to 256 x 256 texels, plus a 16-pixel neighboring-image gutter.
Each can supply multiple 64 x 64 GPU pages. The `image_tiles` helper seeks and decodes
only requested disk tiles, retaining four decoded tiles with LRU replacement.
The on-screen tile-read counter shows file reads on CPU cache misses (the OS
may satisfy them from its own filesystem cache). The smaller GPU page gutters
are copied from those decoded neighboring texels. Mips are filtered offline,
so missing detail falls back to a blurred version of the same actual image.

This remains a software virtual-texturing teaching example: visibility is
computed on the CPU, mip choice is driven by cache capacity, and disk reads,
decoding, and GPU transfers are synchronous. It does not implement GPU feedback,
asynchronous streaming, or trilinear virtual mip filtering. The original version
implemented [issue #42](https://github.com/jonassorgenfrei/OpenGL/issues/42).

The implementation uses a plain `virtual_texture::State` with explicit
`initialize`, `update`, `clearDetail`, and `destroy` functions. GPU handles,
page mappings, LRU slots, and counters remain visible as data. The independent
`image_tiles::State` holds file/decoder state. CPU cache dimensions are passed
as uniforms so the shader and upload code agree when changing the cache size.

## Image source and rebuilding the asset

The bundled image is derived from NASA's
[Blue Marble: Next Generation with Topography, July, tile C1](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography/#july),
a 21600 x 21600 source downsampled to 16384 x 16384. Attribution, original download,
usage terms, and checksums are recorded in
[the asset notice](../../../content/images/virtual_texturing/blue_marble_july_c1.LICENSE.txt).
The prepared pack is `content/images/virtual_texturing/blue_marble_july_c1.vtpack`.

To rebuild it, download the original JPEG linked in that notice, then run:

```sh
python -m pip install Pillow==12.0.0
python scripts/build_virtual_texture.py world.topo.200407.3x21600x21600.C1.jpg content/images/virtual_texturing/blue_marble_july_c1.vtpack
```

Preparation needs several GiB of RAM to resample the source; it is an offline
step. Running or building the C++ example does not require Pillow. The pack uses
an eight-byte `VTPACK01` signature, six little-endian 32-bit header fields (image
size, page size, mip count, disk tile size, gutter, tile count), then one 64-bit
offset and 32-bit byte length per JPEG. Tiles are ordered by mip, then row and
column, with image rows already oriented for OpenGL.

Build just this sample from an already configured tree:

```sh
cmake --build build/windows --config Release --target Texture_VirtualTexturing
```

On Windows, run `bin/Basics/Release/Texture_VirtualTexturing.exe`. For other
platforms and dependency setup, see the [repository README](../../../README.md).
