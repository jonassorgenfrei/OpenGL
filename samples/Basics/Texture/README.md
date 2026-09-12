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
| `4` | 2048x2048 virtual texture backed by a 10x10 physical page cache |

In virtual-texture mode, use `WASD` or the arrow keys to pan, the mouse wheel or
`Q`/`E` to zoom, and `R` to reset the view. Missing pages are purple while the
cache incrementally fills. The window title reports residency and upload activity.

DXT1 and ASTC are uploaded with `glCompressedTexImage2D`. If the active OpenGL
driver does not expose the corresponding extension, that mode uses the RGBA8
texture as a safe fallback and reports this in both the console and window title.

The virtual texture demonstrates page-table indirection, bounded physical
storage, LRU replacement, an upload budget, and one-pixel page gutters. Its
logical image would require 16 MiB as RGBA8; the physical cache and page table
use about 1.7 MiB regardless of logical texture size.
