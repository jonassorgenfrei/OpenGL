# Texture lab

This sample compares a generated RGBA8 source image with two GPU block-compressed
representations and a software-managed virtual texture.
It starts in virtual-texture mode so page streaming is visible immediately.
It implements [ASTC + DXT Texturing issue #7](https://github.com/jonassorgenfrei/OpenGL/issues/7)
and [Virtual Texturing issue #42](https://github.com/jonassorgenfrei/OpenGL/issues/42).

| Key | Mode |
| --- | --- |
| `1` | RGBA8 source texture |
| `2` | DXT1 / BC1, encoded into 4x4 64-bit blocks |
| `3` | ASTC 4x4, encoded into 128-bit LDR void-extent blocks |
| `4` | 2048x2048 virtual texture backed by an 11x11 physical page cache |

In virtual-texture mode, use `WASD` or the arrow keys to pan, the mouse wheel or
`Q`/`E` to zoom, and `R` to reset the view. The window title reports residency,
upload activity, and the currently requested mip level.

DXT1 and ASTC are uploaded with `glCompressedTexImage2D`. If the active OpenGL
driver does not expose the corresponding extension, that mode uses the RGBA8
texture as a safe fallback and reports this in both the console and window title.
The console distinguishes an unavailable extension from a rejected compressed
upload and prints the active renderer to make driver capability issues explicit.

The virtual texture demonstrates a stacked mip page table, bounded physical
storage, LRU replacement, an upload budget, and one-pixel page gutters. It
selects the finest visible mip that fits the streaming cache and permanently
keeps the 2x2 and 1x1 mip levels resident. Missing detailed pages therefore
fall back to a complete coarse image instead of exposing cache holes or
thrashing when the view is zoomed out. The 16 MiB logical RGBA8 image uses
about 2.1 MiB of physical cache and page-table storage.
