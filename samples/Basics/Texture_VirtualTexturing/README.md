# Virtual texturing

This sample demonstrates a 2048x2048 logical texture backed by a bounded 11x11
physical page cache. It implements
[issue #42](https://github.com/jonassorgenfrei/OpenGL/issues/42).

| Key | Action |
| --- | --- |
| `WASD` / arrow keys | Pan |
| Mouse wheel / `Q` / `E` | Zoom |
| `R` | Reset the view |
| `Esc` | Quit |

The implementation uses a stacked mip page table, LRU replacement, four uploads
per frame, and one-pixel filtering gutters. It chooses the finest visible mip
that fits the streaming cache and permanently keeps the 2x2 and 1x1 mip levels
resident. Missing detailed pages therefore use a complete coarse image rather
than exposing holes or thrashing while zoomed out.

The logical RGBA8 image would occupy 16 MiB. Its physical cache and page table
use about 2.1 MiB, independent of the logical texture size. The title reports
cache occupancy, upload activity, active mip, and zoom level.
